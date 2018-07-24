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
scaling_factor = {}
fitted = {}

id_in='BETA_CNAF-IN'
id_out='BETA_CNAF-OUT'

#path = os.environ['WORKSPACE'] + os.path.sep + 'peoplebox' + os.path.sep + 'data' + os.path.sep + 'barriera_900'
#requires_fit=True
#requires_norm=False

#path = os.environ['WORKSPACE'] + os.path.sep + 'peoplebox' + os.path.sep + 'data' + os.path.sep + 'newfit'
#requires_fit=False
#requires_norm=False

path = os.environ['WORKSPACE'] + os.path.sep + 'peoplebox' + os.path.sep + 'data' + os.path.sep + 'scan'
requires_fit=True
requires_norm=True

barriers=[
#          '700_700',
#          '750_750',
#          '800_800',
#          '850_850',
#          '900_900',
#          '950_950',
#          '1000_1000',
#          '1050_1050',
          '1100_1100',
#          '1150_1150',
#          '1200_1200'
          ]

tags=[
      '20180716-1200',
      '20180716-1600',
#      '20180716-2000',
      '20180717-0800',
      '20180717-1200',
      #'20180717-1200-1',
      #'20180717-1200-2',
      '20180719-2000',
      '20180720-0800',
      '20180720-1200'
      ]

