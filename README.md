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

# Scans:
In the scans Blue represents an object and green represents a place where the area is clear. Darker (Closer to black) means lower confidence and brighter (More saturated) means a higher confidence

# Scan 1
Data:
<img width="500" height="auto" alt="sweep1" src="https://github.com/user-attachments/assets/97d9da84-84ec-48e3-a121-04192f487dda" />

Environment:
<img width="500" height="auto" alt="sweep1Image" src="https://github.com/user-attachments/assets/d33bd8da-1bb0-48bb-b805-151f8959cb40" />

# Scan 2:
Data:
<img width="500" height="auto" alt="sweep2" src="https://github.com/user-attachments/assets/51d24947-7b17-470e-b905-055a0a573397" />

Environment:
<img width="500" height="auto" alt="sweep2Image" src="https://github.com/user-attachments/assets/c658a076-73c1-437a-a405-287650d86e57" />


# Scan 3:
Data:
<img width="500" height="auto" alt="sweep3" src="https://github.com/user-attachments/assets/ae1fa974-555c-4272-b429-14625a295312" />

Environment:
<img width="500" height="auto" alt="sweep3Image" src="https://github.com/user-attachments/assets/067356d9-3f5e-492b-b8cf-6f7ef0b96982" />

