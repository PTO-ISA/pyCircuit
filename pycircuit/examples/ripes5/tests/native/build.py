"""Rebuild the unmodified pinned Ripes/VSRTL CPU and observation adapter."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from ..paths import ROOT
PIN = {'ripes':'5b8a616edcb6f0a2ddb07e78951348b72497f1e1', 'vsrtl':'8497dd14fe80e57efcff4c424a9a3b6363d93eb7'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qt',type=Path,default=ROOT/'pycircuit/builds/ripes-qt')
    parser.add_argument('--output',type=Path,default=ROOT/'pycircuit/builds/ripes-native')
    parser.add_argument('--jobs',type=int,default=6)
    args=parser.parse_args()
    source=ROOT/'reference/ripes-reference'
    for name,path in [('ripes',source),('vsrtl',source/'external/VSRTL')]:
        commit=subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()
        dirty=subprocess.check_output(['git','-C',str(path),'status','--porcelain'],text=True).strip()
        if commit!=PIN[name] or dirty:
            raise RuntimeError(f'{name} must be clean at {PIN[name]}; got {commit}, dirty={bool(dirty)}')
    env=dict(os.environ,CCACHE_DISABLE='1')
    env['LD_LIBRARY_PATH']=str(args.qt/'lib')+':/home/lc/opt/gcc14/lib:'+env.get('LD_LIBRARY_PATH','')
    command=['cmake','-S',str(source),'-B',str(args.output),'-G','Ninja',
             '-DCMAKE_CXX_COMPILER=/home/lc/opt/pycircuit-dev/bin/c++','-DCMAKE_BUILD_TYPE=Release',
             '-DCMAKE_PREFIX_PATH='+str(args.qt),
             '-DCMAKE_PROJECT_Ripes_INCLUDE='+str(Path(__file__).with_name('targets.cmake')),
             '-DOPENGL_INCLUDE_DIR='+str(args.qt/'include'),
             '-DOPENGL_opengl_LIBRARY='+str(args.qt/'lib/libOpenGL.so'),
             '-DOPENGL_glx_LIBRARY='+str(args.qt/'lib/libGLX.so')]
    for dependency in ('libelfin','elfio','signals','magic-enum','cereal'):
        candidate=ROOT/'reference'/dependency
        if not candidate.exists():
            candidate=ROOT/'reference/builds/ripes-reference/_deps'/(dependency+'-src')
        if not candidate.exists():
            raise RuntimeError('missing reference dependency '+str(candidate))
        command.append('-DFETCHCONTENT_SOURCE_DIR_'+dependency.upper()+'='+str(candidate))
    args.output.mkdir(parents=True,exist_ok=True)
    with (args.output/'configure.log').open('w') as out:
        subprocess.run(command,env=env,stdout=out,stderr=subprocess.STDOUT,check=True)
    with (args.output/'build.log').open('w') as out:
        subprocess.run(['cmake','--build',str(args.output),'--target','ripes5-reference','-j',str(args.jobs)],env=env,stdout=out,stderr=subprocess.STDOUT,check=True)
    binary=args.output/'ripes5-reference'
    identity=json.loads(subprocess.check_output([str(binary),'--identity'],env=env,text=True))
    assert identity==PIN
    evidence=dict(commits=PIN,unmodified_reference_sources=True,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  adapter_sha256=hashlib.sha256(Path(__file__).with_name('runner.cpp').read_bytes()).hexdigest(),configure=command,
                  ecall_observation='capture pre-edge arguments; complete output post-edge after older stores commit; finalize exit pre-edge')
    (args.output/'provenance.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps(evidence,indent=2))

if __name__=='__main__':main()
