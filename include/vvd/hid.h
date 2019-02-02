#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t report_id;
    uint8_t report_size; //in bits
    uint8_t report_offset; //in bits, from the beginning of a report
    int8_t logical_minimum, logical_maximum;
} HIDIO;

typedef struct
{
    int type; //HID_REQUEST_AXIS/BUTTON/LIGHT
    int io_index;
    void *value; //set for inputs and used for outputs
} HIDRequest;

typedef struct
{
    int type; //HID_REQUEST_INPUT/OUTPUT
    uint8_t report_id;
    uint8_t report_size; //size of the report for report_id, in bits

    int num_requests;
    HIDRequest *requests;
} HIDRequestGroup;

typedef struct
{
    char *devnode_path;
    int fd;
    bool fd_open;

    uint16_t vendor_id, product_id;

    int num_axes, num_buttons, num_lights;
    HIDIO *axes, *buttons, *lights;

    int num_request_groups;
    HIDRequestGroup *request_groups;
} HIDDevice;

HIDDevice *hid_device_get(uint16_t vendor_id, uint16_t product_id);
void hid_device_free(HIDDevice *device);

void hid_device_get_axis(HIDDevice *device, int index, float *value);
void hid_device_get_button(HIDDevice *device, int index, bool *pressed);

void hid_device_set_light(HIDDevice *device, int index, bool on);

void hid_device_update(HIDDevice *device);
