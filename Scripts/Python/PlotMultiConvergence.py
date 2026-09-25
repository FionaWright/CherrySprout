import csv
import os
import sys
import numpy as np

from Utils.BokehPlot import *

os.chdir(os.path.dirname(os.path.abspath(__file__))) # Change CWD to "Python Scripts"

# ARGS:
# 1 - str: TestName (TODO)
# 2+ - str: CSV Filepaths
# --show
# --save

rmseDataLists = []
testNames = []

for testName in sys.argv[1:]:
    if "--" in testName:
        continue  # skip flags

    rmseData = []
    testNames.append(os.path.splitext(os.path.basename(testName))[0])

    with open(testName, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            rmseData.append(float(row['RMSE']))

    rmseDataLists.append(rmseData)

show_plot = sys.argv.__contains__('--show')
save_plot = sys.argv.__contains__('--save')

use_log_y = "--log" in sys.argv

plot_multiple_lines(rmseDataLists, testNames, show_plot=show_plot, save_plot=save_plot, use_log_y=use_log_y)