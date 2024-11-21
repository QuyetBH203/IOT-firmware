#ifndef MCD1_H
#define MCD1_H
#include <Arduino.h>
#include <vector>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "mcd.h"

struct Location
{
  double lat;
  double lng;
};

double calculateDistance(Location a, Location b);

DeviceStatus antiTheft(bool statusAntiTheft, Location last, Location current);
bool mpuInit();
DeviceStatus checkFallandCrash();
bool handleDeviceStatus();

void updateData();

#endif