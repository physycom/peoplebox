#!/usr/bin/env python

import cv2
import sys

if len(sys.argv) != 2:
	print("Usage : %s path/to/video")
	exit(1)

#cv2.namedWindow("video", cv2.WINDOW_NORMAL)
vidcap = cv2.VideoCapture(sys.argv[1])
success = True
while success:
	success, img = vidcap.read()
	cv2.imshow('video',img)
	cv2.waitKey(1)
	
vidcap.release()
cv2.destroyAllWindows()
