#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
def run(args):subprocess.run(args,cwd=root,check=True)
with tempfile.TemporaryDirectory() as tmp:
 for source in sorted((root/"tests").glob("*.cpp")):
  binary=str(Path(tmp)/source.stem)
  run([os.environ.get("CXX","c++"),"-std=c++17","-O2",str(source),"-o",binary]);run([binary])
for source in sorted((root/"tests").glob("*.cjs")):run(["node",str(source)])
for source in sorted((root/"tests").glob("*.py")):run(["python3",str(source)])
print("All regression suites passed.")
