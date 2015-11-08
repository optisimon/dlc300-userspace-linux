/**
 * Test talking to some strange USB camera using libusb.
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#ifndef SNAPSHOTHELPERS_H_
#define SNAPSHOTHELPERS_H_

#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace SnapshotHelpers {


std::string buildPPMSnapshotFilename(int index)
{
	char filename[50];
	snprintf(filename, sizeof(filename), "combined_%05d.ppm", index);
	return filename;
}


std::string buildPPMSnapshot_demosaic_linearFilename(int index)
{
	char filename[50];
	snprintf(filename, sizeof(filename), "combined_demosaic_linear_%05d.ppm", index);
	return filename;
}


std::string buildRAWSnapshotFilename(int index)
{
	char filename[20];
	snprintf(filename, sizeof(filename), "raw_chunk_%05d.raw", index);
	return filename;
}


bool fileExists(const std::string& filename)
{
	struct stat buf;

	return stat(filename.c_str(), &buf) != -1;
}


int getNextUnusedIndex(int startIndex = 0)
{
	bool indexIsOccupied = true;
	int index;

	for (index = startIndex; index >= 0 ; index++)
	{
		indexIsOccupied =
				fileExists( buildRAWSnapshotFilename(index) ) |
				fileExists( buildPPMSnapshotFilename(index) ) |
				fileExists( buildPPMSnapshot_demosaic_linearFilename(index) );

		if (!indexIsOccupied) {
			return index;
		}
	}

	return -1;
}


void savePPMSnapshot(unsigned char* img, int w, int h, int index)
{
	std::string filename = buildPPMSnapshotFilename(index);
	std::ofstream ofs(filename.c_str());
	printf("\n=====[Saving frame as %s]=====\n", filename.c_str());
	ofs << "P6\n" << w/2 << " " << h/2 << " 255\n";

	for (int y = 0; y < h/2; y++)
	{
		for (int x = 0; x < w/2; x++)
		{
			uint8_t R  = img[(y*2  )*w + x*2 + 0];
			uint8_t G1 = img[(y*2  )*w + x*2 + 1];
			uint8_t G2 = img[(y*2+1)*w + x*2 + 0];
			uint8_t B  = img[(y*2+1)*w + x*2 + 1];
			uint8_t G  = (G1+G2)/2;
			const uint8_t pixel[3] = {R, G, B};
			ofs.write(reinterpret_cast<const char*>(pixel), 3);
		}
	}
}


static uint8_t getMeanPlus(unsigned char* img, int w, int h, int x, int y)
{
	return (img[(y-1)*w + x] + img[(y+1)*w + x] + img[y*w + x+1] + img[y*w + x-1] + 2) / 4;
}


static uint8_t getMeanCross(unsigned char* img, int w, int h, int x, int y)
{
	return (img[(y-1)*w + (x-1)] + img[(y+1)*w + (x-1)] + img[(y-1)*w + x+1] + img[(y+1)*w + x+1] + 2) / 4;
}


void savePPMSnapshot_demosaic_linear(unsigned char* img, int w, int h, int index)
{
#warning "This function won't demosaic the outermost pixels, so image saved is (w-2) x (h-2) pixels"

	std::string filename = buildPPMSnapshot_demosaic_linearFilename(index);

	std::ofstream ofs(filename.c_str());
	printf("\n=====[Saving frame as %s]=====\n", filename.c_str());
	ofs << "P6\n" << w-2 << " " << h-2 << " 255\n";

	for (int y = 1; y < (h-1); y++)
	{
		for (int x = 1; x < (w-1); x++)
		{
			uint8_t R = 0;
			uint8_t G = 0;
			uint8_t B = 0;

			switch ((x&1) + 2*(y&1))
			{
			case 0:
				// RED pixel
				R = img[y*w+x];
				G = getMeanPlus(img, w, h, x, y);
				B = getMeanCross(img, w, h, x, y);
				break;
			case 1:
				// Green1 pixel
				R = (img[y*w + x-1] + img[y*w + x+1] + 1)/2;
				G = img[y*w+x];
				B = (img[(y-1)*w + x] + img[(y+1)*w + x] + 1)/2;
				break;
			case 2:
				// Green2 pixel
				R = (img[(y-1)*w + x] + img[(y+1)*w + x] + 1)/2;
				G = img[y*w+x];
				B = (img[y*w + x-1] + img[y*w + x+1] + 1)/2;
				break;
			case 3:
				// Blue pixel
				R = getMeanCross(img, w, h, x, y);
				G = getMeanPlus(img, w, h, x, y);
				B = img[y*w+x];
				break;
			}

			const uint8_t pixel[3] = {R, G, B};
			ofs.write(reinterpret_cast<const char*>(pixel), 3);
		}
	}
}


void saveRAWSnapshot(unsigned char* img, int w, int h, int index)
{
	std::string filename = buildRAWSnapshotFilename(index);

	std::ofstream ofs(filename.c_str());
	printf("\n=====[Saving frame as %s]=====\n", filename.c_str());
	ofs.write((char*) img, w*h);
}

} //SnapshotHelpers


#endif /* SNAPSHOTHELPERS_H_ */
