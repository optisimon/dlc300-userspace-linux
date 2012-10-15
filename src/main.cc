/**
 * Test talking to some strange USB camera (VID:PID 1578:0076) using libusb.
 *
 * Builds fine on ubuntu 12.04 (SDL-1.2, SDL_gfx, and libusb-1.0)
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <memory>

#include "DLC300.h"
#include "SnapshotHelpers.h"
#include "GUIHelpers.h"


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


int main(int argc, char** argv)
{
	DLC300::resolutionEnum res = DLC300::RESOLUTION_2048x1536;

	int exposure = 73;
	int gain = 0x2C;
	bool should_view_not_save = true;

	char opt;
	while ((opt = getopt(argc, argv, "r:e:g:bh")) != -1)
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
				return -1;
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
				return -1;
			}
		}
		break;

		case 'g':
		{
			int g = atoi(optarg);
			if (g >= 0 && g <= 63)
			{
				gain = g;
			} else {
				printf("Expected gain within range 0-63 (was %d)\n", g);
				return -1;
			}
		}
		break;

		case 'b':
			should_view_not_save = false;
			break;

		case 'h':
			printf("Available flags:\n"
					"-r          Sets resolution of images. Valid resolutions are: 640x480, 800x600, 1024x768, 1280x1024, 1600x1200, 2048x1536\n"
					"            Note that the image is cropped whenever a resolution smaller then the sensors native resolution is requested.\n"
					"-e 1..370   Sets exposure\n"
					"-g 0..63    Sets gain (the same value is used for all channels)\n"
					"-b          \"Blind mode, no visual imaging. Instead it saves a few image and then quits\n"
					);
			break;

		default: /* '?' */
			printf("-h for command line arguments...\n");
			return -1;
		}
	}


	DLC300 myCam;

	if (myCam.isPresent())
	{
		myCam.setResolution(res);
		myCam.setExposure(exposure);
		myCam.setGains(gain, gain, gain);

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
				if (res != DLC300::RESOLUTION_800x600)
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
					myWindow->drawBayerAsRGB(buffer, w, h);

					input->refresh();

					if (input->shouldTakeSnapshot())
					{
						saveSnapshot(buffer, w, h, save_no);
					}

					if (input->shouldQuit())
					{
						break;
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
				return -1;
			}
		}
	}

	return 0;
}
