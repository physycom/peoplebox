#!/usr/bin/env bash

export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH
./uselib_track darknet/data/coco.names darknet/cfg/yolov3.cfg darknet/yolov3.weights test.mp4 0.20
