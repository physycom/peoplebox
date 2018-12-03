#!/usr/bin/env pwsh

.\darknet.exe detector demo darknet\cfg\coco.data darknet\cfg\yolov3.cfg darknet\yolov3.weights .\test.mp4 -i 0
