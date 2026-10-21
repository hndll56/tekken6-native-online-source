#!/usr/bin/env python3
"""Initialize the exact SDK from either a Git checkout or a source ZIP."""
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
lock=json.loads((ROOT/'DEPENDENCIES.lock.json').read_text())
sdk=ROOT/'third_party/rexglue-sdk'
def git(*args,cwd=ROOT): subprocess.run(['git',*map(str,args)],cwd=cwd,check=True)
if not (sdk/'.git').exists():
    if sdk.exists() and any(sdk.iterdir()): sys.exit('SDK directory is not empty; refusing to replace it')
    git('clone','--no-checkout','--depth','1',lock['sdk_repository'],sdk)
    git('fetch','--depth','1','origin',lock['sdk_revision'],cwd=sdk)
    git('checkout','--detach',lock['sdk_revision'],cwd=sdk)
actual=subprocess.check_output(['git','rev-parse','HEAD'],cwd=sdk,text=True).strip()
if actual!=lock['sdk_revision']: sys.exit(f'Expected SDK {lock["sdk_revision"]}, got {actual}; refusing to reset local work')
git('submodule','update','--init','--recursive','--depth','1','--jobs','4',cwd=sdk)
subprocess.run([sys.executable,ROOT/'tools/apply_rexglue_patches.py'],check=True)
