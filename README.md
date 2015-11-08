# dlc300-userspace-linux

GUI application for using the DLC300 camera.


## Description

This linux application connects to a DLC300 (or compatible) camera, and
can live view the video stream, save snapshots, and control some
of the cameras parameters, such as exposure, white balance, and
crop.

The camera have vendor ID / product ID 1578:0076, and is visible in dmesg as
```
usb 8-2.1.3: New USB device found, idVendor=1578, idProduct=0076
usb 8-2.1.3: New USB device strings: Mfr=1, Product=2, SerialNumber=3
usb 8-2.1.3: Product: USB2-Camera
usb 8-2.1.3: Manufacturer: Digital Lab
usb 8-2.1.3: SerialNumber: HQDLFW-01.03
```

The application currently work for ubuntu 14.04 (only OS I can verify it on),
but it's probably still working for ubuntu 12.04 (my previous OS).


## Screenshot

![Screenshot](/../screenshots/screenshots/screenshot.png?raw=true "Screenshot")


## Controls available in the SDL window

```
+/- to change exposure
F1  to take a snapshot
F2  to start/stopping taking snapshots continuously
F3  Set the white balance (something grey should be in the center of the view)
F4  to cycle the cameras resolution
ESC to quit the program
```


## Command line options

```
-r         Sets resolution of images. Valid resolutions are:
           640x480, 800x600, 1024x768, 1280x1024, 1600x1200, 2048x1536.
           Note that the image is cropped whenever a resolution smaller then
           the sensors native resolution is requested
-e 1..370  Sets exposure
-g 0..63   Sets gain (the same value is used for all channels)
-b         "Blind mode", no visual imaging. It saves a few image before exiting
-c         DO NOT Center cropped area in low resolution modes (possibly needed for compatibility with other cameras)
-v         Verbose debug output (for developers)
-h         Shows this help message
```


## Compile and install (ubuntu 14.04)

```
# Check out the source code
sudo apt-get install git
git clone https://github.com/optisimon/dlc300-userspace-linux.git

# Install dependencies
sudo apt-get install libsdl1.2-dev libsdl-gfx1.2-dev libusb-1.0-0-dev

# Compile it
cd dlc300-userspace-linux/src
make

# Install it
sudo make install
```


## Uninstall

```
#uninstall it
sudo make uninstall
```


## Known bugs

 - Is a bit sensitive to how the camera is connected. Typically works on externally powered USB hubs,
 and on my USB3 hub, but not on my front USB ports on my computer.

 - All commands was reverse engineered by looking at how the supplied windows software behaved.
 Even though I've tried to do the same transfers on the bus, there might be timing related problems,
 or error recovery code in the windows software that I've never saw while reverse engineering it.
 
