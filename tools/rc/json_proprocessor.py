#!/usr/bin/python3

import json
import sys

json.dump(json.load(sys.stdin), sys.stdout, indent=None, separators=(",", ":"))