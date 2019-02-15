# vvd

*Temporary development name.*

A SOUND VOLTEX simulator for Raspberry Pi that aims to emulate the original game as accurately as possible.

Tested on a Raspberry Pi 1B (256mb of RAM, 128mb GPU split) running Raspbian Lite (November 2018 update.)

# Installation

Note that this is only intended to run on an RPi and will not run on other systems.

This program also runs off the framebuffer so it does not need an X session to run.

```
git clone https://github.com/aoki-marika/vvd
cd vvd
sudo apt-get install build-essential libudev-dev
make
```

The executable will now be in the `bin/` directory at the root of the project.

# Controller Setup

Due to the way Linux handles program access to HID, some manual setup is required to enable your controller in vvd.

Note that this will make your controller readable and writeable by any user on the system.

* Run `lsusb`, and find your device in the list.
* Get the PID and VID of your device.
    * The PID and VID are the colon separated values before the device name in the list (`ID VID:PID Name`)
* Create a file called `99-something.rules` in `/etc/udev/rules.d/`, `something` can be anything (I name mine after the controller, `99-sdvx-arduino-leonardo.rules`).
* Fill the file with the following, replacing `VID` and `PID` with the values you got from `lsusb`:

```
KERNEL=="hidraw*", ATTRS{busnum}=="1", ATTRS{idVendor}=="VID", ATTRS{idProduct}=="PID", MODE=="0666"
```

* Reload the udev rules by running `sudo udevadm control --reload-rules && sudo udevadm trigger`

Your controller should now be usable with vvd.

**TODO**: Add controller setup in the program to create an HID config.
