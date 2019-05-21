#! /usr/bin/env python3

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
parser.add_argument("-f", "--data-dir",     help="data folder (no trailing /)",     default="D:/Alex/data")
parser.add_argument("-t", "--ground_truth", help="ground truth file location",      default="D:/Alex/videotest/ground_truth.txt")
parser.add_argument("-r", "--file-regex",   help="json format regex",               default="*.json")
args = parser.parse_args()


jet_list = ["jetson2", "jetson3"]

gt = pd.read_csv(args.ground_truth, sep = ";", engine = "python")

filenames = []
counting_in = []
counting_out = []
par1 = []
par2 = []
files = []

for jet in jet_list:
    files += sorted(glob.glob(args.data_dir + os.sep + jet + args.file_regex))

gt = gt[gt["VIDEO"].astype(str).str.startswith(tuple(jet_list))]

for file in files:
    sepfile = file.split("jetson")[1].split("_")
    filename = "jetson" + sepfile[0] + "_frames" + sepfile[1]
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
    par1.append((int)(sepfile[2]))
#    par2.append((int)(sepfile[3]))

cv = pd.DataFrame({
                  "VIDEO": filenames,
                  "IN": counting_in,
                  "OUT": counting_out,
                  "PAR1": par1,
                  "PAR2": par2,
                  })
    
cv = cv.set_index("VIDEO")
gt = gt.set_index("VIDEO")
diff = cv.copy()
diff["IN"]  = np.abs(diff["IN"] - gt["IN"])
diff["OUT"] = np.abs(diff["OUT"] - gt["OUT"])
tot_nrmse = pd.DataFrame()
tot_nmae = pd.DataFrame()

for p1 in np.unique(par1):
    for p2 in np.unique(par2):
        temp = diff.loc[diff["PAR1"] == p1].drop("PAR1", axis=1)
        nrmse = np.sqrt((temp*temp).mean()) / np.sqrt((gt**2).mean())   
        nmae = (temp.abs()).mean() / gt.mean()
        tot_nrmse[p1] = nrmse
        tot_nmae[p1] = nmae

plt.figure(figsize=(20,5))
ax1 = plt.subplot(1, 2, 1)
sns.heatmap(tot_nmae, annot=True, cmap="RdYlBu_r", ax=ax1)
ax1.set_title("NMAE")
ax1.set_xlabel("PAR1")
ax1.set_ylabel("PAR2")
ax2 = plt.subplot(1, 2, 2)
sns.heatmap(tot_nrmse, annot=True, cmap="RdYlBu_r", ax=ax2)
ax2.set_title("NRMSE")
ax2.set_xlabel("PAR1")
ax2.set_ylabel("PAR2")
plt.savefig(args.data_dir + os.sep + "parametric_scan.png")
#plt.clf()