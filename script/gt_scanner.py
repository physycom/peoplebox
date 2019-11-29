#! /usr/bin/env python3
# USAGE: Use this to launch scan executable multiple times varying some parameters in the JSON. You need a starting JSON for each folder.

import subprocess
import json
import glob
import os
import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-w", "--work-dir",    help="peoplebox folder (no trailing /)", required=True)
parser.add_argument("-f", "--video-dir",   help="video folder (no trailing /)",     required=True)
parser.add_argument("-d", "--data-outdir", help="JSON output dir",                  required=True)
parser.add_argument("-r", "--file-regex",  help="video format regex",               default="*.avi")
args = parser.parse_args()

# select folders you want to process and parameters range for the scan
jet_list = ["jetson2", "jetson3", "jetson4", "jetson5"]
par1 = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
par2 = [70, 80, 90, 100, 110, 120, 130, 140, 150, 160]

for jet_dir in jet_list:
  dir_regex = args.video_dir + os.sep + jet_dir + os.sep + args.file_regex
  files = sorted(glob.glob(dir_regex))
  for p1 in par1: #### variable loop on PAR1
    for p2 in par2: #### variable loop on PAR2
      with open(args.work_dir + os.sep + "config" + os.sep + jet_dir + ".json") as f:
        data = f.read()
      d = json.loads(data)

      ### ALL JSON CHANGES HERE
      d["inputs"] = files
      d["id_box"] = "_" + str(p1) + "_" + str(p2)
      d["crossing_dt"] = p1
      d["max_dist_px"] = p2
      d["data_outdir"] = args.data_outdir
      ###

      with open(args.work_dir + os.sep + "config" + os.sep + jet_dir + ".json", 'w') as f:
        f.write(json.dumps(d, indent = 4))
      print("Processing dir " + jet_dir)
      subprocess.run([args.work_dir + os.sep + "bin" + os.sep + "scan.exe", args.work_dir + os.sep + "config" + os.sep + jet_dir + ".json"], stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)