#!/usr/bin/env python3
"""Concurrent publication and overflow behavior for the framebuffer lifecycle trace."""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='fb-controller-lifecycle-trace-') as d:
 b=Path(d)/'check'
 subprocess.run(['clang++','-std=c++14','-O1','-g','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(r/'WhateverXe'),str(r/'tests/fb_controller_lifecycle_trace.cpp'),'-o',str(b)],check=True)
 subprocess.run([str(b)],check=True)
