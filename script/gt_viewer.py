#! /usr/bin/env python3
# USAGE: Use scan executable on all videos you need. Then use this to plot the results.

import os
import glob
import json
import matplotlib.pyplot as plt
from collections import OrderedDict
import pandas as pd
import numpy as np
import seaborn as sns
sns.set()
sns.set_color_codes()

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-f", "--data-dir",     help="data folder (no trailing /)",     required = True)
parser.add_argument("-t", "--ground_truth", help="ground truth file location",      required = True)
parser.add_argument("-r", "--file-regex",   help="json format regex",               default="*.json")
args = parser.parse_args()

# select only folders you processed
jet_list = ["jetson2", "jetson3", "jetson4", "jetson5"]

filenames = []
counting_in = []
counting_out = []
par1 = []
par2 = []
files = []

for jet in jet_list:
    files += sorted(glob.glob(args.data_dir + os.sep + jet + args.file_regex))
gt = pd.read_csv(args.ground_truth, sep = ";", engine = "python")
gt = gt[gt["VIDEO"].astype(str).str.startswith(tuple(jet_list))]

for file in files:
    sepfile = file.split("jetson")[1].split("_")
    filename = "jetson" + sepfile[0] + "_" + sepfile[1].split(".")[0]
    with open(file) as f:
        data = json.load(f, object_pairs_hook = OrderedDict)
    count_in  = 0
    count_out = 0
    for frame in data:
        count_in  += (data[frame]["people_count"][0]["count"])
        count_out += (data[frame]["people_count"][1]["count"])
    filenames.append(filename)
    counting_in.append(count_in)
    counting_out.append(count_out)

cv = pd.DataFrame({
                  "VIDEO": filenames,
                  "IN": counting_in,
                  "OUT": counting_out
                  })

#%% error dist plot
fig = plt.figure(figsize=(10,10))
fig.suptitle("Normalized errors distributions",  fontweight='bold')
diff = cv.set_index("VIDEO") - gt.set_index("VIDEO")
diff_abs = diff.abs()
rmse = np.sqrt((diff*diff).mean())
nrmse = rmse / np.sqrt((gt.drop("VIDEO", axis = 1) **2).mean())
mae = diff_abs.mean()
nmae = mae / gt.mean()

ax1 = plt.subplot(2, 2, 1)
sns.distplot(diff["IN"], label = "IN")
ax1.legend()
ax1.set_xlabel("NRMSE: " + str(round(nrmse["IN"],3)) + ", NMAE: " + str(round(nmae["IN"],3)))
ax2 = plt.subplot(2, 2, 2, sharey = ax1)
sns.distplot(diff["OUT"], label = "OUT", color="r")
plt.setp(ax2.get_yticklabels(), visible=False)
ax2.legend()
ax2.set_xlabel("NRMSE: " + str(round(nrmse["OUT"],3)) + ", NMAE: " + str(round(nmae["OUT"],3)))
ax3 = plt.subplot(2, 2, 3)
sns.distplot(diff_abs["IN"], label = "IN - ABS", kde=False)
ax3.legend()
ax3.set_xlabel("")
ax4 = plt.subplot(2, 2, 4, sharey = ax3)
sns.distplot(diff_abs["OUT"], label = "OUT - ABS", kde=False, color = "r")
plt.setp(ax4.get_yticklabels(), visible=False)
ax4.legend()
ax4.set_xlabel("")
plt.savefig(args.data_dir + os.sep + "error_dist.png")

#%% crescent gt diff plot

def short_name(name):
    sn = name.split("_")
    return "j" + sn[0][-1] + "_f" + sn[1][-2:]
    
plt.figure(figsize=(20,10))
gt.columns = ["VIDEO", "IN_GT", "OUT_GT"]
gt["VIDEO"] = gt["VIDEO"].map(short_name)
cv["VIDEO"] = cv["VIDEO"].map(short_name)
dataset = pd.concat((gt, cv.drop("VIDEO", axis=1)), axis=1)
dataset = dataset.sort_values("OUT_GT")
ax1 = plt.subplot(2, 1, 1)
ax1.set_title("Crescent comparison plot",  fontweight='bold')
sns.lineplot(x = "VIDEO", y = "OUT_GT", data=dataset, label= "ground truth", sort= False)
sns.lineplot(x = "VIDEO", y = "OUT", data=dataset, label = "detected", sort= False)
ax1.set_xlabel("")
ax1.legend()
plt.xticks(rotation=90)
dataset = dataset.sort_values("IN_GT")
ax2 = plt.subplot(2, 1, 2)
sns.lineplot(x = "VIDEO", y = "IN_GT", data=dataset, label= "ground truth", sort= False)
sns.lineplot(x = "VIDEO", y = "IN", data=dataset, label = "detected", sort= False)
ax2.set_xlabel("")
ax2.legend()
plt.xticks(rotation=90)
plt.savefig(args.data_dir + os.sep + "comparison_diff_crescent.png")

#%% gt diff plot
plt.figure(figsize=(20,10))
ax1 = plt.subplot(2, 1, 1)
ax1.set_title("Comparison plot",  fontweight='bold')
sns.lineplot(x = "VIDEO", y = "OUT_GT", data=dataset, label= "ground truth")
sns.lineplot(x = "VIDEO", y = "OUT", data=dataset, label = "detected")
ax1.set_xlabel("")
ax1.legend()
plt.xticks(rotation=90)
ax2 = plt.subplot(2, 1, 2, sharex = ax1)
sns.lineplot(x = "VIDEO", y = "IN_GT", data=dataset, label= "ground truth")
sns.lineplot(x = "VIDEO", y = "IN", data=dataset, label = "detected")
ax2.legend()
plt.xticks(rotation=90)
plt.savefig(args.data_dir + os.sep + "comparison_diff.png")