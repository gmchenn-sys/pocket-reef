#!/usr/bin/env python3
"""Verify the release files and flash four segments, preserving NVS by default."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--port",required=True)
p.add_argument("--erase",action="store_true",help="Erase all previous firmware and aquarium ownership data")
p.add_argument("--dry-run",action="store_true")
a=p.parse_args();root=Path(__file__).resolve().parents[1];folder=root/"docs/firmware"
manifest=json.loads((root/"docs/manifest.json").read_text())
checks={line.split()[1]:line.split()[0] for line in (folder/"SHA256SUMS").read_text().splitlines() if line.strip()}
parts=manifest["builds"][0]["parts"]
for item in parts:
 f=root/"docs"/item["path"]
 if hashlib.sha256(f.read_bytes()).hexdigest()!=checks[f.name]:raise SystemExit("Checksum mismatch: "+f.name)
base=[sys.executable,"-m","esptool","--chip","esp32s3","--port",a.port,"--baud","460800"]
commands=[]
if a.erase:commands.append(base+["erase-flash"])
commands.append(base+["write-flash","--flash-mode","keep","--flash-freq","keep","--flash-size","keep"]+[arg for item in parts for arg in [hex(item["offset"]),str(root/"docs"/item["path"])]])
for cmd in commands:
 print(subprocess.list2cmdline(cmd),flush=True)
 if not a.dry_run:subprocess.run(cmd,check=True)
