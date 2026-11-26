#!/usr/bin/env python3
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
subprocess.run(["python3",str(root/"AquariumHome/bundle_page.py")],check=True)
subprocess.run(["arduino-cli","compile","--fqbn","esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=default","--output-dir",str(root/"build"),str(root/"AquariumHome")],check=True)
