#include "hid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <libudev.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>

// max values
#define HID_MAX_BUFFER                 65
#define HID_DEVICE_MAX_IO              256
#define HID_DEVICE_MAX_REQUEST_GROUPS  64
#define HID_REQUEST_GROUP_MAX_REQUESTS 64

// request types
#define HID_REQUEST_INPUT  1 << 0
#define HID_REQUEST_OUTPUT 1 << 1

#define HID_REQUEST_AXIS   1 << 2 | HID_REQUEST_INPUT
#define HID_REQUEST_BUTTON 1 << 3 | HID_REQUEST_INPUT
#define HID_REQUEST_LIGHT  1 << 4 | HID_REQUEST_OUTPUT

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
#define HID_IOF_CONSTANT 1 << 0

// Assert that a given result is >= 0, prints the given message via perror and exits if it is not.
void assert_result(int result, const char *perror_message)
{
    if (result < 0)
    {
        perror(perror_message);
        exit(1);
    }
}

// Get the devnode path for a device with the given Vendor ID and Product ID.
void hid_device_set_devnode_path(HIDDevice *device)
{
    // whether or not a devnode was found for device
    bool devnode_found = false;

    // get and assert a udev reference
    struct udev *udev = udev_new();
    assert(udev);

    // create a list of the devices in the hidraw subsystem for enumeration
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);

    // get the string representations of devices vendor_id and product_id
    // need them as strings to compare against udev device vids and pids
    char *device_vendor_id = malloc(4 * sizeof(char));
    char *device_product_id = malloc(4 * sizeof(char));

    snprintf(device_vendor_id, 5, "%04x", device->vendor_id);
    snprintf(device_product_id, 5, "%04x", device->product_id);

    // iterate all the device in the device list
    struct udev_list_entry *device_list_entry;
    udev_list_entry_foreach(device_list_entry, devices)
    {
        // the path to the current device list entry
        const char *path = udev_list_entry_get_name(device_list_entry);

        // get the hidraw device for the current entry
        struct udev_device *hidraw_device = udev_device_new_from_syspath(udev, path);
        assert(hidraw_device);

        // get the usb device for the hidraw device by finding its first parent of the usb subsystem
        struct udev_device *usb_device = udev_device_get_parent_with_subsystem_devtype(hidraw_device, "usb", "usb_device");
        assert(usb_device);

        // if the usb device matches the given devices vid and pid
        if (strcmp(udev_device_get_sysattr_value(usb_device, "idVendor"), device_vendor_id) == 0 &&
            strcmp(udev_device_get_sysattr_value(usb_device, "idProduct"), device_product_id) == 0)
        {
            // get the hidraw devices devnode and copy it into device->devnode_path
            const char *devnode = udev_device_get_devnode(hidraw_device);
            int devnode_length = strlen(devnode);
            device->devnode_path = malloc(devnode_length + 1);
            strcpy(device->devnode_path, devnode);

            // say the device was found
            devnode_found = true;
        }

        // unref the devices
        udev_device_unref(hidraw_device);
        udev_device_unref(usb_device);

        // break if the device was found
        if (devnode_found)
            break;
    }

    // release all the udev references
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    free(device_vendor_id);
    free(device_product_id);

    // assert that the devnode was found
    assert(devnode_found);
}

// Open the file descriptor of the given device, if it's closed.
void hid_device_open(HIDDevice *device)
{
    assert(!device->fd_open);

    // open in r/w and nonblocking mode
    device->fd = open(device->devnode_path, O_RDWR | O_NONBLOCK);
    assert_result(device->fd, "open device devnode");
    device->fd_open = true;
}

// Close the file descriptor of the given device, if it's open.
void hid_device_close(HIDDevice *device)
{
    assert(device->fd_open);

    close(device->fd);
    device->fd_open = false;
}

