#include <stdlib.h>
#include "mgos.h"
#include "mgos_onewire.h"
#include "ds18b20.h"

// Helper for allocating new things
#define new(what) (what *)malloc(sizeof(what))

// Helper for allocating strings
#define new_string(len) (char *)malloc(len * sizeof(char))

// Converts a uint8_t rom address to a MAC address string
#define to_mac(r, str) sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7])

// Read all temperatures
void ds18b20_read_all(int pin, int res, ds18b20_read_t callback) {
    uint8_t rom[8], data[9];
    int16_t raw;
    int us, cfg;
    struct mgos_onewire *ow;
    struct ds18b20_result *temp, *list = NULL;

    // Step 1: Determine config
    if ( res == 9 )       { cfg=0x1F;  us=93750;  } // 9-bit resolution (93.75ms delay)
    else if ( res == 10 ) { cfg=0x3F;  us=187500; } // 10-bit resolution (187.5ms delay)
    else if ( res == 11 ) { cfg=0x5F;  us=375000; } // 11-bit resolution (375ms delay)
    else /* 12-bit */     { cfg=0x7F;  us=750000; } // 12-bit resolution (750ms delay)

    // Step 2: Find all the sensors
    ow = mgos_onewire_create(pin);                  // Create one-wire
    mgos_onewire_search_clean(ow);                  // Reset search
    while ( mgos_onewire_next(ow, rom, 1) ) {       // Loop over all devices
        if (rom[0] != 0x28) continue;               // Skip devices that are not DS18B20's
        temp = new(struct ds18b20_result);          // Create a new results struct
        if ( temp == NULL ) {                       // Make sure it worked
            printf("Memory allocation failure!");   // If not, print a useful message
            exit(1);                                // And blow up
        }
        temp->rom = rom;                            // Set the rom attribute
        temp->mac = new_string(23);                 // Allocate a string for the MAC address
        to_mac(rom, temp->mac);                     // Convert the rom to a MAC address string
        temp->next = list;                          // link to previous sensor
        list = temp;                                // set list point to new result
    }

    // Step 3: Write the configuration
    mgos_onewire_reset(ow);                         // Reset
    mgos_onewire_write(ow, 0xCC);                   // Skip Rom
    mgos_onewire_write(ow, 0x4E);                   // Write to scratchpad
    mgos_onewire_write(ow, 0x00);                   // Th or User Byte 1
    mgos_onewire_write(ow, 0x00);                   // Tl or User Byte 2
    mgos_onewire_write(ow, cfg);                    // Configuration register
    mgos_onewire_write(ow, 0x48);                   // Copy scratchpad
    
    // Step 4: Start temperature conversion
    mgos_onewire_reset(ow);                         // Reset
    mgos_onewire_write(ow, 0xCC);                   // Skip Rom (starts conversion on all sensors)
    mgos_onewire_write(ow, 0x44);                   // Start conversion
    mgos_usleep(us);                                // Wait for conversion

    // Step 5: Read the temperatures
    temp = list;                                    // Temporary results holder
    while ( temp != NULL ) {                        // Loop over all devices
        mgos_onewire_reset(ow);                     // Reset
        mgos_onewire_select(ow, temp->rom);         // Select the device
        mgos_onewire_write(ow, 0xBE);               // Issue read command
        mgos_onewire_read_bytes(ow, data, 9);       // Read the 9 data bytes
        raw = (data[1] << 8) | data[0];             // Get the raw temperature
        cfg = (data[4] & 0x60);                     // Read the config (just in case)
        if (cfg == 0x00)      raw = raw & ~7;       // 9-bit raw adjustment
        else if (cfg == 0x20) raw = raw & ~3;       // 10-bit raw adjustment
        else if (cfg == 0x40) raw = raw & ~1;       // 11-bit raw adjustment
        temp->temp = (float) raw / 16.0;            // Convert to celsius and store the temp
        temp = temp->next;                          // Switch to the next sensor in the list
    }

    // Step 6: Invoke the callback
    callback(list);

    // Step 7: Cleanup
    while ( list != NULL ) {                        // Loop over all device results
        temp = list->next;                          // Store a ref to the next device
        free(list->mac);                            // Free up the MAC address string
        free(list);                                 // Free up the struct
        list = temp;                                // Cleanup next device
    }
    mgos_onewire_close(ow);                         // Close one wire
}

bool mgos_mongoose_os_ds18b20_init(void) {
    return true;
}
