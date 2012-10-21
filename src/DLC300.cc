/**
 * Test talking to some strange USB camera using libusb.
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#include "DLC300.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> //for sleep
#include <stdlib.h> //for exit

/*
 * These 15 registers are sent from the bridge chip to the image sensor for each frame we capture.
 * No other registers besides the following, and a few others only written at power up is ever accessed.
 *
 *
 * Registers sent to the sensor before each frame, when windows software was set to a resolution of 2048x1536:
 *
 *	reg  value  description
 *	0x01 0x0014 (default), row start
 *	0x02 0x0020 (default), column start
 *	0x03 0x0600 (default 0x5FF), row size
 *	0x04 0x0800 (default 0x07ff), column size
 *
 *	0x08 0x0000 (default 0 Shutter Width Upper)
 *	0x09 0x0BB8 (default 0x619), Shutter Width
 *
 *	0x2D 0x002C Red Gain
 *	0x2B 0x002C Green1 Gain
 *	0x2C 0x002C Blue Gain
 *	0x2E 0x002C Green2 Gain
 *
 *	0x62 0x0499 Black Level Calibration
 *	0x63 0x0000 Red Offset
 *	0x64 0x0000 Blue Offset
 *	0x60 0x0000 Cal Green1 Offset
 *	0x61 0x0000 Cal Green2 Offset
 *
 *
 *	The following registers are not set for each frame, but was set once after power-on
 *	reg  value  description
 *	0x05 0x02BC (default 0x008E), horizontal blanking
 *	0x06 0x0019 (LSB = chip enable)
 *	0x07 0x0002 This register controls various features of the output format for the sensor.
 *	0x1E 0xC040 (default 0xC040) “Read Mode 1”
 *	0x20 0x2000 (default 0x2000) “Read Mode 2”
 *	0x09 0x0064 (default 0x619), Shutter Width
 */


/** This class contains the raw data commands which are sent to the camera to start capturing a single new frame. */
struct DlcMsgStruct
{
	// Always contains  2c 00 0e 00 01 00 00 00 00 00 00 00 20 00
	uint8_t unknown_fixed_field1[14];


	/* The following eight bytes depend on the requested camera resolution.
	 * Note that my camera never scales the image, it just crops out a small region of the image when a smaller resolution is selected.
	 *
	 * The first byte is always identical during one session, but seems to sometimes vary between launches of the windows program. Bug? Feature?
	 *
	 * 640x480:   51 07 e0 01 00 00 80 02
	 * 640x480:   07 07 e0 01 00 00 80 02
	 *
	 * 800x600:   25 07 58 02 00 00 20 03
	 *
	 * 1024x768:  46 07 00 03 00 00 00 04
	 *
	 * 1280x1024: 30 07 c0 03 00 00 00 05  // warning: 0x3C0=960 lines, is the windows GUI reporting the wrong resolution?
	 *
	 * 1600x1200: 53 07 b0 04 00 00 40 06
	 *
	 * 2048x1536: 12 07 00 06 00 00 00 08
	 *
	 *                  ^^^^^       ^^^^^
	 *                  #LINES      #ROWS
	 */
	uint8_t possibly_random_byte;
	uint8_t always_0x07; // should be 0x07
	uint8_t col_size_lsb;
	uint8_t col_size_msb;
	uint8_t zeros0[2];
	uint8_t row_size_lsb;
	uint8_t row_size_msb;


	//
	// The following portion contains crop region dependent information
	//
	uint8_t zeros2[2];
	uint8_t col_start_lsb;
	uint8_t col_start_msb;
	uint8_t zeros3[2];
	uint8_t row_start_lsb;
	uint8_t row_start_msb;
	uint8_t zeros4[2];


	//
	// Exposure
	//
	// Maybe call it shutter width as in the datasheet?
	// Should verify USB snoop/I2C writes, so they match?
	// The windows software had some strange scaling of these values...
	uint8_t exposure_lsb;
	uint8_t exposure_msb; // Windows software with exposure set to maximum only touches bit0 in this byte...
	uint8_t zeros5[2];