// Get a report descriptor for the given device.
struct hidraw_report_descriptor hid_device_get_report_descriptor(HIDDevice *device)
{
    assert(device->fd_open);

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

// Get the given device's report descriptor and sets it's IO to the values returned from it.
void hid_device_set_io(HIDDevice *device)
{
    struct hidraw_report_descriptor report_descriptor = hid_device_get_report_descriptor(device);

    // values that are stored between each enumeration to set on inputs/outputs
    uint8_t report_id, report_size, report_count;
    int8_t logical_minimum, logical_maximum;
    uint8_t report_offset, report_offset_id; //offset in bits for an input/output in a report

    // io arrays are allocated with an initial max size, then at the end reallocated to fit their items
    device->axes = malloc(HID_DEVICE_MAX_IO * sizeof(HIDIO));
    device->buttons = malloc(HID_DEVICE_MAX_IO * sizeof(HIDIO));
    device->lights = malloc(HID_DEVICE_MAX_IO * sizeof(HIDIO));

    for (int i = 0; i < report_descriptor.size;)
    {
        uint8_t key = report_descriptor.value[i];
        uint8_t value = report_descriptor.value[i + 1];

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
                // constant io isnt useful for anything this program uses, so ignore it
                if (value & HID_IOF_CONSTANT)
                    break;

                // ensure report_count is at least 1 so io isnt skipped when it doesnt define it
                if (report_count == 0)
                    report_count = 1;

                // reset report_offset if the report id changed
                if (report_offset_id != report_id)
                {
                    report_offset = 0;
                    report_offset_id = report_id;
                }

                HIDIO io = (HIDIO)
                {
                    report_id,
                    report_size,
                    0,
                    logical_minimum,
                    logical_maximum,
                };

                // iterate all the reports are add them to their respective io arrays
                for (int i = 0; i < report_count; i++)
                {
                    io.report_offset = report_offset;

                    switch (key)
                    {
                        case HID_RD_INPUT:
                            if (io.logical_minimum == 0 && io.logical_maximum == 1)
                            {
                                // button
                                device->buttons[device->num_buttons] = io;
                                device->num_buttons++;
                            }
                            else
                            {
                                // axes
                                device->axes[device->num_axes] = io;
                                device->num_axes++;
                            }
                            break;
                        case HID_RD_OUTPUT:
                            // light
                            device->lights[device->num_lights] = io;
                            device->num_lights++;
                            break;
                    }

                    report_offset++;
                }

                break;
        }

        // advance i so keys/values arent repeated
        if (key == HID_RD_END_COLLECTION)
            // end_collection is the only key without a value
            i += 1;
        else
            i += 2;
    }

    // realloc the io to fit their items
    device->axes = realloc(device->axes, device->num_axes * sizeof(HIDIO));
    device->buttons = realloc(device->buttons, device->num_buttons * sizeof(HIDIO));
    device->lights = realloc(device->lights, device->num_lights * sizeof(HIDIO));
}

// Get the first HIDDevice matching the given Vendor ID and Product ID.
HIDDevice *hid_device_get(uint16_t vendor_id, uint16_t product_id)
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

    // allocate the request groups array
    device->request_groups = malloc(HID_DEVICE_MAX_REQUEST_GROUPS * sizeof(HIDRequestGroup));

    // return the device
    return device;
}

// Free an HIDDevice from memory.
void hid_device_free(HIDDevice *device)
{
    hid_device_close(device);
    free(device->devnode_path);
    free(device->axes);
    free(device->buttons);
    free(device->lights);

    for (int i = 0; i < device->num_request_groups; i++)
        free(device->request_groups[i].requests);

    free(device->request_groups);
    free(device);
}

// Add the combined report size of the given HIDIO that match the given HIDRequestGroup's report ID.
void hid_request_group_add_report_size(HIDRequestGroup *request_group, HIDIO *ios, int num_ios)
{
    for (int i = 0; i < num_ios; i++)
        if (ios[i].report_id == request_group->report_id)
            request_group->report_size += ios[i].report_size;
}

// Find or create an HIDRequestGroup in the given device with the given type and report ID and return it.
HIDRequestGroup *hid_device_find_or_create_request_group(HIDDevice *device, int type, uint8_t report_id)
{
    assert(type == HID_REQUEST_INPUT || type == HID_REQUEST_OUTPUT);

    // try to find a request group with a matching report id and type
    for (int i = 0; i < device->num_request_groups; i++)
        if (device->request_groups[i].report_id == report_id && device->request_groups[i].type == type)
            return &device->request_groups[i];

    // a request group wasnt found, create one
    device->request_groups[device->num_request_groups] = (HIDRequestGroup)
    {
        .type = type,
        .report_id = report_id,
        .report_size = 0,
        .num_requests = 0,
        .requests = malloc(HID_REQUEST_GROUP_MAX_REQUESTS * sizeof(HIDRequestGroup)),
    };

    device->num_request_groups++;

    // get a pointer to the new request group
    HIDRequestGroup *request_group = &device->request_groups[device->num_request_groups - 1];

    // set request_group->report_size
    // todo: could be better, not great to loop every frame
    switch (type)
    {
        case HID_REQUEST_INPUT:
            hid_request_group_add_report_size(request_group, device->axes, device->num_axes);
            hid_request_group_add_report_size(request_group, device->buttons, device->num_buttons);
            break;
        case HID_REQUEST_OUTPUT:
            hid_request_group_add_report_size(request_group, device->lights, device->num_lights);
            break;
    }

    return request_group;
}

// Add a new HIDRequest to the given device for the given report ID, index, and value;
void hid_device_add_request(HIDDevice *device, int type, uint8_t report_id, int index, void *value)
{
    assert(type == HID_REQUEST_AXIS || type == HID_REQUEST_BUTTON || type == HID_REQUEST_LIGHT);

    // get the reqeust group type from the request type
    int request_group_type;
    if (type & HID_REQUEST_INPUT)
        request_group_type = HID_REQUEST_INPUT;
    else if (type & HID_REQUEST_OUTPUT)
        request_group_type = HID_REQUEST_OUTPUT;

    // get the request group
    HIDRequestGroup *request_group = hid_device_find_or_create_request_group(device, request_group_type, report_id);

    // add a new request to it
    request_group->requests[request_group->num_requests] = (HIDRequest)
    {
        .type = type,
        .io_index = index,
        .value = value,
    };

    request_group->num_requests++;
}

