#include "hid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>

// Assert that a given result is >= 0, prints the given message via perror and exits if it is not.
void assert_result(int result, const char *perror_message)
{
    if (result < 0)
    {
        perror(perror_message);
        exit(result);
    }
}

// Get the devnode path for a device with the given Vendor ID and Product ID.
void hid_device_set_devnode_path(HIDDevice *device)
{
    // whether or not a matching device was found
    bool devnode_found = false;

    // get a udev reference and assert that its valid
    struct udev *udev = udev_new();
    assert(udev);

    // create a list of the devices in the hidraw subsystem for enumeration
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);

    // the current entry in the devices enumeration
    struct udev_list_entry *device_list_entry;

    // enumerate each device and find the first one with a matching vid and pid
    udev_list_entry_foreach(device_list_entry, devices)
    {
        // the path for the current entry
        const char *path = udev_list_entry_get_name(device_list_entry);

        // get the hidraw device for the current entry
        struct udev_device *hidraw_device = udev_device_new_from_syspath(udev, path);
        assert(hidraw_device);

        // get the usb device for the hidraw device by finding its first parent of the usb subsystem
        struct udev_device *usb_device = udev_device_get_parent_with_subsystem_devtype(hidraw_device, "usb", "usb_device");
        assert(usb_device);

        // if the usb device matches the given devices vid and pid
        if (strcmp(udev_device_get_sysattr_value(usb_device, "idVendor"), device->vendor_id) == 0 &&
            strcmp(udev_device_get_sysattr_value(usb_device, "idProduct"), device->product_id) == 0)
        {
            // get the hidraw devices devnode and copy it into device->devnode_path
            const char *devnode = udev_device_get_devnode(hidraw_device);
            int devnode_length = strlen(devnode);
            device->devnode_path = malloc(devnode_length + 1);
            strcpy(device->devnode_path, devnode);

            // say the devnode was found
            devnode_found = true;
        }

        // unref the devices
        udev_device_unref(hidraw_device);
        udev_device_unref(usb_device);

        // break if the devnode was found
        if (devnode_found)
            break;
    }

    // release all the udev references
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    // assert that the devnode was found
    assert(devnode_found);
}

// Open the file descriptor of the given device, if it's closed.
void hid_device_open(HIDDevice *device)
{
    // assert that the device is closed
    assert(!device->fd_open);

    // open the file descriptor
    device->fd = open(device->devnode_path, O_RDWR | O_NONBLOCK);

    // assert that the file descriptor is valid
    assert_result(device->fd, "open device devnode");

    // update fd_open
    device->fd_open = true;
}

// Close the file descriptor of the given device, if it's open.
void hid_device_close(HIDDevice *device)
{
    // assert that the device is open
    assert(device->fd_open);

    // close the file descriptor
    close(device->fd);

    // update fd_open
    device->fd_open = false;
}

// Get a report descriptor for the given device.
struct hidraw_report_descriptor hid_device_get_report_descriptor(HIDDevice *device)
{
    // assert the device is open open
    assert(device->fd_open);

    // the last ioctl call result
    int result;

    // get the report descriptor size
    int report_descriptor_size;
    result = ioctl(device->fd, HIDIOCGRDESCSIZE, &report_descriptor_size);
    assert_result(result, "HIDIOCGRDESCSIZE");

    // get the report descriptor
    struct hidraw_report_descriptor report_descriptor;
    report_descriptor.size = report_descriptor_size;
    result = ioctl(device->fd, HIDIOCGRDESC, &report_descriptor);
    assert_result(result, "HIDIOCGRDESC");

    // return the report descriptor
    return report_descriptor;
}

// Gets the given device's report descriptor and sets it's IO to the values returned from it.
void hid_device_set_io(HIDDevice *device)
{
    // the report descriptor for the given device
    struct hidraw_report_descriptor report_descriptor = hid_device_get_report_descriptor(device);

    // the inputs/outputs for the given device
    uint8_t input_count = 0, output_count = 0;
    HIDIO *inputs = NULL, *outputs = NULL;

    // values that are stored between each enumeration to set on inputs/outputs
    uint8_t report_id, report_size, report_count;
    int8_t logical_minimum, logical_maximum;

    // enumerate through all the report descriptor values
    for (int i = 0; i < report_descriptor.size;)
    {
        // the key value pair for the current value in the report descriptor
        // todo: test with vendor defined values, which have 2 bytes instead of 1
        uint8_t key = report_descriptor.value[i];
        uint8_t value = report_descriptor.value[i + 1];

        // handle the key accordingly
        switch (key)
        {
            case HID_RD_REPORT_ID:
                report_id = value;
                break;
            case HID_RD_REPORT_SIZE:
                report_size = value;
                break;
            case HID_RD_REPORT_COUNT:
                report_count = value;
                break;
            case HID_RD_LOGICAL_MINIMUM:
                logical_minimum = value;
                break;
            case HID_RD_LOGICAL_MAXIMUM:
                logical_maximum = value;
                break;
            case HID_RD_INPUT:
            case HID_RD_OUTPUT:
                // constant io isnt useful for input, so ignore it
                if (value & HID_IOF_CONSTANT)
                    break;

                // the hidio for the current io
                HIDIO io = (HIDIO)
                {
                    report_id,
                    report_size,
                    report_count,
                    logical_minimum,
                    logical_maximum,
                };

                switch (key)
                {
                    case HID_RD_INPUT:
                        // add io to the end of inputs, resizing it to fit
                        input_count++;
                        inputs = realloc(inputs, input_count * sizeof(HIDIO));
                        inputs[input_count - 1] = io;
                        break;
                    case HID_RD_OUTPUT:
                        // add io to the end of outputs, resizing it to fit
                        output_count++;
                        outputs = realloc(outputs, output_count * sizeof(HIDIO));
                        outputs[output_count - 1] = io;
                        break;
                }

                break;
        }

        // end_collection is the only key without a value
        if (key == HID_RD_END_COLLECTION)
            i += 1;
        else
            i += 2;
    }

    // fill in the given devices values
    device->input_count = input_count;
    device->output_count = output_count;

    device->inputs = inputs;
    device->outputs = outputs;
}

// Get the first HIDDevice matching the given Vendor ID and Product ID.
HIDDevice *hid_device_get(const char *vendor_id, const char *product_id)
{
    // create the device
    HIDDevice *device = malloc(sizeof(HIDDevice));

    // set the devices values
    device->vendor_id = vendor_id;
    device->product_id = product_id;
    hid_device_set_devnode_path(device);

    // open the device and set its io
    hid_device_open(device);
    hid_device_set_io(device);

    // return the device
    return device;
}

// Free an HIDDevice from memory.
void hid_device_free(HIDDevice *device)
{
    hid_device_close(device);
    free(device->devnode_path);
    free(device->inputs);
    free(device->outputs);
    free(device);
}
