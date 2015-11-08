/**
 * Test talking to some strange USB camera (VID:PID 1578:0076) using libusb.
 *
 * Builds fine on ubuntu 12.04 (SDL-1.2, SDL_gfx, and libusb-1.0)
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012-2015 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <memory>

#include "AutoWhiteBalance.h"
#include "DLC300.h"
#include "SnapshotHelpers.h"
#include "GUIHelpers.h"


template <class T>
T coerce(const T& value, const T& min, const T& max)
{
	return std::max(std::min(value, max), min);
}


void saveSnapshot(unsigned char* img, int w, int h, int& saveIndex)
{
	saveIndex = SnapshotHelpers::getNextUnusedIndex(saveIndex);

	if (saveIndex >= 0)
	{
		SnapshotHelpers::saveRAWSnapshot(img, w, h, saveIndex);
		SnapshotHelpers::savePPMSnapshot(img, w, h, saveIndex);
		SnapshotHelpers::savePPMSnapshot_demosaic_linear(img, w, h, saveIndex);

		saveIndex++;
	}
	else
	{
		printf("Could not save snapshot. Image numbering exhausted\n"
				"(i.e. all non-negative integers have been used up)\n");
	}
}


void handleExposureAdjustment(int exposureDirection, int& exposure, bool should_be_verbose)
{
	if (exposureDirection)
	{
		if (should_be_verbose) {
			printf("exposureDirection=%d\n", exposureDirection);
		}
		if (exposureDirection > 0)
		{
			exposure = coerce(exposure + 5, 1, 370);
		}
		else if (exposureDirection < 0)
		{
			exposure = coerce(exposure - 5, 1, 370);
		}
		printf("exposure=%d\n", exposure);
	}
}


/**
 * This summation is very specific to the bayer pattern generated by our camera
 */
void calculateWhitebalanceRegionSums(unsigned char* buffer, unsigned bufferStride,
		int top, int bottom, int left, int right,
		long & sum_R, long & sum_G, long & sum_B)
{
	sum_R = 0;
	sum_G = 0;
	sum_B = 0;

	for (int y = top / 2; y < bottom / 2; y++)
	{
		for (int x = left / 2; x < right / 2; x++)
		{
			uint8_t R  = buffer[(y * 2) * bufferStride     + x * 2 + 0];
			uint8_t G1 = buffer[(y * 2) * bufferStride     + x * 2 + 1];
			uint8_t G2 = buffer[(y * 2 + 1) * bufferStride + x * 2 + 0];
			uint8_t B  = buffer[(y * 2 + 1) * bufferStride + x * 2 + 1];

			sum_R += R;
			sum_G += (G1 + G2) / 2;
			sum_B += B;
		}
	}

}


