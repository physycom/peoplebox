#!/usr/bin/env python3

import cv2
import sys

if len(sys.argv) != 3:
	print("Usage : %s path/to/video path/to/folder")
	exit(1)

vidcap = cv2.VideoCapture(sys.argv[1])
success,image = vidcap.read()
count = 0
success = True
while success:
  success,image = vidcap.read()
  cv2.imwrite(sys.argv[2] + "/" + sys.argv[1].split("/")[-1].split(".")[0] + "_%06d.jpg" % count, image)
  count += 1
