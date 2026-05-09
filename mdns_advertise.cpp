#include <Arduino.h>
#include <ESPmDNS.h>

#include "mdns_advertise.h"
#include "device_id.h"
#include "config.h"

void mdns_advertise_begin() {
  const char* host = device_hostname();
  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("[mdns] up: http://%s.local:%d\n", host, HTTP_PORT);
  } else {
    Serial.println("[mdns] begin FAILED");
  }
}
