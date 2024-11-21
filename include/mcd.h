#ifndef MCD_H
#define MCD_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

enum DeviceStatus
{
  NONE = 0,  // Bình thường
  FALL = 1,  // Xe bị đổ (sms + buzzer)
  CRASH = 2, // Xe tai nạn (call + buzzer)
  LOST1 = 3, // Xe ra khỏi vùng an toàn  R > 10m (sms)
  LOST2 = 4, // Xe ra khỏi vùng an toàn  R > 50m (call)
  SOS = 5    // SOS (sms + call)
};

struct DataToSend
{
  String deviceId = "";
  std::vector<double> location = {0, 0};
  DeviceStatus status = NONE;
  bool toggleAntiTheft = false;
  String toJson();
};

struct DataForReceive
{
  String deviceId;
  bool needUpdateLocation = false;
  bool toggleAntiTheft = false;
  bool offWarning = false;
  static DataForReceive fromJson(String jsonData);
};
#endif