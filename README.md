# SweepMap
Using a distance sensor and stepper/servo motor to get a 2d map of an area.

# Hardware:
I am using a "VL53LXX-V2" Time of Flight (ToF) sensor mounted on a generic 9g servo with an I2C 16x2 LCD to show live data. This is all running on the Arduino Uno Q 2gb and may or may not work with other boards.

# Process on Microcontroller:
1. 5 ToF readings and save them to a list.
2. Order the list from lowest to highest then extract the mean.
3. Using a linear interpolation calibrate the mean against the table. (One entry every 50mm, can be changed if needed)
4. make a confidence value based on the distance from the sensor and spread of the 5 readings
5. convert distance+angle to (x,y) coordinates and send those and the confidence over the bridge to the Python script
6. Increment angle then back to 1

# Process on Python:
1. Get incoming point data and convert it to custom byte structure (FTTSSSSS. 1 Flag bit, 2 Type bits, 5 Score bits)
2. Save the point to a list for all the points in the area and check if the scan finished.
3. If the scan finished launch mapVisualiser.py to make a grid using matplotlib, else back to 1.
