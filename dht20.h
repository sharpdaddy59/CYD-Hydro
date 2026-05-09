// dht20.h — DHT20 air temperature + humidity reader.
// Lives on CN1 (SDA = GPIO 22, SCL = GPIO 27), 5-second cadence.

#pragma once

void dht20_start();   // creates and starts the FreeRTOS task