gt = {
      '20180716-1200' : {
              'BETA_CNAF-IN'  : 312,
              'BETA_CNAF-OUT' : 169
              },
      '20180716-1600' : {
              'BETA_CNAF-IN'  : 107,
              'BETA_CNAF-OUT' : 377
              },
      '20180716-2000' : {
              'BETA_CNAF-IN'  : 96,
              'BETA_CNAF-OUT' : 154
              },
      '20180717-0800' : {
              'BETA_CNAF-IN'  : 271,
              'BETA_CNAF-OUT' : 107
              },
      '20180717-1200' : {
              'BETA_CNAF-IN'  : 292,
              'BETA_CNAF-OUT' : 179
              },
      '20180717-1200-1' : {
              'BETA_CNAF-IN'  : 143,
              'BETA_CNAF-OUT' : 103
              },
      '20180717-1200-2' : {
              'BETA_CNAF-IN'  : 130,
              'BETA_CNAF-OUT' : 67
              },
      '20180719-2000' : {
              'BETA_CNAF-IN'  : 207,
              'BETA_CNAF-OUT' : 237
              },
      '20180720-0800' : {
              'BETA_CNAF-IN'  : 300,
              'BETA_CNAF-OUT' : 104
              },
      '20180720-1200' : {
              'BETA_CNAF-IN'  : 212,  #pepe 228
              'BETA_CNAF-OUT' : 157   #bazz 144
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
#prob_in = 1.25
#prob_out = 0.99
prob_in = 0.944
prob_out = 0.9089
np.random.seed(12345)
def t(n, prob):
    nt = n
    for i in np.random.uniform(size=n):
        if i < (prob-int(prob)):
            nt += 1
        nt += int(prob)
    return nt

def get_sf(dic):
    sf = {}
    sf[id_in] = 0.0
    sf[id_out] = 0.0
    for tag in tags:
        sf[id_in] += dic[tag][id_in]
        sf[id_out] += dic[tag][id_out]
    sf[id_in] /= len(tags)
    sf[id_out] /= len(tags)
    return sf


#%%

for barrier in barriers:
    count[barrier] = {}
    cumulate[barrier] = {}
    times[barrier] = {}
    dates[barrier] = {}
    scaling_factor[barrier] = {}
    for tag in tags:
        count[barrier][tag] = {}
        cumulate[barrier][tag] = {}
        times[barrier][tag] = {}
        dates[barrier][tag] = {}
        scaling_factor[barrier][tag] = {}
        folder=path + os.path.sep + tag + '_' + barrier
        files=os.listdir(folder)
        for file in files:
            timestamp=int(file.split(os.path.sep)[-1].split('.')[0].split('_')[-1])
            #if timestamp < timestamp_min or timestamp > timestamp_max:
                #continue

            with open(folder + os.path.sep + file) as f:
                data = json.load(f, object_pairs_hook=OrderedDict)

            for key in data:
                if not count[barrier][tag]:
                    count[barrier][tag][data[key]["people_count"][0]["id"]] = []
                    count[barrier][tag][data[key]["people_count"][1]["id"]] = []
                    times[barrier][tag] = []
                    dates[barrier][tag] = []
                times[barrier][tag].append(data[key]["timestamp"])
                dates[barrier][tag].append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
                if requires_fit:
                    count[barrier][tag][data[key]["people_count"][0]["id"]].append(t(data[key]["people_count"][0]["count"], prob_in))
                    count[barrier][tag][data[key]["people_count"][1]["id"]].append(t(data[key]["people_count"][1]["count"], prob_out))
                else:
                    count[barrier][tag][data[key]["people_count"][0]["id"]].append(data[key]["people_count"][0]["count"])
                    count[barrier][tag][data[key]["people_count"][1]["id"]].append(data[key]["people_count"][1]["count"])

        for loc in count[barrier][tag]:
            cumulate[barrier][tag][loc] = np.cumsum(count[barrier][tag][loc])
            if cumulate[barrier][tag][loc][-1] > 0:
                scaling_factor[barrier][tag][loc]=gt[tag][loc]/cumulate[barrier][tag][loc][-1]
            else:
                scaling_factor[barrier][tag][loc]=1
for barrier in barriers:
    accumulated_error=0
    accumulated_counted=0
    accumulated_gt=0
    print()
    print(barrier, get_sf(scaling_factor[barrier])[id_in], get_sf(scaling_factor[barrier])[id_out])
    #print("%20s : %-13s %5s %5s %5s %4s%%" % ('time','location', 'track', 'fit', 'gt', 'err'))
    print("%20s : %-13s %5s %5s %4s%%" % ('time','location', 'track', 'gt', 'err'))
    for tag in tags:
        fitted[id_in]=cumulate[barrier][tag][id_in][-1]*get_sf(scaling_factor[barrier])[id_in]
        fitted[id_out]=cumulate[barrier][tag][id_out][-1]*get_sf(scaling_factor[barrier])[id_out]
        #print("%20s : %-13s %5d %5d %5d %4d%%" % (tag, id_in, cumulate[barrier][tag][id_in][-1], fitted[id_in], gt[tag][id_in], int((fitted[id_in] - gt[tag][id_in])/gt[tag][id_in]*100)))
        print("%20s : %-13s %5d %5d %4d%%" % (tag, id_in, cumulate[barrier][tag][id_in][-1], gt[tag][id_in], int((fitted[id_in] - gt[tag][id_in])/gt[tag][id_in]*100)))
        accumulated_error += abs(int((fitted[id_in] - gt[tag][id_in])/gt[tag][id_in]*100))
        accumulated_counted += int(fitted[id_in])
        accumulated_gt += gt[tag][id_in]
        #print("%20s : %-13s %5d %5d %5d %4d%%" % (tag, id_out, cumulate[barrier][tag][id_out][-1], fitted[id_out], gt[tag][id_out], int((fitted[id_out] - gt[tag][id_out])/gt[tag][id_out]*100)))
        print("%20s : %-13s %5d %5d %4d%%" % (tag, id_out, cumulate[barrier][tag][id_out][-1], gt[tag][id_out], int((fitted[id_out] - gt[tag][id_out])/gt[tag][id_out]*100)))
        accumulated_error += abs(int((fitted[id_in] - gt[tag][id_in])/gt[tag][id_in]*100))
        accumulated_counted += int(fitted[id_out])
        accumulated_gt += gt[tag][id_out]
    print("%11s %8s %11s%% %15s%%" % ('Tot counted', 'tot gt', 'tot error', 'tot acc error'))
    print("%11s %8s %11s%% %15s%%" % (accumulated_counted, accumulated_gt, int((accumulated_counted - accumulated_gt)/accumulated_gt*100), accumulated_error))


#%%
if False:
    for barrier in barriers:
        tag = tags[5]
        fig, (ax, ax1, ax2) = plt.subplots(1,3)
        subsample=1
        [ax.plot(times[barrier][tag][::subsample], count[::subsample], '-o', label=label) for label, count in count[barrier][tag].items()]
        [ax1.plot(times[barrier][tag][::subsample], cumul[::subsample], '-o', label=label) for label, cumul in cumulate[barrier][tag].items()]
        ax2.hist([count for label, count in count[barrier][tag].items()], bins=np.arange(10))
        ax.set_title("Counts vs time")
        ax.set_xticks(times[barrier][tag][0::subsample])
        ax.set_xticklabels(dates[barrier][tag][0::subsample], rotation=90, fontsize=10)
        ax.legend()
        ax1.set_title("Cumulative Counts vs time")
        ax1.set_xticks(times[barrier][tag][0::subsample])
        ax1.set_xticklabels(dates[barrier][tag][0::subsample], rotation=90, fontsize=10)
        ax1.legend()
        ax2.set_title("Counts histogram")
        ax2.set_xticks(np.arange(10))
        ax2.legend()
        plt.savefig("report.png")
        plt.show()
