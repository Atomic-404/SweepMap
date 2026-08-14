#include <Wire.h> //i2c library
#include <hd44780.h> //LCD driver library
#include <hd44780ioClass/hd44780_I2Cexp.h> // I2C expander i/o class
#include <VL53L0X.h> //tof sensor
#include <cmath>
#include "Arduino_RouterBridge.h"


hd44780_I2Cexp lcd; // Auto-configures address and pinout
VL53L0X sensor; //intilises the sensor. I belive only one sensor is allowed over i2c without some work so its outside the struct


//define variables
//ToF related functions

//uses comparison of the byte size of one list item vs the whole list to increment it forward (item 0 -> item 1) nomatter the size of the list.
void sortList(uint16_t list[], uint16_t length){

    for (uint16_t pos = 0; pos < length; pos += 1){
        uint16_t currValue = list[pos];

        int sortPos = pos-1;

        //shift the element right if they are larger than our current value
        while (sortPos >= 0 && list[sortPos] > currValue){
            list[sortPos + 1] = list[sortPos]; // Shift element forward
            sortPos -= 1;//works from the end of the list

        }
    }
}



void incrementList_uint16(uint16_t list[], uint16_t length, uint16_t offset, uint16_t newVal){
    for (int16_t pos = length-2; pos>=offset; pos -= 1){ //increments from the end of the list to avoid needing to store the value being overwritten
        list[pos+1] = list[pos];
    }
    list[offset] = newVal;
}


struct TofSensor {

    static const uint8_t numReadings = 5; //these can be changed by the setVariable function. 7 is the max
    static const uint8_t meanPos = round(numReadings/2);

    //Stores the raw distances reported by the VLX sensor every 50mm starting from 0mm (index 1 is 50mm, index 2 is 100mm)
    static const uint16_t calibrationTable[25]; //max is 1200mm

    uint16_t readings[numReadings]; //circular buffer of the 5 most recent readings
    uint16_t sorted[numReadings]; //usedfor finding mean/range
    uint8_t numNewReadings = 0; //used to keep track of the number of new readings since the last 5 were processed

    uint16_t reading = 0; //most recent reading
    uint16_t readingCalibrated = 0; //if you can figure out this one I'll give you $5

    uint16_t mean = 0; //the mean of the 5 readings in sorted[]
    uint16_t range = 0; //the variation in readings
    uint16_t precise = 0; //the mean but calibrated, used when precision matters.

    bool newReading = false; //the updateTof function sets this to ture when there is a new reading


    //functions specific to each instance of the sensor

    void updateSensor() {
        if ((sensor.readReg(VL53L0X::RESULT_INTERRUPT_STATUS) & 0x07) != 0) {

            reading = sensor.readReg16Bit(VL53L0X::RESULT_RANGE_STATUS + 10);
            newReading = true;

            sensor.writeReg(
                VL53L0X::SYSTEM_INTERRUPT_CLEAR,
                0x01
            );
        }
    }


    bool preciseRead(){
        if (newReading == true){//only checks if there is a new reading

            incrementList_uint16(readings, numReadings, 0, reading); //increments the list and adds the new value at the begining
            numNewReadings += 1;
            newReading = false; //used the new reading, sets it to false

            if (numNewReadings >= numReadings){  //if we have taken 5 or more consecutivve readings calculate the mean
                numNewReadings = 0;

                for (uint8_t pos = 0; pos < numReadings; pos += 1){
                    sorted[pos] = readings[pos]; //copies the readings list into the sorted list
                }

                sortList(sorted, numReadings);
                mean = sorted[meanPos]; //sets the mean to the mean in the list to make it easier
                precise = calibrateReading(mean); //passes the mean through the calibration function to get a reading within 5-20mm

                return true;
            }

        }
        return false;
    }


