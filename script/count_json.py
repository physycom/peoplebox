#!/usr/bin/env python3

import json
import sys
from collections import OrderedDict

datafile = sys.argv[1]
data = json.load(open(datafile), object_pairs_hook=OrderedDict)

IN  = sum([frame["people_count"][0]["count"] for _, frame in data.items()])
OUT = sum([frame["people_count"][1]["count"] for _, frame in data.items()])

print("IN  : ", IN)
print("OUT : ", OUT)
