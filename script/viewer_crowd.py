#! /usr/bin/env python3

import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict
import glob

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-w", "--work-dir", help="working folder (no trailing /)", default=".")
parser.add_argument("-r", "--file-regex", help="json name regex", default="*.json")
parser.add_argument("-b", "--bar-tag", help="tag to match in selecting displayed barriers", default="")
parser.add_argument("-s", "--subsample", help="x-axis label subsampling", default=1, type=int)
parser.add_argument("-o", "--output", help="output image basename (no path)", default="")
args = parser.parse_args()

times = []
dates = []
flux = {}

import os
dir_regex = args.work_dir + os.sep + args.file_regex
files = sorted(glob.glob(dir_regex))

try:
  for i, file in enumerate(files):
    print("Processing %3d/%d) file : %s" % (i+1, len(files), file))

    with open(file) as f:
      data = json.load(f, object_pairs_hook=OrderedDict)

    for key in data:
      times.append(data[key]["timestamp"])
      dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
      for cnt in data[key]["people_count"]:
        flux.setdefault(cnt["id"], []).append(cnt["count"])
except:
  import sys
  print("EXC: Problems in parsing json files.")
  sys.exit(1)

fig, ax = plt.subplots(1,1)
[ax.plot(times, count, '-o', label=label) for label, count in flux.items() if label.startswith(args.bar_tag)]
ax.set_xticks(times[0::args.subsample])
ax.set_xticklabels(dates[0::args.subsample], rotation=45, fontsize=10)
plt.legend()
plt.tight_layout()

if args.output == "":
  output = args.work_dir + os.sep + "barriers.png"
else:
  output = args.work_dir + os.sep + args.output
print("Saving plot to : %s" % output)
plt.savefig(output)
plt.show()
