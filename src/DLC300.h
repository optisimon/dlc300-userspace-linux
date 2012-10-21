/**
 * Test talking to some strange USB camera using libusb.
 *
 * Original author Simon Gustafsson (www.simong.eu)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#ifndef DLC300_H_
#define DLC300_H_

#include <libusb-1.0/libusb.h>

/**
 * This class exposes all settings available in the windows program.
 */
class DLC300 {
public:

	enum resolutionEnum {
		RESOLUTION_MIN = 0,
		RESOLUTION_640x480 = 0,
		RESOLUTION_800x600,
		RESOLUTION_1024x768,
		RESOLUTION_1280x1024,
		RESOLUTION_1600x1200,
		RESOLUTION_2048x1536,
		RESOLUTION_MAX = RESOLUTION_2048x1536,

		RESOLUTION_UNDEFINED = -1
	};

	enum {
		VID = 0x1578, ///< Vendor ID of this USB device
		PID = 0x0076  ///< Product ID of this USB device
	};

private:

	libusb_device_handle *devh_;

	int w_;
	int h_;

	resolutionEnum res_;
	uint16_t exposure_;
	int red_gain_;
	int green_gain_;
	int blue_gain_;
	int red_offset_;
	int green_offset_;
	int blue_offset_;

	int debug_level_;

	int openDevice();
	void closeDevice();

	int restartDevice();

public:

	DLC300();
	~DLC300();

	int getWidth();
	int getHeight();

	int setResolution(resolutionEnum res);
	resolutionEnum getResolution() { return res_; }
	int setExposure(int exposure);

	void setGains(int R, int G, int B);
	void setOffsets(int8_t R, int8_t G, int8_t B);

	bool isPresent(); ///< @return true if camera present when the class was constructed

	void printData(unsigned char* data, int length, int requestLength);

	void read64(int warn_when_this_differ = -1);
	void read256(int warn_when_this_differ = -1);
	void read512(int warn_when_this_differ = -1);

	int sendHeader();

	int write(unsigned char* data, int length, int& numTransfered);

	int read(unsigned char* data, int length, int& numTransfered, int warn_when_this_differ = -1);

	int getFrame(unsigned char* buffer, int bufferSize);

	int setDebugLevel(int newDebugLevel);
};


#endif /* DLC300_H_ */
