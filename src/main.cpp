#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <vector>
#include <PubSubClient.h>
#include "mcd.h"
#include "mcd1.h"
#include <TinyGPSPlus.h>

#define BUTTON_SOS 4
#define BUZZER 5
#define DATA_UPDATE_CYCLE_DEFAUlT 30000
#define SYSTEM_CYCLE_DEFAULT 100000
#define TIME_CHECK_FALL 3000

const char *ssid = "QuyetBui";
const char *password = "20032003";
const char *mqttServer = "broker.emqx.io";
const char *topicPub = "/mtbcd221214/location";
const char *topicSub = "/mtbcd221214/update";

WiFiClient espClient;
PubSubClient client(espClient);
TinyGPSPlus gps;
DataToSend dataToSend;
DataForReceive dataForReceive;
String dataReceiveMqtt = "";
DeviceStatus deviceStatus = NONE;

Location savedLocation;
Location curruntLocation;

bool statusAntiTheft = false;
bool statusWarining = false;
bool tempFall = false;

bool statusBuzzer = HIGH;

unsigned long now;
unsigned long last;
int dataUpdateCycle = DATA_UPDATE_CYCLE_DEFAUlT; //
int systemCycle = SYSTEM_CYCLE_DEFAULT;			 // 60000ms = 60s = 1'
char *gpsStream = (char *)malloc(100 * sizeof(char));