	//
	// Gain values and offsets for individual colors
	//
	// (probably used for white balance)
	uint8_t red_gain;
	uint8_t green_gain;
	uint8_t blue_gain;
	uint8_t unknown_fixed_field2[2]; // contains 0x80 0x02 (640)
	int8_t  red_offset;   // needs verification. bit7 high --> negative sign on value
	int8_t  green_offset; // needs verification. bit7 high --> negative sign on value
	int8_t  blue_offset;  // needs verification. bit7 high --> negative sign on value
	uint8_t zeros6[20];


	/**
	 * Fill this structure with default values
	 * @note possibly considered a bad way of doing this, but assigning ALL member variables the usual way would look even worser
	 */
	void fillDefaults()
	{
		const uint8_t default_values[64] = {
				0x2c, 0x00, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x12, 0x07,
				0x00, 0x06, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x5c, 0x00, 0x00, 0x00, 0x2c, 0x2c, 0x2c, 0x80, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

		memcpy(this, default_values, sizeof(default_values));
	}


	/**
	 * Sets the top-left corner of the crop region.
	 *
	 * A crop region is used every time a resolution smaller than the cameras native resolution is requested.
	 * (The current firmware in the camera seems to be unaware of the scaling registers built into the sensor)
	 */
	void setCropStart(int xpos, int ypos)
	{
		col_start_msb = xpos >> 8;
		col_start_lsb = xpos & 0xFF;

		row_start_msb = ypos >> 8;
		row_start_lsb = ypos & 0xFF;
	}


	/** Sets camera exposure */
	void setExposure(uint16_t val)
	{
		exposure_msb = val >> 8;
		exposure_lsb = val & 0xFF;
	}


	/**
	 * Set individual gain levels for each color channel
	 * These gain values are applied in the analog domain, before the image is digitized.
	 */
	void setGains(int R, int G, int B)
	{
		red_gain = R;
		green_gain = G;
		blue_gain = B;
	}


	/** @note values sent by windows software is half the values of the GUI controls */
	void setOffsets(int8_t R, int8_t G, int8_t B)
	{
		red_offset = R;
		green_offset = G;
		blue_offset = B;
	}


	/**
	 * Sets capture resolution.
	 * @warning: I don't have a clue about what would happen when a resolution higher then the camera supports is selected.
	 */
	void setResolution(DLC300::resolutionEnum res)
	{
		int w = 0;
		int h = 0;

		switch(res)
		{
		case DLC300::RESOLUTION_640x480:
			possibly_random_byte = 0x51;
			w = 640;
			h = 480;
			break;
		case DLC300::RESOLUTION_800x600:
			possibly_random_byte = 0x25;
			w = 800;
			h = 600;
			break;
		case DLC300::RESOLUTION_1024x768:
			possibly_random_byte = 0x46;
			w = 1024;
			h = 768;
			break;
		case DLC300::RESOLUTION_1280x1024:
			possibly_random_byte = 0x30;
			w = 1280;
			h = 1024;
			break;
		case DLC300::RESOLUTION_1600x1200:
			possibly_random_byte = 0x53;
			w = 1600;
			h = 1200;
			break;
		case DLC300::RESOLUTION_2048x1536:
			possibly_random_byte = 0x12;
			w = 2048;
			h = 1536;
			break;
		case DLC300::RESOLUTION_UNDEFINED:
			assert(0);
		}

		row_size_msb = w >> 8;
		row_size_lsb = w & 0xFF;

		col_size_msb = h >> 8;
		col_size_lsb = h & 0xFF;
	}
};


/**
 * Opens a device with a VID:PID matching our camera
 */
int DLC300::openDevice()
{
	devh_ = libusb_open_device_with_vid_pid(NULL, VID, PID);

	if (!devh_) {
		printf("libusb_open_device_with_vid_pid failed\n");
		return -1;
	}

	if (libusb_claim_interface(devh_, 0) < 0)
	{
		printf("usb_claim_interface error\n");
		libusb_close(devh_);
		devh_ = 0;
		return -1;
	}

	return 0;
}

void DLC300::closeDevice()
{
	if (devh_)
	{
		libusb_release_interface(devh_, 0);
		libusb_close(devh_);
		devh_ = 0;
	}
}


int DLC300::getWidth()
{
	return w_;
}


int DLC300::getHeight()
{
	return h_;
}


int DLC300::setResolution(resolutionEnum res)
{
	res_ = res;

	switch(res)
	{
	case RESOLUTION_640x480:
		w_ = 640;
		h_ = 480;
		break;
	case RESOLUTION_800x600:
		w_ = 800;
		h_ = 600;
		break;
	case RESOLUTION_1024x768:
		w_ = 1024;
		h_ = 768;
		break;
	case RESOLUTION_1280x1024:
		w_ = 1280;
		h_ = 1024;
		break;
	case RESOLUTION_1600x1200:
		w_ = 1600;
		h_ = 1200;
		break;
	case RESOLUTION_2048x1536:
		w_ = 2048;
		h_ = 1536;
		break;
	case RESOLUTION_UNDEFINED:
		assert(0);
	}

	return 0;
}


/**
 * Sets camera exposure
 * @note valid range is 1 to 369
 */
int DLC300::setExposure(int exposure)
{
	if (exposure > 0 && exposure < 370)
	{
		exposure_ = exposure;
		return 0;
	} else {
		return -1;
	}
}


/**
 * Set red, green, and blue analog gains used by the sensor
 * @todo check limits
 */
void DLC300::setGains(int R, int G, int B)
{
	red_gain_ 	= R;
	green_gain_	= G;
	blue_gain_ 	= B;
}

/**
 * Set red, green, and blue analog offsets used by the sensor
 * @todo check limits
 */
void DLC300::setOffsets(int8_t R, int8_t G, int8_t B)
{
	red_offset_		= R;
	green_offset_	= G;
	blue_offset_	= B;
}


bool DLC300::isPresent()
{
	return devh_ != 0;
}

DLC300::DLC300() :
		devh_(0),
		w_(0),
		h_(0),
		res_(RESOLUTION_UNDEFINED),
		exposure_(75),
		red_gain_(0x2C),
		green_gain_(0x2C),
		blue_gain_(0x2C),
		red_offset_(0),
		green_offset_(0),
		blue_offset_(0),
		debug_level_(1)
{
	int r = libusb_init(NULL);

	if (r < 0)
	{
		printf("Could not initialize libusb!\n");
	}
	else
	{
		if (openDevice() < 0)
		{
			printf("Could not find and open a DLC300 camera\n");
			return;
		}
	}
}

DLC300::~DLC300()
{
	closeDevice();
	libusb_exit(NULL);
}


void DLC300::printData(unsigned char* data, int length, int requestLength)
{
	printf("got %d bytes (%d requested): \n", length, requestLength);
	for (int i = 0; i < length; i++)
	{
		printf("%02x ",int(data[i]));

		if ( (i>0) && ((i+1)%16)==0)
		{
			printf("\n");
		}
	}

	if (((length+1)%16)!=0)
	{
		printf("\n");
	}
}

/**
 * @param warn_when_this_differ Number of expected bytes in the transfer. When received length differ from this,
 *        print an error message. (you can set it to the default of -1 to decrease the amount of console output)
 */
void DLC300::read512(int warn_when_this_differ)
{
	unsigned char data[0x200];

	int numTransferred;

	/*int rc = */this->read(data, sizeof(data), numTransferred, warn_when_this_differ);

	if (debug_level_ > 1) {
		printData(data, numTransferred, sizeof(data));
	}
}

void DLC300::read256(int warn_when_this_differ)
{
	unsigned char data[0x100];

	int numTransferred;

	/*int rc = */this->read(data, sizeof(data), numTransferred, warn_when_this_differ);

	if (debug_level_ > 1) {
		printData(data, numTransferred, sizeof(data));
	}
}

void DLC300::read64(int warn_when_this_differ)
{
	unsigned char data[64];

	int numTransferred;

	/*int rc = */this->read(data, sizeof(data), numTransferred, warn_when_this_differ);

	if (debug_level_ > 1) {
		printData(data, numTransferred, sizeof(data));
	}
}



int DLC300::sendHeader()
{
DlcMsgStruct dlcMsg;

	memset(&dlcMsg, 0, sizeof(dlcMsg));
	dlcMsg.fillDefaults();
	dlcMsg.setResolution(res_);
	dlcMsg.setExposure(exposure_);
	dlcMsg.setGains(red_gain_, green_gain_, blue_gain_);

	int dummy;

	return this->write((unsigned char*)&dlcMsg, sizeof(dlcMsg), dummy);
}

int DLC300::write(unsigned char* data, int length, int& numTransfered)
{
	if (devh_ == 0)
	{
		printf("No valid USB device handle!\n");
		exit(1);
	}

	const int endpoint = 2;
	const int timeout = 4000; // ms

	int transferred = 0;

	int rc = libusb_bulk_transfer(devh_, endpoint, data, length, &transferred, timeout);

	numTransfered = transferred;

	bool any_error = (rc!=0) || (transferred != length);

	if (any_error || (debug_level_ > 1))
	{
		printf("OUT %d bytes\n", transferred);
	}

	if (rc!=0)
	{
		printf("Error 3: (%d)", rc);
		return -1;
	}

	if (transferred != length)
	{
		printf("Error 4: (%d)", transferred);
		return -1;
	}

	return 0;
}

/**
 * @param warn_when_this_differ Number of expected bytes in the transfer. When received length differ from this,
 *        print an error message. (you can set it to the default of -1 to decrease the amount of console output)
 */
int DLC300::read(unsigned char* data, int length, int& numTransfered, int warn_when_this_differ)
{
	if (devh_ == 0)
	{
		printf("No valid USB device handle!\n");
		exit(1);
	}

	const int endpoint = 0x86;
	const int timeout = 4000; // ms

	int transferred = 0;

	int rc = libusb_bulk_transfer(devh_, endpoint, data, length, &transferred, timeout);

	if (warn_when_this_differ == -1)
		warn_when_this_differ = length;


	if (transferred == warn_when_this_differ)
	{
		if (debug_level_ > 1) {
			printf("IN %d bytes\n", transferred);
		}
	}
	else
	{
		if (debug_level_ > 0) {
			printf("rc = %d, requested=%d, transferred = %d, diff = %d%s", rc, length, transferred, length-transferred, transferred==length?"\n":" EXPECTED MORE DATA!!!\n");
		}
	}

	numTransfered = transferred;

	return rc;
}

int DLC300::restartDevice()
{
	printf("Trying to restart LIBUSB. closing\n");
	closeDevice();
	sleep(1);
	printf("opening\n");
	return openDevice();
}

int DLC300::getFrame(unsigned char* buffer, int bufferSize)
{
	sendHeader();

	//read512(64); // We expect 64 bytes (but requested 512)
	int numTransferred = 0;

	//WARNING: this is a deviation from the requests sent by the windows software,
	//but it seems to work bette
	this->read(buffer, bufferSize, numTransferred);

	if (numTransferred != 64)
	{
		for (int i = 0; i < 20; i++)
			printf("We are not in sync!\n");
	}


	int rc = this->read(buffer, bufferSize, numTransferred); // expects all bytes

	if (rc == LIBUSB_ERROR_NO_DEVICE)
	{
		restartDevice();
		return -1;
	}

	if (res_ != RESOLUTION_800x600)
	{
		read512(256); // we expect 256 bytes (but requested 512)
	}

	return 0;
}

int DLC300::setDebugLevel(int newDebugLevel)
{
	int oldDebugLevel = debug_level_;

	debug_level_ = newDebugLevel;

	return oldDebugLevel;
}

