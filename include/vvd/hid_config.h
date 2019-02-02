#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t vendor_id, product_id;

    // indexes of the io in their respective arrays

    uint8_t vol_l, vol_r; //axes
    uint8_t start; //buttons
    uint8_t bt_a, bt_b, bt_c, bt_d; //buttons
    uint8_t fx_l, fx_r; //button

    uint8_t light_start; //lights
    uint8_t light_bt_a, light_bt_b, light_bt_c, light_bt_d; //lights
    uint8_t light_fx_l, light_fx_r; //lights
} HIDConfig;

// todo: proper setup for creating a config

// returns whether or not an hid config file at the given path is valid to be read
bool hid_config_is_valid(const char *path);

// reads an hid config file from the given path and returns an HIDConfig with the values from it
HIDConfig hid_config_read(const char *path);

// writes an hid config file to the given path with the values from the given HIDConfig
void hid_config_write(HIDConfig config, const char *path);