int main(int argc, char** argv)
{
	DLC300::resolutionEnum res = DLC300::RESOLUTION_2048x1536;

	int exposure = 73;
	int gain = 0x2C;
	int gain_red = gain;
	int gain_green = gain;
	int gain_blue = gain;

	bool should_view_not_save = true;
	bool should_be_verbose = false;

	bool should_center_lower_resolution = true;

	char opt;
	while ((opt = getopt(argc, argv, "r:e:g:bchv")) != -1)
	{
		switch (opt)
		{
		case 'r':
			if (strcmp(optarg, "640x480") == 0) {
				res = DLC300::RESOLUTION_640x480;
			} else if (strcmp(optarg, "800x600") == 0) {
				res = DLC300::RESOLUTION_800x600;
			} else if (strcmp(optarg, "1024x768") == 0) {
				res = DLC300::RESOLUTION_1024x768;
			} else if (strcmp(optarg, "1280x1024") == 0) {
				res = DLC300::RESOLUTION_1280x1024;
			} else if (strcmp(optarg, "1600x1200") == 0) {
				res = DLC300::RESOLUTION_1600x1200;
			} else if (strcmp(optarg, "2048x1536") == 0) {
				res = DLC300::RESOLUTION_2048x1536;
			} else {
				printf("UNKNOWN RESOLUTION\n");
				return 1;
			}

			break;

		case 'e':
		{
			int n = atoi(optarg);
			if (n > 0 && n <= 370)
			{
				exposure = n;
			} else {
				printf("Expected exposure within range 1-370\n");
				return 1;
			}
		}
		break;

		case 'g':
		{
			int g = atoi(optarg);
			if (g >= 0 && g <= 63)
			{
				gain = g;
				gain_red = gain;
				gain_green = gain;
				gain_blue = gain;
			} else {
				printf("Expected gain within range 0-63 (was %d)\n", g);
				return 1;
			}
		}
		break;

		case 'b':
			should_view_not_save = false;
			break;
			
		case 'c':
			should_center_lower_resolution = false;
			break;

		case 'v':
			should_be_verbose = true;
			break;

		case 'h':
			printf("Available flags:\n"
					"-r         Sets resolution of images. Valid resolutions are:\n"
					"           640x480, 800x600, 1024x768, 1280x1024, 1600x1200, 2048x1536.\n"
					"           Note that the image is cropped whenever a resolution smaller then\n"
					"           the sensors native resolution is requested\n"
					"-e 1..370  Sets exposure\n"
					"-g 0..63   Sets gain (the same value is used for all channels)\n"
					"-b         \"Blind mode\", no visual imaging. It saves a few image before exiting\n"
					"-c         DO NOT Center cropped area in low resolution modes (possibly needed for compatibility with other cameras)\n"
					"-v         Verbose debug output (for developers)\n"
					"\n"
					"\n"
					"If not running in \"Blind mode\", one can use\n"
					"+/- to change exposure\n"
					"F1  to take a snapshot\n"
					"F2  to start/stopping taking snapshots continuously\n"
					"F3  Set the white balance (something grey should be in the center of the view)\n"
					"F4  to cycle the cameras resolution\n"
					"ESC to quit the program\n"
					);
			return 0;
			break;

		default: /* '?' */
			printf("-h for command line arguments...\n");
			return 1;
		}
	}

	DLC300 myCam;

	if (should_be_verbose) {
		myCam.setDebugLevel(10);
	} else {
		myCam.setDebugLevel(0);
	}
	
	myCam.setShouldCenterLowResolution(should_center_lower_resolution);

	if (myCam.isPresent())
	{
		myCam.setResolution(res);
		myCam.setExposure(exposure);
		myCam.setGains(gain_red, gain_green, gain_blue);

		AutoWhiteBalance whiteBalbance(gain_red, gain_green, gain_blue);

		unsigned char buffer[2048*1536];

		std::auto_ptr<SDLWindow> myWindow(0);

		std::auto_ptr<SDLEventHandler> input(0);

		if (should_view_not_save)
		{
			myWindow.reset(new SDLWindow());
			input.reset(new SDLEventHandler());
		}

		int save_no = SnapshotHelpers::getNextUnusedIndex();

		for (int i = 0; should_view_not_save || (i < 10); i++)
		{
			unsigned w = myCam.getWidth();
			unsigned h = myCam.getHeight();

			if (w*h <= sizeof(buffer))
			{
				if (myCam.getResolution() != DLC300::RESOLUTION_800x600)
				{
					myCam.getFrame(buffer, w*h);
				}
				else
				{
					//
					// For unknown reasons, the 800x600 pixel mode had to be handled differently
					//
					assert(sizeof(buffer) >= w*h + 256);
					myCam.getFrame(buffer, w*h + 256);
				}

				if (should_view_not_save)
				{
					input->refresh();

					// Should set white balance?
					if (input->shouldSetGreyPoint() && ! whiteBalbance.isRunning())
					{
						whiteBalbance.start();
					}

					// Should change exposure?
					int exposureDirection = input->getExposureDirection();
					handleExposureAdjustment(exposureDirection, exposure, should_be_verbose);

					myWindow->setShowWhitebalanceRegion(whiteBalbance.isRunning());
					myWindow->drawBayerAsRGB(buffer, w, h);

					if (whiteBalbance.isRunning())
					{
						int left, right, top, bottom;
						myWindow->calculateWhitebalanceRegion(w, h, left, top, right, bottom);

						long sum_R, sum_G, sum_B, mean;

						calculateWhitebalanceRegionSums(buffer, w, top, bottom, left, right, sum_R, sum_G, sum_B);

						mean = (sum_R + sum_G + sum_B) / 3;

						printf("R=%ld, G=%ld, B=%ld, mean=%ld\n", sum_R, sum_G, sum_B, mean);

						whiteBalbance.processCurrentSums(sum_R, sum_G, sum_B);
						whiteBalbance.getCurrentGains(gain_red, gain_green, gain_blue);

						printf("gain_R=%d, gain_G=%d, gain_B=%d\n", gain_red, gain_green, gain_blue);

						//TODO: Maybe we should automatically adjust exposure as well, so the user can't force white balancing to fail...
					}

					myCam.setGains(gain_red, gain_green, gain_blue);
					myCam.setExposure(exposure);

					if (input->shouldTakeSnapshot())
					{
						saveSnapshot(buffer, w, h, save_no);
					}

					if (input->shouldQuit())
					{
						break;
					}

					if (input->shouldCycleResolution())
					{
						int nextMode = myCam.getResolution() + 1;

						if (nextMode > DLC300::RESOLUTION_MAX) {
							nextMode = DLC300::RESOLUTION_MIN;
						}

						myCam.setResolution(DLC300::resolutionEnum(nextMode));
						myWindow->clear();
					}

				}
				else
				{
					saveSnapshot(buffer, w, h, save_no);
				}
			}
			else
			{
				printf("Something is wrong\n");
				return 1;
			}
		}
	}

	return 0;
}
