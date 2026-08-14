import time
import math
import subprocess

from arduino.app_utils import App
from arduino.app_utils import *

#initialize bridge to allow communication with the sketch

#variables to set on the STM side
sweepStartDeg = 0
sweepEndDeg = 180
sweepStepDeg = 3
cellSize_mm = 50

#grid variables
CONFIDENCE_MASK = 0b00011111  # max value 31. 5 bits
TYPE_MASK       = 0b00000011  # max value 3. 2 bits
FLAG_MASK       = 0b00000001  # max valye 1. 1 bit

UNKNOWN = 0
WALL = 1
EMPTY = 2
NOGO = 3

GRID_WIDTH = 51 #this grid is for my specific setup.
GRID_HEIGHT = 26 #I have a max range of 1200mm and sweep of 180 degrees with 50mm cells

lastAngle = 0
sweepPoints = bytearray(GRID_WIDTH*GRID_HEIGHT) #makes every entry one byte since we are using a struct
newSweep = sweepPoints.copy()

processSweep = False

newPoint = 0b00000000 #sets the variable to be an empty byte

def packPoint(confidence, call_type, flag):
    #uses bitwise AND (&) to constrain a the value to the specific place in the bit for more efficient storage
    return (confidence & CONFIDENCE_MASK) | ((call_type & TYPE_MASK) << 5) | ((flag & FLAG_MASK) << 7)

def unpackPoint(value):
    confidence = value & CONFIDENCE_MASK
    type = (value >> 5) & TYPE_MASK
    flag = (value >> 7) & FLAG_MASK

    return confidence, type, flag

def indexFromPoint(x, y):
    return (y * GRID_WIDTH) + (x + GRID_WIDTH // 2)


def on_newPoint(distance_mm, angle_deg, x_grid, y_grid, score):
    global lastAngle, newSweep, processSweep, sweepPoints

    newPoint = packPoint(score, WALL, 0) #the function to save the coordinate as one byte. Uses ~65% less space vs as a list

    # Trigger a new sweep processing if the angle resets
    if (lastAngle >= angle_deg):
        newSweep = sweepPoints.copy()

        processSweep = True

        sweepPoints[:] = [0] * len(sweepPoints)

    # x_grid must be between -25 and +25. y_grid must be between 0 and 25.
    if (-GRID_WIDTH // 2 <= x_grid <= GRID_WIDTH // 2) and (0 <= y_grid < GRID_HEIGHT):
        idx = indexFromPoint(x_grid, y_grid)
        sweepPoints[idx] = newPoint
    lastAngle = angle_deg

print("SoC is ready. Sending configuration variables")
Bridge.call("setVariables", sweepStartDeg, sweepEndDeg, sweepStepDeg)

Bridge.provide("on_newPoint", on_newPoint)

def loop():
    global processSweep

    if processSweep:
        processSweep = False
        print("New sweep ready at sweep_data.bin")

        with open("sweep_data.bin", "wb") as f:
            f.write(newSweep)

        subprocess.Popen(["python3", "/app/python/mapVisualizer.py"])


    time.sleep(0.1)



# See: https://docs.arduino.cc/software/app-lab/tutorials/getting-started/#app-run
App.run(user_loop=loop)
