import matplotlib.pylab as plt
import pandas as pd
import numpy as np
from scipy import stats

data = pd.read_csv("e:/Alessandro/Peoplebox/crowd/crowd_truth.csv", sep=";", header=0)

x = np.asarray(data.COUNTER)
y = np.asarray(data.GROUNDTRUTH)

slope, intercept, r_value, p_value, std_err = stats.linregress(x, y)

fig, (ax1, ax2) = plt.subplots(nrows = 1, ncols = 2, figsize=(8,8))
ax1.scatter(x, y, color="b", marker="o", label="data")
ax1.plot(x, x*slope + intercept, "r--", alpha=.5, label="regress")
ax1.legend(loc="best", fontsize=14)

ax2.plot(x, "o-", color="b", label="counter")
ax2.plot(x*slope + intercept, "o-", color="r", label="fit")
ax2.plot(y, "o-", color="g", label="gt")
ax2.legend(loc="best", fontsize=14)

print("set(C0    %5f)" % intercept)
print("set(C1    %5f)" % slope)

plt.show()