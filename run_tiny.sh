#!/usr/bin/env bash

./darknet detector demo darknet/cfg/coco.data darknet/cfg/yolov3-tiny.cfg darknet/yolov3-tiny.weights test.mp4 -i 0