    uint16_t calibrateReading(uint16_t reading){

        uint8_t pos = 1;

        while (pos < 24){ //increments through the list of calibration values

            if (reading <= calibrationTable[pos]){
                break; //when a value in the table is more than the current reading break the loop to save that value

            } else {

                pos += 1;
            };
        };


        uint16_t baseCorrect = (pos-1)*50; //the table is in 50mm increments. saves the lower value for later
        uint8_t overEntry = reading - calibrationTable[pos-1]; //saves the amount the reading is over the lower entry
        uint8_t toNextEntry = calibrationTable[pos]-calibrationTable[pos-1]; //saves the amount to the next entry from the lowesr

        if (toNextEntry == 0){
            return calibrationTable[pos];
        }

        uint32_t offset = ((50 * overEntry) + (toNextEntry / 2)) / toNextEntry; //calculates the offset as a % between the two entries then does that to 50

        uint16_t finalNum = static_cast<uint16_t>(offset+baseCorrect); //adds the offset to the base value for a final reading
        return finalNum;
    }


    uint8_t preciseScore(){
        uint16_t spread = sorted[numReadings-1]-sorted[0]; //takes the range of the last precice reading.
        uint8_t spreadScore = 1;
        uint8_t distanceScore = 0;

        if (spread <= 20) spreadScore = 2; //checks if the spread is less than whatever ammount then sets the score
        if (spread <= 10) spreadScore = 3;
        if (spread <= 5)  spreadScore = 4;
        if (spread <= 3)  spreadScore = 5;

        //the same as the other one but for distance.
        if (precise > 1200) return 0;
        if (precise <= 1200) distanceScore =  1; //checks if the spread is less than whatever ammount then sets the score
        if (precise <= 800)  distanceScore =  2;
        if (precise <= 600)  distanceScore =  3;
        if (precise <= 300)  distanceScore =  4;
        if (precise <= 150)  distanceScore =  5;
        if (precise <= 50)   distanceScore =  1;

        return (spreadScore*2+distanceScore)/3;

    }


};

const uint16_t TofSensor::calibrationTable[25]{
    45, //0mm
    78, //50mm
    130, //100
    186, //150mm
    242, //200mm
    293, //250mm
    348, //300mm
    400, //350mm
    458, //400mm
    502, //450mm
    555, //500mm
    605, //550mm
    655, //600mm
    703, //650mm
    755, //700mm
    805, //750mm
    856, //800mm
    907, //850mm
    962, //900mm
    1008, //950mm
    1060, //1000mm
    1110, //1050mm
    1160, //1100mm
    1210, //1150mm
    1260 //1200
};

TofSensor tof1;

//tofServo/positioning variables and functions
uint8_t tofSweepPos = 90;
uint8_t tofSweepLastPos = 90;
uint32_t lastTofMove = 0;
uint8_t tofSweepDegStart = 0;
uint8_t tofSweepDegEnd = 180;
uint8_t tofSweepStep = 5;
uint16_t tofSweepDelay = 50; //time to let servo move one tofSweepStep
//The proper function for the tofSweepDelay in miliseconds is delay = 2*(degrees moved)+20
//I didnt want to implement it so I left it here for future me.
//Think of it as a gift

void servoStartPin10(){
    // Use the arduino core to connect Pin 10 to Timer 4 as per documentatipn. We send a dummy value just to initialize the route.
    analogWrite(10, 0);

    // The UNO Q's STM32 runs at 160 MHz. We want 1 MHz (1 tick per microsecond).
    // Formula: (System Clock / Target Clock) - 1
    // (160,000,000 / 1,000,000) - 1 = 159
    TIM4->PSC = 159;

    // 2. SET THE OVERFLOW / PERIOD (ARR - Auto-Reload Register)
    // We want a 20ms period (50 Hz). 20ms = 20,000 microseconds.
    TIM4->ARR = 20000;

    // 3. SET THE COMPARE MATCH / DUTY CYCLE (CCR4 - Capture/Compare Register 4)
    // Start with a 1.5ms pulse (1575 microseconds / ~90 degrees)
    TIM4->CCR4 = 1575;

    // 4. Force the hardware timer to update immediately with our new settings
    TIM4->EGR = 1;

}

void writeServoAngle(int degrees) {
    // Constrain to safe bounds
    degrees = constrain(degrees, 0, 180);

    // Map 0-180 degrees to 1000us-2000us
    uint32_t pulse_us = map(degrees, 0, 180, 650, 2500);

    // Drop the new pulse width directly into the hardware compare register!
    TIM4->CCR4 = pulse_us;
}



//grid/mapping functions and variables

