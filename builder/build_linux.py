#!/usr/bin/env python3
"""Build the native ELF game from the user's own supported Xbox 360 ISO/folder.

Usage: python3 builder/build_linux.py <ISO-or-folder> | --check <path> | --update | --relink
No Windows compiler, prebuilt DLL, Wine or Proton is used.
"""
from __future__ import annotations
import os
from pathlib import Path
import shutil
import subprocess
import sys
import json
import shlex
import build_native as original

SOURCE = Path(__file__).resolve().parents[1]
HOME = Path(os.environ.get('T6_HOME', Path(os.environ.get('XDG_DATA_HOME', Path.home()/'.local/share'))/'tekken6-native-online')).expanduser().resolve()
OUT = HOME/'native'
GAME = OUT/'game'
PROJECT = OUT/'project'
SDK = SOURCE/'third_party/rexglue-sdk'
SDK_BUILD = OUT/'sdk-build'
GAME_BUILD = OUT/'cmake-game'
LOG = None

def say(text):
    print(text, flush=True)
    if LOG:
        LOG.write(text+'\n')
        LOG.flush()

def run(args, cwd=SOURCE):
    say('  '+str(args[0])+' '+ ' '.join(map(str,args[1:])))
    with subprocess.Popen(list(map(str,args)), cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace') as process:
        tail=[]
        for line in process.stdout:
            if LOG: LOG.write(line)
            tail=(tail+[line.rstrip()])[-12:]
        if process.wait():
            raise RuntimeError(f'{args[0]} failed: '+ '\n'.join(tail)+f'\nFull log: {OUT / "build.log"}')

def compiler(name):
    result=shutil.which(os.environ.get('CXX' if name=='clang++' else 'CC',name))
    if not result:
        result=next((shutil.which(f'{name}-{version}') for version in range(22,17,-1) if shutil.which(f'{name}-{version}')),None)
    if not result: raise RuntimeError(f'{name} 18 or newer is required')
    return result

def cmake_config(source, build, *extra):
    run(['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
         f'-DCMAKE_C_COMPILER={compiler("clang")}',f'-DCMAKE_CXX_COMPILER={compiler("clang++")}',
         '-DCMAKE_CXX_FLAGS=-march=x86-64-v2 '+shlex.join([f'-ffile-prefix-map={SOURCE}=./source',f'-ffile-prefix-map={HOME}=./userdata'])+' '+os.environ.get('CXXFLAGS',''),'-DREXGLUE_USE_VULKAN=ON','-DREXGLUE_USE_D3D12=OFF',
         '-DREXGLUE_ENABLE_TRACY=OFF',f'-DREXGLUE_OUTPUT_BASE={OUT / "sdk-out"}', *extra])

def iso_tool(*args):
    candidates=[Path(os.environ.get('T6_XDVDFS','/nonexistent')),SOURCE/'build/components/tools/xdvdfs/xdvdfs_tool',OUT/'tools/xdvdfs_tool']
    tool=next((p for p in candidates if p.is_file() and os.access(p,os.X_OK)),None)
    if tool is None:
        tool_build=OUT/'tools'
        run(['cmake','-S',SOURCE/'tools/xdvdfs','-B',tool_build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'])
        run(['cmake','--build',tool_build,'--parallel','2'])
        tool=tool_build/'xdvdfs_tool'
    return [str(tool),*map(str,args)]

def check(path):
    original.OUT=str(OUT)
    original.xdvdfs=iso_tool
    return original.check(str(path))

def stage_project():
    PROJECT.mkdir(parents=True,exist_ok=True)
    for name in ['CMakeLists.txt','cmake','src','tests','tools','third_party']:
        dst=PROJECT/name
        if dst.is_symlink():
            if dst.resolve()!=SOURCE/name: dst.unlink()
            else: continue
        if dst.exists(): raise RuntimeError(f'Refusing to overwrite unexpected staging path: {dst}')
        dst.symlink_to(SOURCE/name, target_is_directory=(SOURCE/name).is_dir())
    generated=PROJECT/'generated'
    generated.mkdir(exist_ok=True)
    shutil.copy2(SOURCE/'generated/rexglue.cmake',generated/'rexglue.cmake')
    shutil.copy2(SOURCE/'tekken6_config.toml',PROJECT/'tekken6_config.toml')
    xex=original.find_xex(str(GAME))
    # JSON quoted paths are also valid TOML basic strings (escapes/Unicode/spaces included).
    manifest='[project]\nname = "tekken6"\nsdk_version = "0.10.0"\ngame_root = '+json.dumps('../game')+'\n\n[entrypoint]\nfile_path = '+json.dumps('../game/'+Path(xex).name)+'\nout_directory_path = "generated/default"\nincludes = ["tekken6_config.toml"]\n'
    (PROJECT/'tekken6_manifest.toml').write_text(manifest)

def main(argv):
    global LOG
    if argv==['--help'] or not argv:
        print(__doc__)
        return 0 if argv else 2
    os.environ.setdefault("SOURCE_DATE_EPOCH", str(json.loads((SOURCE/"DEPENDENCIES.lock.json").read_text())["source_date_epoch"]))
    OUT.mkdir(parents=True,exist_ok=True)
    if len(argv)==2 and argv[0]=='--check':
        what,info=check(Path(argv[1]).expanduser().resolve())
        say(('OK ' if what else 'ERROR ')+str(info))
        return 0 if what else 1
    if argv in [['--update'],['--relink']]: source=GAME
    elif len(argv)==1: source=Path(argv[0]).expanduser().resolve()
    else: raise RuntimeError('Expected one ISO/folder or --update / --relink')
    with (OUT/'build.log').open('w',encoding='utf-8') as LOG:
        say('STEP 1/6 Checking the game copy')
        what,info=check(source)
        if not what: raise RuntimeError(str(info))
        say('PROGRESS 2')
        say('STEP 2/6 Preparing game data')
        if source!=GAME:
            if GAME.exists():
                what,existing=check(GAME)
                if not what: raise RuntimeError(f'{GAME} contains a different/incomplete copy; move it aside before rebuilding: {existing}')
            elif source.is_dir(): shutil.copytree(source,GAME)
            else: run(iso_tool('extract-all',source,GAME))
        say('PROGRESS 10')
        say('STEP 3/6 Building the pinned native ReXGlue SDK')
        if not (SDK/'CMakeLists.txt').exists(): raise RuntimeError('Initialize the pinned SDK: git submodule update --init --recursive')
        run([sys.executable,SOURCE/'tools/apply_rexglue_patches.py'])
        cmake_config(SDK,SDK_BUILD)
        jobs=os.environ.get('T6_JOBS','2')
        run(['cmake','--build',SDK_BUILD,'--target','rexglue','--parallel',jobs])
        codegen=OUT/'sdk-out/linux-amd64/rexglue'
        if not codegen.exists(): raise RuntimeError(f'Native code generator not found: {codegen}')
        stage_project()
        run([codegen,'-f','codegen',PROJECT/'tekken6_manifest.toml'],cwd=PROJECT)
        say('PROGRESS 35')
        say('STEP 4/6 Applying all netplay code patches')
        for patch in original.PATCH_TOOLS:
            run([sys.executable,'-I',SOURCE/'builder/project/tools'/f'{patch}.py',PROJECT/'generated/default'])
        say('PROGRESS 45')
        say('STEP 5/6 Compiling native Linux game and Vulkan runtime')
        cmake_config(PROJECT,GAME_BUILD,f'-DREXSDK_DIR={SDK}','-DT6_BUILD_GAME=ON','-DT6_BUILD_LAUNCHER=OFF','-DBUILD_TESTING=OFF')
        run(['cmake','--build',GAME_BUILD,'--target','tekken6','--parallel',jobs])
        say('STEP 6/6 Staging executable and shared libraries')
        exe=GAME_BUILD/'tekken6'
        if not exe.is_file(): raise RuntimeError(f'Missing output {exe}')
        for binary in [exe,*GAME_BUILD.glob('*.so*'),*(OUT/'sdk-out/linux-amd64').glob('*.so*')]:
            temp=OUT/(binary.name+'.new')
            shutil.copy2(binary,temp)
            temp.replace(OUT/binary.name)
        shutil.copy2(SOURCE/'builder/build.version',OUT/'build.version')
        say('PROGRESS 100')
        say(f'DONE {OUT / "tekken6"}')
    return 0

if __name__=='__main__':
    try:
        raise SystemExit(main(sys.argv[1:]))
    except (OSError,RuntimeError,subprocess.SubprocessError) as exc:
        print(f'ERROR {exc}',flush=True)
        raise SystemExit(1)
