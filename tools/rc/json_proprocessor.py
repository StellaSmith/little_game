#!/usr/bin/python3

import json
import sys

contents = sys.stdin.read()
try:
    json.dump(json.loads(contents), sys.stdout, indent=None, separators=(",", ":"))
except Exception as e:
    sys.stdout.write(contents)