void setupWifi()
{
	delay(10);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	randomSeed(micros());
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void getGPS()
{
	char c;
	int check = 0;
	int count = 0;
	gpsStream[73] = '\0';
	while (Serial2.available())
	{
		c = Serial2.read();
		// Serial.print(c);
		if (c == '$')
		{
			check = 1;
		}
		if (check == 1)
		{
			gpsStream[count] = c;
			count++;
		}
		if (c == '\n' && check == 1)
		{
			count = 0;
			check = 0;
			break;
		}
	}
	for (int i = 0; i < strlen(gpsStream); i++)
	{
		if (gps.encode(gpsStream[i]))
		{
		}
	}
	curruntLocation.lat = gps.location.lat();
	curruntLocation.lng = gps.location.lng();
	dataToSend.location[0] = curruntLocation.lat;
	dataToSend.location[1] = curruntLocation.lng;
}

void callback(char *topic, byte *payload, unsigned int length)
{
	bool update = false;
	dataReceiveMqtt = "";
	for (int i = 0; i < length; i++)
	{
		// Serial.print((char)payload[i]);
		dataReceiveMqtt += String((char)payload[i]);
	}
	Serial.println(dataReceiveMqtt);
	dataForReceive = DataForReceive::fromJson(dataReceiveMqtt);

	if (statusAntiTheft != dataForReceive.toggleAntiTheft)
	{
		for (int i = 0; i < 6; i++)
		{
			statusBuzzer = !statusBuzzer;
			digitalWrite(BUZZER, statusBuzzer);
			delay(300);
		}
		statusAntiTheft = dataForReceive.toggleAntiTheft;
		update = true;
	}

	if (dataForReceive.offWarning == true)
	{
		statusWarining = false;
		deviceStatus = NONE;
	}

	dataToSend.status = deviceStatus;
	if (dataForReceive.needUpdateLocation == true || update == true)
	{
		updateData();
		client.publish(topicPub, dataToSend.toJson().c_str());
	}
	Serial.print("Status Anti Theft = ");
	Serial.println(statusAntiTheft);
	// Serial.println(dataForReceive.deviceId);
}

void reconnect()
{
	// Loop until we're reconnected
	while (!client.connected())
	{
		Serial.print("Attempting MQTT connection...");
		// Create a random client ID
		String clientId = "ESPClient-";
		clientId += String(random(0xffff), HEX);
		// Attempt to connect
		if (client.connect(clientId.c_str()))
		{
			Serial.println("connected");
			// Once connected, publish an announcement...
			// ... and resubscribe
			client.subscribe(topicSub);
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

bool checkSosAndAntiTheft(uint8_t sosPin, unsigned int timeOut = 2000U)
{
	if (digitalRead(sosPin) == 0) // sosPin = 0
	{
		int count = 1;
		int timePress = 3000; // 3s
		DataToSend dataToSendSosAndAntiTheft;
		dataToSendSosAndAntiTheft.deviceId = WiFi.macAddress();
		now = millis();
		while (1)
		{
			if ((millis() - now > timePress) && (digitalRead(sosPin) == 0))
			{
				Serial.println("SOS");
				deviceStatus = SOS;
				dataToSendSosAndAntiTheft.antiTheft = statusAntiTheft;
				dataToSendSosAndAntiTheft.status = deviceStatus; // SOS
				statusWarining = true;
				// dataUpdateCycle = DATA_UPDATE_CYCLE_DEFAUlT/5;
				client.publish(topicPub, dataToSendSosAndAntiTheft.toJson().c_str());
				return true;
			}
			while (digitalRead(sosPin) == 1) // neu nha
			{
				// count++;// count = 1
				if (millis() - now > timeOut)
				{
					return false;
				}
				if (digitalRead(sosPin) == 0) // neu bang 0
				{
					count++; // count = 2;
					now = millis();
					while (digitalRead(sosPin) == 0) // doi nha ra
					{
						if (millis() - now > timeOut)
						{
							return false;
						}
					}
					if (count >= 4)
					{
						// duoc bam
						// Serial.println("Antitheft");
						Serial.print("AntiTheft");
						statusAntiTheft = !statusAntiTheft;
						for (int i = 0; i < 6; i++)
						{
							statusBuzzer = !statusBuzzer;
							digitalWrite(BUZZER, statusBuzzer);
							delay(300);
						}
						dataToSendSosAndAntiTheft.antiTheft = statusAntiTheft;
						now = millis();
						if (statusAntiTheft == true)
						{
							getGPS();
							savedLocation.lat = curruntLocation.lat;
							savedLocation.lng = savedLocation.lng;
							dataToSendSosAndAntiTheft.location[0] = savedLocation.lat;
							dataToSendSosAndAntiTheft.location[1] = savedLocation.lng;
							client.publish(topicPub, dataToSendSosAndAntiTheft.toJson().c_str());
						}
						count = 1;
						return true;
					}
				}
			}
			delay(10);
		}
	}
	return true;
}

void setup()
{
	Serial.begin(9600);
	Serial2.begin(9600);
	gpsStream[73] = '\0';
	pinMode(BUTTON_SOS, INPUT_PULLUP);
	pinMode(BUZZER, OUTPUT);
	// statusBuzzer = ! statusBuzzer;
	digitalWrite(BUZZER, statusBuzzer);
	setupWifi();
	dataToSend.deviceId = WiFi.macAddress();
	client.setServer(mqttServer, 1883);
	client.setCallback(callback);
	now = millis();
	last = millis();
	mpuInit();
}

void loop()
{
	// MQTT client connect
	if (!client.connected())
	{
		reconnect();
	}
	client.loop();
	checkSosAndAntiTheft(BUTTON_SOS); // kiểm tra vào chức năng SOS hoặc chống trộm
	if (statusAntiTheft == true)
	{
		deviceStatus = antiTheft(statusAntiTheft, savedLocation, curruntLocation); // chống trộm
	}
	// kiểm tra ngã hoặc đổ xe
	if (deviceStatus != CRASH && deviceStatus != SOS)
	{
		deviceStatus = checkFallandCrash();
	}

	handleDeviceStatus();
	// 	update data
	if (statusWarining == true)
	{
		dataUpdateCycle = DATA_UPDATE_CYCLE_DEFAUlT / 5;
	}
	else
	{
		dataUpdateCycle = DATA_UPDATE_CYCLE_DEFAUlT;
	}
	if ((now - last) % dataUpdateCycle >= (dataUpdateCycle - 10))
	{
		updateData();
	}
	delay(10);
	now = millis();
	if (now - last > systemCycle)
	{
		last = now;
	}
}

bool handleDeviceStatus()
{
	bool updateLocation = false;
	unsigned long time1 = millis();
	unsigned long time2 = millis();
	switch (deviceStatus)
	{
	case NONE:
		// skip
		break;
	case FALL:
		// coi keu + sms sau 5s
		while (millis() - time1 < TIME_CHECK_FALL)
		{
			if (checkFallandCrash() == CRASH)
			{
				deviceStatus = CRASH;
				return false;
			}
			delay(10);
		}
		if (checkFallandCrash() == FALL)
		{
			// fall
			dataToSend.status = FALL;
			time1 = millis();
			time2 = millis();
			// updateData();
			while ((millis() - time2) < 10000U)
			{
				if ((millis() - time1) > 500U)
				{
					digitalWrite(BUZZER, statusBuzzer);
					statusBuzzer = !statusBuzzer;
					time1 = millis();
				}
				if (checkFallandCrash() != FALL)
				{
					statusBuzzer = HIGH;
					digitalWrite(BUZZER, statusBuzzer);
					return true;
				}
				delay(100);
			}
			updateData();
			statusBuzzer = HIGH;
			digitalWrite(BUZZER, statusBuzzer);
			time1 = millis();
			while (checkFallandCrash() == FALL)
			{
				if (millis() - time1 > dataUpdateCycle)
				{
					updateData();
					time1 = millis();
				}
				delay(100);
			}
		}
		break;
	case CRASH:
		dataToSend.status = CRASH;
		updateData();
		dataToSend.status = NONE;
		statusWarining = true;
		while (statusWarining == true)
		{
			// Serial.println("Hi3");
			if ((millis() - time1) > 500U)
			{
				// Serial.println("Hi4");
				digitalWrite(BUZZER, statusBuzzer);
				statusBuzzer = !statusBuzzer;
				time1 = millis();
			}
			if ((millis() - time2) > DATA_UPDATE_CYCLE_DEFAUlT / 5)
			{
				updateData();
			}
			// if (statusWarining == false)
			// {
			// 	statusBuzzer = HIGH;
			// 	digitalWrite(BUZZER, statusBuzzer);
			// 	break;
			// }
			client.loop();
		}
		statusBuzzer = HIGH;
		digitalWrite(BUZZER, statusBuzzer);
		break;
	case LOST1:
		dataToSend.status = deviceStatus;
		updateData();
		statusWarining = true;
		break;
	case LOST2:
		dataToSend.status = deviceStatus;
		updateData();
		statusWarining = true;
		break;
	}
	return true;
}

void updateData()
{
	getGPS();
	// dataToSend.location[0] = curruntLocation.lat;
	// dataToSend.location[1] = curruntLocation.lng;
	dataToSend.status = deviceStatus;
	dataToSend.antiTheft = statusAntiTheft;
	client.publish(topicPub, dataToSend.toJson().c_str());
}