// Add an axis input request to the given device with the given index and value.
// Index is the index of the axis to get in device->axes.
// Value is set when hid_device_update is called on the device.
// Value is on a scale of 0 to 1 representing the position of the axis.
void hid_device_get_axis(HIDDevice *device, int index, float *value)
{
    assert(index < device->num_axes);
    HIDIO *axis = &device->axes[index];

    hid_device_add_request(device,
                           HID_REQUEST_AXIS,
                           axis->report_id,
                           index,
                           value);
}

// Add a button input request to the given device with the given index and value.
// Index is the index of the button to get in the device->buttons.
// Value is set when hid_device_update is called on the device.
// Value is true when the button is pressed and false when not.
void hid_device_get_button(HIDDevice *device, int index, bool *pressed)
{
    assert(index < device->num_buttons);
    HIDIO *button = &device->buttons[index];

    hid_device_add_request(device,
                           HID_REQUEST_BUTTON,
                           button->report_id,
                           index,
                           pressed);
}

// Add a light outout request to the given device with the given index and value.
// Index is the index of the light to set in the device->lights.
// Value is applied when hid_device_update is called on the device.
// If value is true the light is turned on, if false it is turned off.
void hid_device_set_light(HIDDevice *device, int index, bool on)
{
    assert(index < device->num_lights);
    HIDIO *light = &device->lights[index];

    hid_device_add_request(device,
                           HID_REQUEST_LIGHT,
                           light->report_id,
                           index,
                           on ? &light->logical_maximum : &light->logical_minimum);
}

// Process and clear out the given devices requests.
void hid_device_update(HIDDevice *device)
{
    // iterate the devices request groups
    for (int rgi = 0; rgi < device->num_request_groups; rgi++)
    {
        HIDRequestGroup *request_group = &device->request_groups[rgi];
        char buffer[HID_MAX_BUFFER];

        memset(buffer, 0x00, sizeof(buffer));

        // the first byte of the buffer is always the report id
        buffer[0] = request_group->report_id;

        // + 1 to include the report id
        uint8_t report_size = 1 + ceil((float)request_group->report_size / 8.0);

        // process inputs so the report can be read into the requests
        if (request_group->type == HID_REQUEST_INPUT)
        {
            // read all the unread results
            // this is necessary as multiple reports can occur during a frame
            // so when the device tries to update it will be behind on reports
            while(1)
            {
                int result = read(device->fd, buffer, report_size);

                // the EAGAIN error (resource temporarily unavailable) is set when there is no more data to read
                if (result == -1 && errno == EAGAIN)
                    break;

                assert_result(result, "reading hid report");
            }
        }

        // iterate the request groups requests
        for (int ri = 0; ri < request_group->num_requests; ri++)
        {
            HIDRequest *request = &request_group->requests[ri];
            HIDIO *io;

            switch (request->type)
            {
                case HID_REQUEST_AXIS:
                    io = &device->axes[request->io_index];
                    break;
                case HID_REQUEST_BUTTON:
                    io = &device->buttons[request->io_index];
                    break;
                case HID_REQUEST_LIGHT:
                    io = &device->lights[request->io_index];
                    break;
            }

            // the offset, in bytes, from the beginning of the buffer for values to be read from/written to
            // + 1 for the report id
            int offset = 1 + (io->report_offset + io->report_size) / 8;

            switch (request->type)
            {
                case HID_REQUEST_AXIS:
                {
                    // some devices like to not abide by the logical min/max they report, so force them to
                    int8_t capped_value = buffer[offset];
                    capped_value = capped_value < io->logical_minimum ? io->logical_minimum : capped_value;
                    capped_value = capped_value > io->logical_maximum ? io->logical_maximum : capped_value;

                    // store commonly used values
                    int8_t min = io->logical_minimum, max = io->logical_maximum;
                    float value;

                    // handle negative logical minimums
                    if (min < 0)
                        value = (float)(capped_value + -min) / (float)(max + -min);
                    else
                        value = (float)(capped_value - min) / (float)(max - min);

                    // set the value
                    *(float *)request->value = value;
                    break;
                }
                case HID_REQUEST_BUTTON:
                    // set value to bit request->io_index of buffer
                    *(bool *)request->value = (buffer[offset] >> io->report_offset) & 1;
                    break;
                case HID_REQUEST_LIGHT:
                    // set bit io->report_offset of byte offset in buffer to request->value
                    buffer[offset] = buffer[offset] & (~(1 << io->report_offset)) | (*(int8_t *)request->value << request->io_index);
                    break;
            }
        }

        // process outputs now that the buffer is set
        if (request_group->type == HID_REQUEST_OUTPUT)
        {
            int result = write(device->fd, buffer, report_size);
            assert_result(result, "writing hid report");
        }

        // free the requests
        free(request_group->requests);
        request_group->num_requests = 0;
    }

    // reset the request group count
    // request_groups isnt freed as the next gets/sets will overwrite previous values
    // this makes it so it doesnt have to be freed and mallocd every update
    device->num_request_groups = 0;
}
