import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict
import glob

times = []
dates = []
flux = {}

files = sorted(glob.glob('ORESI*'))
for file in files:
  print(file)

  with open(file) as f:
    data = json.load(f, object_pairs_hook=OrderedDict)

  for key in data:
    if not flux:
      flux[data[key]["people_count"][0]["id"]] = []
      flux[data[key]["people_count"][1]["id"]] = []
    times.append(data[key]["timestamp"])
    dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
    flux[data[key]["people_count"][0]["id"]].append(data[key]["people_count"][0]["count"])
    flux[data[key]["people_count"][1]["id"]].append(data[key]["people_count"][1]["count"])

fig, ax = plt.subplots(1,1)
[ax.plot(times, count, '-o', label=label) for label, count in flux.items()]
subsample=5
ax.set_xticks(times[0::subsample])
ax.set_xticklabels(dates[0::subsample], rotation=90, fontsize=10)
plt.legend()
plt.savefig("report.png")
plt.show()
