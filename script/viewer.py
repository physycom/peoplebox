#!/usr/bin/env python3

import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict
import glob
import numpy as np

times = []
dates = []
flux = {}

#files = sorted(glob.glob('PAPADOPOLI_15277399*'))
#files = sorted(glob.glob('PAPA*'))
files = sorted(glob.glob('ORESI/ORESI*'))
for file in files:
  #print(file)

  with open(file) as f:
    data = json.load(f, object_pairs_hook=OrderedDict)

  for key in data:
    if not flux:
      flux[data[key]["people_count"][0]["id"]] = []
      flux[data[key]["people_count"][1]["id"]] = []
    times.append(data[key]["timestamp"])
    dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
    flux[data[key]["people_count"][0]["id"]].append(data[key]["people_count"][0]["count"])
    flux[data[key]["people_count"][1]["id"]].append(data[key]["people_count"][1]["count"])

cumulate = {}
for loc in flux:
  #print(loc)
  cumulate[loc] = np.cumsum(flux[loc])

fig, (ax, ax1) = plt.subplots(1,2)
subsample=100
[ax.plot(times[::subsample], count[::subsample], '-o', label=label) for label, count in flux.items()]
[ax1.plot(times[::subsample], cumul[::subsample], '-o', label=label) for label, cumul in cumulate.items()]
ax.set_xticks(times[0::subsample])
ax.set_xticklabels(dates[0::subsample], rotation=90, fontsize=10)
ax1.set_xticks(times[0::subsample])
ax1.set_xticklabels(dates[0::subsample], rotation=90, fontsize=10)
plt.legend()
plt.savefig("report.png")
plt.show()
