#! /usr/bin/env python3

import glob
import json
import time
import datetime
import matplotlib.pyplot as plt
import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('-c', '--filter-counter', help='regex on COUNTER legend names to select viewed ones', default='')
parser.add_argument('-f', '--filter-flow', help='regex on FLOW legend names to select viewed ones', default='')
parser.add_argument('-d', '--wdirs', nargs='+', help='list of directory containing cam raw json', required=True)
args = parser.parse_args()

# Import json from cameras
wdirs=args.wdirs
counter = {}
flow = {}
for wdir in wdirs:
  files = sorted(glob.glob(wdir + '/*.json'))
  for file in files:
    with open(file) as f:
      data = json.load(f)
      ts = data['timestamp'] // 1000
      if data['detection'] == 'people_counter':
        for measure in data['measurement']:
          counter.setdefault(measure['id'], {}).setdefault('times', []).append(ts)
          counter.setdefault(measure['id'], {}).setdefault('values', []).append(measure['value'])
      elif data['detection'] == 'flow_counter':
        #flow.setdefault(data['id_box'], {}).setdefault('counter', []).append(data['measurement']['people-count'])
        for measure in data['measurement']['flows']:
          flow.setdefault(measure['id'], {}).setdefault('times', []).append(ts)
          flow.setdefault(measure['id'], {}).setdefault('values', []).append(measure['value'])
          flow.setdefault(measure['id'], {}).setdefault('speed', []).append(measure['speed'])

times_u = {
  'counter' : set(),
  'flow' : set()
}
dates_u = {}

for v in counter.values():
  times_u['counter'].update(v['times'])
dates_u['counter'] = [ datetime.datetime.fromtimestamp(ts).strftime("%d/%m/%Y %H:%M:%S") for ts in times_u['counter'] ]
for v in flow.values():
  times_u['flow'].update(v['times'])
dates_u['flow'] = [ datetime.datetime.fromtimestamp(ts).strftime("%d/%m/%Y %H:%M:%S") for ts in times_u['flow'] ]

#print(flow)

refil_counter = '.*' if args.filter_counter == '' else args.filter_counter
refil_flow = '.*' if args.filter_flow == '' else args.filter_flow

fig, axs = plt.subplots(1,2)

ax = axs[0]
[ ax.plot(v['times'], v['values'], label=k,  linestyle='-', marker='o') for k,v in counter.items() if re.search(refil_counter, k) ]
ax.set_xlabel('Time (CET)')
ax.set_ylabel('Counter')
ax.set_title('Camera detections "people_counter"')
ax.set_xticks(list(times_u['counter']))
ax.set_xticklabels(dates_u['counter'], rotation=45, ha='right')
ax.grid(True)
ax.legend()
#ax.gcf().subplots_adjust(bottom=0.15)

ax = axs[1]
[ ax.plot(v['times'], v['values'], label=k,  linestyle='-', marker='o') for k,v in flow.items() if re.search(refil_flow, k) ]
ax.set_xlabel('Time (CET)')
ax.set_ylabel('Counter')
ax.set_title('Camera detections "flow_counter"')
ax.set_xticks(list(times_u['flow']))
ax.set_xticklabels(dates_u['flow'], rotation=45, ha='right')
ax.grid(True)
ax.legend()

plt.gcf().subplots_adjust(bottom=0.15)
plt.show()


