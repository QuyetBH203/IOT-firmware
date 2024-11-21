
#include "mcd1.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>

Adafruit_MPU6050 mpu;
float magnitude;
sensors_event_t a, oldAccel, delta, g, temp;

double calculateDistance(Location a, Location b)
{
	int R = 6371;
	double dLat = (b.lat - a.lat) * M_PI / 180.0;
	double dLng = (b.lng - a.lng) * M_PI / 180.0;

	double temp;
	temp = sin(dLat / 2) * sin(dLat / 2) +
		   cos(a.lat * M_PI / 180) *
			   cos(b.lat * M_PI / 180) *
			   sin(dLng / 2) * sin(dLng / 2);
	double calculateAngle = 2 * atan2(sqrt(temp), sqrt(1 - temp));
	return calculateAngle * R * 1000;
}

DeviceStatus antiTheft(bool statusAntiTheft, Location last, Location current)
{
	if (statusAntiTheft)
	{
		double Distance;
		Distance = calculateDistance(last, current);
		if (Distance > 50)
		{
			return LOST2;
		}
		if (Distance > 10)
		{
			return LOST1;
		}
	}
	return NONE;
}

bool mpuInit()
{
	if (!mpu.begin())
	{
		Serial.println("Failed to find MPU6050 chip");
		while (1)
		{
			delay(10);
		}
	}
	Serial.println("MPU6050 Found!");
	return true;
	mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
	mpu.setMotionDetectionThreshold(1);
	mpu.setMotionDetectionDuration(20);
	mpu.setInterruptPinLatch(true); // Keep it latched.  Will turn off when reinitialized.
	mpu.setInterruptPinPolarity(true);
	mpu.setMotionInterrupt(true);
	Serial.println("");
	delay(100);
	mpu.getEvent(&oldAccel, &g, &temp);
}

DeviceStatus checkFallandCrash()
{
	//if (mpu.getMotionInterruptStatus())
	{
		//Serial.println("Hi");
		mpu.getEvent(&a, &g, &temp);
		delta.acceleration.x = a.acceleration.x - oldAccel.acceleration.x;
		delta.acceleration.y = a.acceleration.y - oldAccel.acceleration.y;
		delta.acceleration.z = a.acceleration.z - oldAccel.acceleration.z;
		// Serial.println(a.acceleration.z);
		/* Print out the values */
		magnitude = sqrt(sq(delta.acceleration.x) + sq(delta.acceleration.y) + sq(delta.acceleration.z));
		oldAccel.acceleration.x = a.acceleration.x;
		oldAccel.acceleration.y = a.acceleration.y;
		oldAccel.acceleration.z = a.acceleration.z;
		if (magnitude > 40.0)
		{
			Serial.println("CRASH");
			return CRASH;
		}
		if (a.acceleration.z < 4)
		{
			//Serial.println("FALL");
			return FALL;
		}
	}
	return NONE;
}


