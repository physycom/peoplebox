# -*- coding: utf-8 -*-
"""
Created on Tue May 22 16:49:56 2018

@author: nico
"""

import matplotlib.pylab as plt
import pandas as pd
import numpy as np
import argparse
import sys
from scipy import stats

def fit(x, y, order, view=False, check=False):
    if order == 1:
        slope, intercept, r_value, p_value, std_err = stats.linregress(x, y)
        z = x*slope + intercept
        fit_coeff = [slope, intercept]
    else:
        fit_coeff = np.poly1d(np.polyfit(x, y, order))
        z = fit_coeff(x)
    cmake_vars = fit_coeff[::-1]
    for i, c in enumerate(cmake_vars):
        print("set(C%d                   %.6ff)"%(i, c) )
    if check:
        rssd = np.sqrt( sum( (z-y)**2 ))
        print("Root sum square difference (fit - gt) = ", rssd )
        print("Root sum square difference normalized = ", rssd / np.sqrt(sum(y**2)) )
    if view:
        fig, (ax1, ax2) = plt.subplots(nrows=1, ncols=2, figsize=(14, 8))
        ax1.scatter(x, y, marker='o', color='b', s=4, alpha=.5, label="data")
        if order == 1: ax1.plot(x, x*slope + intercept, "r--", alpha=.5, label="regress")
        else: ax1.plot(x, z, "r--", alpha=.5, label="regress")
        ax1.set_xlabel("darktrack", fontsize=14)
        ax1.set_ylabel("ground truth", fontsize=14)
        ax1.legend(loc="best", fontsize=12)
        
        step = np.arange(0, len(x))
        ax2.plot(step, x, 'bo-', label="darktrack", alpha=.5)
        ax2.plot(step, y, 'ro-', label="ground truth", alpha=.5)
        ax2.plot(step, z, 'go-', label="fit order %d"%order, alpha=.5)
        ax2.legend(loc="best", fontsize=12)
        plt.show()
    
    
    

def find_best_order(x, y, view=False):
    N = len(x)
    errs = []
    for n in range(2, N):
        z = np.poly1d(np.polyfit(x, y, n))
        rssd = np.sqrt( sum( (z-y)**2 ) / sum(y**2) )
        errs.append(rssd)
    if view:
        fig, ax = plt.subplots(nrwos=1, ncols=1, figsize=(8,8))
        ax.plot(errs, 'bo-', label="Root sum square difference (fit - gt) normalized")
        ax.legend(loc="best", fontsize=14)
        plt.show()
    return 

def fit_regress(x, y, view=False):
    slope, intercept, r_value, p_value, std_err = stats.linregress(x, y)
    if view:
        fig, ax = plt.subplots(nrows = 1, ncols = 1, figsize=(8,8))
        ax.scatter(x, y, color="b", marker="o", label="data")
        ax.plot(x, x*slope + intercept, "r--", alpha=.5, label="regress")
        ax.legend(loc="best", fontsize=14)
        plt.show()
    return slope, intercept, r_value, p_value, std_err
    
    

if __name__ == "__main__":
    description = "Fit Tracking Darknet"
    parser = argparse.ArgumentParser(description = description)
    parser.add_argument("-j", required=True, dest="jetson", action="store", help="Jetson IN/OUT filename")
    parser.add_argument("-g", required=False, dest="ground", action="store", help="Ground-Truth filename (default : ground_truth.txt)", default="ground_truth.txt")
    parser.add_argument("-s", required=False, dest="scale", action="store", help="scale factor (default 15/50 aka 0.3)", default="0.3")
    
    if len(sys.argv) <= 1:
        parser.print_help()
        sys.exit(1)
    else:
        args = parser.parse_args()
        
    ground = args.ground
    jetson_file = args.jetson
    scale = float(args.scale)
    
    gt = pd.read_csv(ground, sep=";", header=0)
    gt['jet'], gt['frames'] = gt['VIDEO'].str.split('_', 1).str
    ground_truth = { jet : [list(data.IN), list(data.OUT)] for jet, data in gt.groupby(gt.jet)}
    
    inout = pd.read_csv(jetson_file, sep=" ", header=None)
    inout_track = [list(inout[0]), list(inout[1])]
    jetson = jetson_file.split("_")[0]
    
    x = np.asarray(sum([inout_track[0], inout_track[1]], []))
    y = np.asarray(sum([ground_truth[jetson][0], ground_truth[jetson][1]], []))
    
    x = x * scale
    y = y * scale
    
    #order = 3
    fit(x, y, order=1, view=True, check=True)
    #fit_regress(x, y, True)
    

