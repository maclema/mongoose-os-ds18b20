# DS18B20 C Library for Mongoose OS

## Overview

Ultra-simple C library for DS18B20 sensors. Connect all the sensors data pins
to a single GPIO pin, and then call ds18b20_read_all. That's it.

## Install

Add to libs section of your mos.yml.

```yml
libs:
    https://github.com/maclema/mongoose-os-ds18b20
```

## Usage

```c
#include "mgos.h"
#include "ds18b20.h"

// Temperatures callback
void temperatures_cb(struct ds18b20_result *results) {
    // Loop over each result
    while ( results != NULL ) {
        // results->rom - uint8_t - Sensor ROM
        // results->mac - char* - MAC address string
        // results->temp - float - Temperature in celsius
        printf("ROM: %s, Temp: %f\n", results->mac, results->temp);
        results = results->next;
    }
}

// Mongoose application initialization
enum mgos_app_init_result mgos_app_init(void) {
    // Read all the temperatures (GPIO 4, 9-bit resolution)
    ds18b20_read_all(4, 9, temperatures_cb);
    
    // Init OK
    return MGOS_APP_INIT_SUCCESS;
}

// DS18B20 Resolution Configuration
// 9-bit resolution:   97.5ms read time
// 10-bit resolution:  187.5ms read time
// 11-bit resolution:  375ms read time
// 12-bit resolution:  750ms read time
```

## Multi-Sensor Wiring

```
  [---]
  [---]
  | | |
  1 2 3

  PIN 1: GND
  PIN 2: Data
  PIN 3: Vcc (3.3v - 5v)
```

- All GND pins to ground
- All Vcc pins to +5v
- All data pins to GPIO pin X
- Connect +5v to GPIO pin X with a 4.7k resistor (pullup resistor - this is important when using long wires for the sensors)
