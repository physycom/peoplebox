#!/usr/bin/env python3

import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict
import glob
import numpy as np

times = []
dates = []
count = {}
count_norm = {}
cumulate = {}
cumulate_norm = {}

#files = sorted(glob.glob('/Users/sinigard/Desktop/movie_dump/json/PAPADOPOLI_*'))
files = sorted(glob.glob('C:/Users/Alessandro/Desktop/data/w0.5/1200/*'))

# 20180716, from 1200 to 1215 (UTC: from 1000 to 1015)
#timestamp_min=1531735200
#timestamp_max=1531736100

# 20180716, from 1600 to 1615 (UTC: from 1400 to 1415)
#timestamp_min=1531749600
#timestamp_max=1531750500

# 20180716, from 2000 to 2015 (UTC: from 1800 to 1815)
#timestamp_min=1531764000
#timestamp_max=1531764900

# 20180717, from 0800 to 0815 (UTC: from 0600 to 0615)
#timestamp_min=1531807200
#timestamp_max=1531808100


#%%

for file in files:
#  timestamp=int(file.split('/')[-1].split('.')[0].split('_')[-1])
  timestamp=int(file.split('\\')[-1].split('.')[0].split('_')[-1])
#  if timestamp < timestamp_min or timestamp > timestamp_max: 
#      continue

  with open(file) as f:
    data = json.load(f, object_pairs_hook=OrderedDict)

  for key in data:
    if not count:
      count[data[key]["people_count"][0]["id"]] = []
      count[data[key]["people_count"][1]["id"]] = []
    times.append(data[key]["timestamp"])
    dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
    count[data[key]["people_count"][0]["id"]].append(data[key]["people_count"][0]["count"])
    count[data[key]["people_count"][1]["id"]].append(data[key]["people_count"][1]["count"])

for loc in count:
    cumulate[loc] = np.cumsum(count[loc])

#%%

for loc in count:
    count_norm[loc]=np.array(1.95*np.array(count[loc])-0.5, dtype=int)
    cumulate_norm[loc] = np.cumsum(count_norm[loc])

#%%

histo = {}
bins=np.arange(0,10)
for loc in count:
    histo[loc] = np.histogram(count[loc], bins=bins)

plt.hist([count[loc] for loc in count], bins=bins)

#%%

fig, (ax, ax1) = plt.subplots(1,2)
subsample=1
[ax.plot(times[::subsample], count[::subsample], '-o', label=label) for label, count in count.items()]
[ax1.plot(times[::subsample], cumul[::subsample], '-o', label=label) for label, cumul in cumulate.items()]
ax.set_xticks(times[0::subsample])
ax.set_xticklabels(dates[0::subsample], rotation=90, fontsize=10)
ax1.set_xticks(times[0::subsample])
ax1.set_xticklabels(dates[0::subsample], rotation=90, fontsize=10)
plt.legend()
plt.savefig("report.png")
plt.show()
