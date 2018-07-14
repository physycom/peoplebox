#!/usr/bin/env python3

import json
import numpy as np
import cv2
import matplotlib.pylab as plt
import os
import matplotlib.animation as animation # animation plot

coords_file = "../frames.json"
data = json.load(open(coords_file))
coords = [coord for name, coord in data.items()]

top_boundary = 400
bottom_boundary = 800
toll = 100
start = 0
stop = 50

coords = [list(filter(lambda x: x[1] > top_boundary - toll and
                                x[1] < bottom_boundary + toll,
                                C))
          for C in coords]

image_folder = "../frames"
images = [os.path.join(image_folder, img) for img in os.listdir(image_folder)
            if img.endswith(".jpg")][start:stop]
#fig, ax = plt.subplots(1, 1, figsize=(14, 9))
#plt.subplots_adjust(left=0.01, right=0.99, top=0.95,  bottom=0.05)

C = coords[start]
pt_track = []
mark = []
up = 0
down = 0
ims = [None] * len(images)
for cnt, im in enumerate(images):
    frame = cv2.imread(im)
    for x1, y1 in coords[cnt + start]:
        print(cnt, x1, y1)
        dists = [np.sqrt( (x1-x2)**2 + (y1-y2)**2 ) for x2, y2 in C]
        dists = [d if d < 100. else 1e4 for d in dists]
        if dists != []:
            mins = np.argmin(dists)
#            print(cnt, dists[mins], mins)
            if dists[mins] == 1e4: continue
#            pt_track.append(ax.plot([C[mins][0], x1], [C[mins][1], y1])[0])
            if y1 < top_boundary and C[mins][1] > top_boundary:
#                mark.append(ax.scatter( (C[mins][0] + x1)*.5, top_boundary, marker="*", color="y"))
                up += 1
            if y1 > bottom_boundary and C[mins][1] < bottom_boundary:
#                mark.append(ax.scatter( (C[mins][0] + x1)*.5, bottom_boundary, marker="*", color="y"))
                down += 1
    #print("cnt : ", cnt, "up :", up, "down :", down)
 #   ims[cnt] = [ax.imshow(frame, animated=True, cmap="jet"),
 #               ax.hlines(top_boundary, 0, 1919, linestyle="dashed", color="r", alpha=.5),
 #               ax.hlines(bottom_boundary, 0, 1919, linestyle="dashed", color="r", alpha=.5)
 #               ] + pt_track + mark
    C = coords[cnt + start]

#movie = animation.ArtistAnimation(fig,
#                                  ims,
#                                  interval=150,
#                                  blit=True,
#                                  repeat_delay=0
#                                  )
print("UP = ", up, ", true UP = ", 2)
print("DOWN = ", down, ", true DOWN = ", 7)