//struct for a point to make things easier to keep track of instead of two variables
struct SmallPoint {
    int8_t x = 0; //allows a max of +/- 128
    int8_t y = 0; //(use SmallPoint.y)
    uint8_t score = 0; //the confidence score of the cell. 0-31
};


SmallPoint newPoint;
uint8_t cellSize = 50; //size of one cell in mm



struct SmallPoint calcPointCell(uint16_t distance_mm, uint8_t angle_deg) {
    SmallPoint targetCell;

    // Convert angle to radians for the math functions
    float angle_rad = angle_deg * (M_PI / 180.0);

    // Calculate raw x/y coordinates in mm
    float raw_x = distance_mm * cos(angle_rad);
    float raw_y = distance_mm * sin(angle_rad);

    // Divide by 50mm to get the grid points
    targetCell.x = round(raw_x / cellSize);
    targetCell.y = round(raw_y / cellSize);


    return targetCell;
}


//input related
//define pins
const int increasePin = 13;

const int decreasePin = 12;

const int takeReadPin = 11;
int takeReadCurr;
int takeReadLast;
uint32_t takeReadLastTime;

uint16_t debounceTime = 50;


struct Button {
    uint8_t pin;
    bool currState;
    bool lastState;
    unsigned long lastTime;

    //an inbuilt function to set the pin of the button. After defining a button (eg. Button button1;) use button1.start(pinTheButtonIsOn)
    void start(uint8_t p) {
        pin = p;
        pinMode(pin, INPUT_PULLUP);
        currState = HIGH;
        lastState = HIGH;
        lastTime = 0;
    }

    //updates the button state with debouncing. Call once at the begining using button1.update() and then button1.state has the
    void update(){
        bool currRead = digitalRead(pin);
        if (currRead != lastState){
            if (lastTime < millis()-debounceTime){
                currState = currRead;
                lastTime = millis();
            }
        }
        lastState = currState;
    }
};


//bridge related functions

void sendPoint(uint16_t distance, uint8_t angle, int8_t x, int8_t y, int8_t score){
    Bridge.notify("on_newPoint", distance, angle, x, y, score);
}

void setVariables(int sweepStartDeg, int sweepEndDeg, int sweepStepDeg) {

    // Sweep data
    tofSweepDegStart = sweepStartDeg;
    tofSweepDegEnd = sweepEndDeg;
    tofSweepStep = sweepStepDeg;
}

void setup() {

    Monitor.begin(9600);
    Wire.begin();
    Bridge.begin();
    Bridge.provide_safe("setVariables", setVariables); //must use this to provide a function for python to use

    delay(500);

    if (!sensor.init()) {
        Monitor.println("Failed to detect VL53L0X!");
        while (1);
    }

    sensor.setMeasurementTimingBudget(50000);
    sensor.startContinuous();

    servoStartPin10();

    lcd.begin(16, 2);
    lcd.print("ToF calibration");

    delay(500);
    lcd.clear();
}



void loop() {

    tof1.updateSensor();
    if (tof1.preciseRead() == true){ //takes 5 readings, gets the mean, then returns the mean if it has a new reading.

        newPoint = calcPointCell(tof1.precise, tofSweepPos);
        newPoint.score = tof1.preciseScore();

        lcd.clear();
        lcd.print("Mean: ");
        lcd.print(tof1.precise);
        lcd.setCursor(0,1);
        lcd.print("(");
        lcd.print(newPoint.x);
        lcd.print(", ");
        lcd.print(newPoint.y);
        lcd.print("), ");
        lcd.print(newPoint.score);

        Monitor.print("(");
        Monitor.print(newPoint.x);
        Monitor.print(", ");
        Monitor.print(newPoint.y);
        Monitor.print("), ");
        Monitor.println(newPoint.score);

        sendPoint(tof1.precise, tofSweepPos, newPoint.x, newPoint.y, newPoint.score);

        if(tofSweepPos >= tofSweepDegEnd){

            Monitor.println("Point, Confidence ");
            tofSweepPos = tofSweepDegStart;
        } else {
            tofSweepPos += tofSweepStep;
        }

    }

    if(tofSweepLastPos != tofSweepPos){
        //only update the servo position if it actually changed
        writeServoAngle(tofSweepPos);
        tofSweepLastPos = tofSweepPos;
    }

}
