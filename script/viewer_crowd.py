import matplotlib.pyplot as plt
import json
import datetime
from collections import OrderedDict

crowdfile='crowd.json'
with open(crowdfile) as f:
	data = json.load(f, object_pairs_hook=OrderedDict)

times = []
dates = []
count = []
print(data.keys())
for key in data:
	print(key)
	times.append(data[key]["timestamp"])
	dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%H:%M:%S'))
#	dates.append(datetime.datetime.fromtimestamp(int(data[key]["timestamp"])).strftime('%Y-%m-%d %H:%M:%S'))
	count.append(data[key]["people_count"][0]["count"])
exit(1)

fig, ax = plt.subplots(1,1)
ax.plot(times, '-o')
#ax.plot(times, count, '-o')
subsample=5
ax.set_xticks(times[0::subsample])
#ax.set_xticklabels(dates[0::subsample], rotation=45, fontsize=10)
#plt.show()

f=[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
print(f[0::subsample])

print(times)
