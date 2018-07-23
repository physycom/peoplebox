#!/usr/bin/env python3

import matplotlib.pyplot as plt
import json
import datetime
import os
from collections import OrderedDict
import numpy as np

times = {}
dates = {}
count = {}
cumulate = {}

#path = os.environ['WORKSPACE'] + os.path.sep + 'peoplebox' + os.path.sep + 'data' + os.path.sep + 'barriera_900'
#requires_fit=True

path = os.environ['WORKSPACE'] + os.path.sep + 'peoplebox' + os.path.sep + 'data' + os.path.sep + 'newfit'
requires_fit=False

tags=[
'20180716_1200',
'20180716_1600',
'20180716_2000',
'20180717_0800',
'20180717_1200',
#'20180717_1200_1',
#'20180717_1200_2',
'20180719_2000',
'20180720_0800']

gt = {
      '20180716_1200' : {
              'BETA_CNAF-IN'  : 312,
              'BETA_CNAF-OUT' : 169
              },
      '20180716_1600' : {
              'BETA_CNAF-IN'  : 107,
              'BETA_CNAF-OUT' : 377
              },
      '20180716_2000' : {
              'BETA_CNAF-IN'  : 96,
              'BETA_CNAF-OUT' : 154
              },
      '20180717_0800' : {
              'BETA_CNAF-IN'  : 271,
              'BETA_CNAF-OUT' : 107
              },
      '20180717_1200' : {
              'BETA_CNAF-IN'  : 292,
              'BETA_CNAF-OUT' : 179
              },
      '20180717_1200_1' : {
              'BETA_CNAF-IN'  : 143,
              'BETA_CNAF-OUT' : 103
              },
      '20180717_1200_2' : {
              'BETA_CNAF-IN'  : 130,
              'BETA_CNAF-OUT' : 67
              },
      '20180719_2000' : {
              'BETA_CNAF-IN'  : 207,
              'BETA_CNAF-OUT' : 237
              },
      '20180720_0800' : {
              'BETA_CNAF-IN'  : 300,
              'BETA_CNAF-OUT' : 104
              }
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
prob_in = 1.25
prob_out = 0.99
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
    files=os.listdir(path + os.path.sep + tag)
    for file in files:
        timestamp=int(file.split(os.path.sep)[-1].split('.')[0].split('_')[-1])
        #if timestamp < timestamp_min or timestamp > timestamp_max:
            #continue

        with open(path + os.path.sep + tag + os.path.sep + file) as f:
            data = json.load(f, object_pairs_hook=OrderedDict)

        for key in data:
            if not count[tag]:
                count[tag][data[key]["people_count"][0]["id"]] = []
                count[tag][data[key]["people_count"][1]["id"]] = []
                times[tag] = []
                dates[tag] = []
            times[tag].append(data[key]["timestamp"])
            dates[tag].append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
            if requires_fit:
                count[tag][data[key]["people_count"][0]["id"]].append(t(data[key]["people_count"][0]["count"], prob_in))
                count[tag][data[key]["people_count"][1]["id"]].append(t(data[key]["people_count"][1]["count"], prob_out))
            else:
                count[tag][data[key]["people_count"][0]["id"]].append(data[key]["people_count"][0]["count"])
                count[tag][data[key]["people_count"][1]["id"]].append(data[key]["people_count"][1]["count"])

    for loc in count[tag]:
        cumulate[tag][loc] = np.cumsum(count[tag][loc])

print("%20s : %-13s %5s %5s %4s%%" % ('time','location', 'track', 'gt', 'err'))
for tag in tags:
    a=[print("%20s : %-13s %5d %5d %4d%%" % (tag, loc, cumulate[tag][loc][-1], gt[tag][loc], int((cumulate[tag][loc][-1] - gt[tag][loc])/gt[tag][loc]*100))) for loc in cumulate[tag]]


#%%
if False:
    tag=tags[5]
    fig, (ax, ax1, ax2) = plt.subplots(1,3)
    subsample=1
    [ax.plot(times[tag][::subsample], count[::subsample], '-o', label=label) for label, count in count[tag].items()]
    [ax1.plot(times[tag][::subsample], cumul[::subsample], '-o', label=label) for label, cumul in cumulate[tag].items()]
    ax2.hist([count for label, count in count[tag].items()], bins=np.arange(10))
    ax.set_title("Counts vs time")
    ax.set_xticks(times[tag][0::subsample])
    ax.set_xticklabels(dates[tag][0::subsample], rotation=90, fontsize=10)
    ax.legend()
    ax1.set_title("Cumulative Counts vs time")
    ax1.set_xticks(times[tag][0::subsample])
    ax1.set_xticklabels(dates[tag][0::subsample], rotation=90, fontsize=10)
    ax1.legend()
    ax2.set_title("Counts histogram")
    ax2.set_xticks(np.arange(10))
    ax2.legend()
    plt.savefig("report.png")
    plt.show()
