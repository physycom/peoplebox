#! /usr/bin/env python3

import subprocess
import json
import glob
import os
import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-w", "--work-dir",   help="peoplebox folder (no trailing /)", default=".")
parser.add_argument("-f", "--video-dir",  help="video folder (no trailing /)",     default="D:/Alex/videotest")
parser.add_argument("-r", "--file-regex", help="video format regex",               default="*.avi")
args = parser.parse_args()

jet_list = ["jetson2", "jetson3", "jetson4", "jetson5"]

for jet_dir in jet_list:
    dir_regex = args.video_dir + os.sep + jet_dir + os.sep + args.file_regex
    files = sorted(glob.glob(dir_regex))
    for file in files:
        for p1 in [2, 3, 4, 5, 6, 7, 8, 9, 10, 11]: #### variable loop
            for p2 in [70, 80, 90, 100, 110, 120, 130, 140, 150, 160]:
                with open(args.work_dir + os.sep + "config" + os.sep + jet_dir + ".json") as f:
                    data = f.read()
                d = json.loads(data)
        
                ### ALL JSON CHANGES HERE
                d["filename"] = file
                d["id_box"] = jet_dir + "_" + file.strip(args.file_regex.split("*")[1]).split("frames")[1] + "_" + str(p1) + "_" + str(p2)
                d["crossing_dt"] = p1
                d["max_dist_px"] = p2
                ###
        
                with open(args.work_dir + os.sep + "config" + os.sep + jet_dir + ".json", 'w') as f:
                    f.write(json.dumps(d, indent = 4))
                print("Processing file " + file)
                subprocess.run([args.work_dir + os.sep + "bin" + os.sep + "demo.exe", args.work_dir + os.sep + "config" + os.sep + jet_dir + ".json"], stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)
