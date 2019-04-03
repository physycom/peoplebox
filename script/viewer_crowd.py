import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict
import glob

times = []
dates = []
flux = {}

files = sorted(glob.glob('GIULIO*'))
for file in files:
  print(file)

  with open(file) as f:
    data = json.load(f, object_pairs_hook=OrderedDict)

  for key in data:
    times.append(data[key]["timestamp"])
    dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
    for cnt in data[key]["people_count"]:
      flux.setdefault(cnt["id"], []).append(cnt["count"])
      
fig, ax = plt.subplots(1,1)
[ax.plot(times, count, '-o', label=label) for label, count in flux.items() ] #if label.startswith("r_")]
subsample=1
ax.set_xticks(times[0::subsample])
ax.set_xticklabels(dates[0::subsample], rotation=45, fontsize=10)
plt.legend()
plt.tight_layout()
plt.savefig("report.png")
plt.show()
