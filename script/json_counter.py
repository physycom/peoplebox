#!/usr/bin/env python3

import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict
import glob
import numpy as np

times = {}
dates = {}
count = {}
cumulate = {}

#files = sorted(glob.glob('/Users/sinigard/Desktop/movie_dump/json/PAPADOPOLI_*'))
path = 'X:/sinigard/Codice/peoplebox/script/data/w0.5'
tags = ['0800', '1200', '1600', '2000']

gt = {
      '0800' : {
              'BETA_CNAF-IN'  : 271,
              'BETA_CNAF-OUT' : 107
              },
      '1200' : {
              'BETA_CNAF-IN'  : 312,
              'BETA_CNAF-OUT' : 169
              },
      '1600' : {
              'BETA_CNAF-IN'  : 107,
              'BETA_CNAF-OUT' : 377
              },
      '2000' : {
              'BETA_CNAF-IN'  : 96,
              'BETA_CNAF-OUT' : 154
              },
      }
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
prob_in = 0.90
prob_out = 1.15
np.random.seed(12345)
def t(n, prob):
    nt = n
    for i in np.random.uniform(size=n):
        if i < (prob-int(prob)):
            nt += 1
        nt += int(prob)
    return nt

#%%

for tag in tags:
    count[tag] = {}
    cumulate[tag] = {}
    times[tag] = {}
    dates[tag] = {}
    for file in sorted(glob.glob(path + '/' + tag + '/*')):
      timestamp=int(file.split('/')[-1].split('.')[0].split('_')[-1])
    #  timestamp=int(file.split('\\')[-1].split('.')[0].split('_')[-1])
    #  if timestamp < timestamp_min or timestamp > timestamp_max: 
    #      continue
    
      with open(file) as f:
        data = json.load(f, object_pairs_hook=OrderedDict)
    
      for key in data:
        if not count[tag]:
          count[tag][data[key]["people_count"][0]["id"]] = []
          count[tag][data[key]["people_count"][1]["id"]] = []
          times[tag] = []
          dates[tag] = []
        times[tag].append(data[key]["timestamp"])
        dates[tag].append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
        count[tag][data[key]["people_count"][0]["id"]].append(t(data[key]["people_count"][0]["count"], prob_in))
        count[tag][data[key]["people_count"][1]["id"]].append(t(data[key]["people_count"][1]["count"], prob_out))

    for loc in count[tag]:
      cumulate[tag][loc] = np.cumsum(count[tag][loc])
    
    a=[print("%5s : %-13s %5d %5d %5d%%" % (tag, loc, cumulate[tag][loc][-1], gt[tag][loc], int((cumulate[tag][loc][-1] - gt[tag][loc])/gt[tag][loc]*100))) for loc in cumulate[tag]]
    

#%%

tag='0800'
fig, (ax, ax1) = plt.subplots(1,2)
subsample=10
[ax.plot(times[tag][::subsample], count[::subsample], '-o', label=label) for label, count in count[tag].items()]
[ax1.plot(times[tag][::subsample], cumul[::subsample], '-o', label=label) for label, cumul in cumulate[tag].items()]
ax.set_xticks(times[tag][0::subsample])
ax.set_xticklabels(dates[tag][0::subsample], rotation=90, fontsize=10)
ax1.set_xticks(times[tag][0::subsample])
ax1.set_xticklabels(dates[tag][0::subsample], rotation=90, fontsize=10)
plt.legend()
plt.savefig("report.png")
plt.show()
