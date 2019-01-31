#pragma once

#include <stdint.h>
#include <stdbool.h>

// report descriptor keys
#define HID_RD_REPORT_ID       0x85
#define HID_RD_REPORT_SIZE     0x75
#define HID_RD_REPORT_COUNT    0x95
#define HID_RD_LOGICAL_MINIMUM 0x15
#define HID_RD_LOGICAL_MAXIMUM 0x25
#define HID_RD_INPUT           0x81
#define HID_RD_OUTPUT          0x91
#define HID_RD_END_COLLECTION  0xC0

// io value flags
// can take into account more flags if there are problems with only detecting constants
#define HID_IOF_CONSTANT 1 << 0

typedef struct
{
    // The ID for the report of this IO.
    uint8_t report_id;

    // The size, in bits, of each value in a report for this IO.
    uint8_t report_size;

    // The amount of values in a report for this IO.
    uint8_t report_count;

    // The logical minimum and maximum values for a value returned in a report for this IO.
    // In the future this may need to be updated to take into account units and other min/max values.
    int8_t logical_minimum, logical_maximum;
} HIDIO;

typedef struct
{
    // The path to this devices devnode.
    char *devnode_path;

    // The file descriptor to this devices devnode, if it's open.
    int fd;

    // Whether or not `fd` is currently open.
    bool fd_open;

    // The respective Vendor and Product IDs of this device, in hexadecimal, in char arrays (no preceding 0x).
    const char *vendor_id, *product_id;

    // The count of the items in inputs and outputs, respectively.
    uint8_t input_count, output_count;

    // The inputs and outputs of this device.
    HIDIO *inputs, *outputs;
} HIDDevice;

HIDDevice *hid_device_get(const char *vendor_id, const char *product_id);
void hid_device_free(HIDDevice *device);
