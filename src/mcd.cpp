
#include "mcd.h"

String DataToSend::toJson()
{
    StaticJsonDocument<200> doc;
    StaticJsonDocument<200> doc1;
    String result = "";
    JsonArray jsonLocation = doc1.createNestedArray();
    jsonLocation.add(location[0]);
    jsonLocation.add(location[1]);
    // Add values to the document
    doc["deviceId"] = deviceId;
    doc["location"] = jsonLocation;
    doc["status"] = status;
    doc["toggleAntiTheft"] = toggleAntiTheft;
    // Serialize the JSON document
    serializeJson(doc, result);
    return result;
}

DataForReceive DataForReceive::fromJson(String jsonData)
{
    DataForReceive dataForReceive;

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonData);
    JsonObject obj = doc.as<JsonObject>();

    dataForReceive.deviceId = obj[F("deviceId")].as<String>();
    dataForReceive.needUpdateLocation = obj[F("needUpdateLocation")].as<bool>();
    dataForReceive.toggleAntiTheft = obj[F("toggleAntiTheft")].as<bool>();
    dataForReceive.offWarning = obj[F("offWarning")].as<bool>();

    return dataForReceive;
}
