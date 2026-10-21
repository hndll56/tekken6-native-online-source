#!/usr/bin/env python3
"""Syntax-check all handwritten runtime translation units without proprietary game code.
This does not compile generated game code, link the game or prove gameplay.
"""
import argparse
import concurrent.futures
import os
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler',default=os.environ.get('CXX','clang++'))
p.add_argument('--jobs',type=int,default=2)
args=p.parse_args()
sdk=ROOT/'third_party/rexglue-sdk'
includes=[ROOT/'src', ROOT/'third_party/ggpo/src/include',sdk/'include']
for path in ['simde','fmt/include','spdlog/include','sdl3/include','imgui','vulkan-headers/include','vulkan-memory-allocator/include']:
    includes.append(sdk/'thirdparty'/path)
flags=['-std=c++23','-march=x86-64-v2','-fsyntax-only','-DREX_PLATFORM_LINUX=1','-DREX_HAS_VULKAN=1']+['-I'+str(i) for i in includes]
sources=[*sorted((ROOT/'src').glob('t6_*.cpp')),ROOT/'src/tekken6_app.cpp']
def check(file):
    result=subprocess.run([args.compiler,*flags,str(file)],cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    return file.name,result.returncode,result.stdout
with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,args.jobs)) as pool:
    results=list(pool.map(check,sources))
for name,code,output in results:
    print(('FAIL ' if code else 'PASS ')+name)
    if output: print(output)
print(f'{sum(not code for _,code,_ in results)}/{len(results)} handwritten units passed; generated main and game are not checked')
sys.exit(int(any(code for _,code,_ in results)))
