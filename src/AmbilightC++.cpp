//============================================================================
// Name        : AmbilightC++.cpp
// Author      : Alex Spiers
// Version     :
// Copyright   : Alex Spiers (c) 2015
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;

//Import the X11 libraries for screen capture
#include <X11/Xlib.h>

#include <CImg.h>
using namespace cimg_library;

#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <math.h>       /* round, floor, ceil, trunc */

Display *display = XOpenDisplay(NULL);
Window root = DefaultRootWindow(display);
XWindowAttributes gwa;

int width = 0;
int height = 0;

void setupX11() {
	XGetWindowAttributes(display, root, &gwa);
	width = gwa.width;
	height = gwa.height;

	cout << "Screen Width:  " << gwa.width << endl;
	cout << "Screen Height: " << gwa.height << endl;

	return;
}

void captureScreen() {
	XImage *image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);

	int red = 0;
	int green = 0;
	int blue = 0;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			unsigned long pixel = XGetPixel(image, x, y);
			red += (pixel & 0x00ff0000) >> 16;
			green += (pixel & 0x0000ff00) >> 8;
			blue += (pixel & 0x000000ff);
		}
	}

	red = red / (width * height);
	green = green / (width * height);
	blue = blue / (width * height);

	//cout << "Average Colour:  (" << red << "," << green << "," << blue << ")" << endl;

	XFree(image);
	return;
}

void captureLoop() {
	int targetFPS = 30;
	int targetInterval = 1000000 / targetFPS;
	int timeTaken = 0;
	clock_t startTime;
	clock_t endTime;
	int counter = 0;

	cout << "Time Interval: " << targetInterval << endl;

	clock_t totalStartTime = clock();

	while (counter < 100) {
		startTime = clock();
		captureScreen();
		endTime = clock();
		timeTaken += (endTime - startTime);

		++counter;
	}

	clock_t totalEndTime = clock();

	cout << "Target Total Runtime: " << (targetInterval * counter) << endl;
	cout << "Total  Total Runtime: " << (totalEndTime - totalStartTime) << endl;
	cout << "Total Runtime:        " << (timeTaken) << endl;
	cout << "Average Runtime:      " << ((float) (totalEndTime - totalStartTime) / 100) << endl;
	cout << "Average FPS:          " << ((float) 100 / ((totalEndTime - totalStartTime) / targetInterval) * 30) << endl;
}

void displayResults(unsigned char regions[][3]) {
	int screen_width = 960;
	int screen_height = 540;

	int repeatX = 10;
	int repeatY = 5;

	const unsigned char white[] = { 255, 0, 0 };

	int regionX = round((double) screen_width / repeatX);
	int regionY = round((double) screen_height / repeatY);

	CImg<unsigned char> visu(screen_width, screen_height, 1, 3, 0);
	CImgDisplay draw_disp(visu, "Intensity profile");
	draw_disp.move((1920 / 2) - (screen_width / 2), (1080 / 2) - (screen_height / 2));

	while (!draw_disp.is_closed()) {
		visu.fill(0);

		for (int x = 0; x < repeatX; ++x) {
			visu.draw_rectangle(x * regionX, 0, (x + 1) * regionX, regionY, regions[x], 1);
			visu.draw_rectangle(x * regionX, (screen_height - regionY), (x + 1) * regionX, (screen_height), regions[40 + x], 1);
		}

		for (int y = 1; y < (repeatY - 1); ++y) {
			visu.draw_rectangle(0, y * regionY, regionX, (y + 1) * regionY, regions[(10 * y)], 1);
			visu.draw_rectangle((screen_width - regionX), y * regionY, screen_width, (y + 1) * regionY, regions[10 * (y + 1) - 1], 1);
		}

		visu.draw_rectangle(0, 0, 0, 0, white, 1).display(draw_disp);
	}
}

void benchmarkTests1() {
	/*
	 * Test capturing the whole screen in one go and the processing
	 */
	clock_t startTime;
	clock_t endTime;

	startTime = clock();
	XImage *image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);

	int repeatX = round((double) width / 200);
	int regionX = width / repeatX;
	int repeatY = round(height / 200);
	int regionY = height / repeatY;

	cout << repeatX << " X's at " << regionX << endl;
	cout << repeatY << " Y's at " << regionY << endl;

	int regions[50][3] = { -1 };

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int index = floor((double) y / regionY) * 10 + floor((double) x / regionX);

			if (10 < index && index < 19) {

			} else if (20 < index && index < 29) {

			} else if (30 < index && index < 39) {

			} else {
				unsigned long pixel = XGetPixel(image, x, y);
				regions[index][0] += (pixel & 0x00ff0000) >> 16;
				regions[index][1] += (pixel & 0x0000ff00) >> 8;
				regions[index][2] += (pixel & 0x000000ff);
			}
		}
	}

	unsigned char regions2[50][3];

	int regionSize = (regionX * regionY);

	for (int i = 0; i < 50; ++i) {
		regions2[i][0] = regions[i][0] / regionSize;
		regions2[i][1] = regions[i][1] / regionSize;
		regions2[i][2] = regions[i][2] / regionSize;
		cout << "Region " << i << ": (" << regions[i][0] << "," << regions[i][1] << "," << regions[i][2] << ")" << endl;
	}

	endTime = clock();

	cout << "Whole Screen Capture: " << (endTime - startTime) << " FPS:" << (1000000 / (endTime - startTime)) << endl;

	XFree(image);

	displayResults(regions2);
}

void benchmarkTests2() {
	/*
	 * Test Capturing the screen in 4 sections, two large horizontal rectangles at the top and bottom, and two small vertical rectangles on either side
	 */
	clock_t startTime;
	clock_t endTime;

	int repeatX = round((double) width / 200);
	int regionX = width / repeatX;
	int repeatY = round(height / 200);
	int regionY = height / repeatY;

	startTime = clock();
	XImage *image2_1 = XGetImage(display, root, 0, 0, width, regionY, AllPlanes, ZPixmap);
	XImage *image2_2 = XGetImage(display, root, 0, (height - regionY), width, regionY, AllPlanes, ZPixmap);

	XImage *image2_3 = XGetImage(display, root, 0, regionY, regionX, (height - 2 * regionY), AllPlanes, ZPixmap);
	XImage *image2_4 = XGetImage(display, root, (width - regionX), regionY, regionX, (height - 2 * regionY), AllPlanes, ZPixmap);

	unsigned char regions[4][3];

	for (int y = 0; y < regionY; ++y) {
		for (int x = 0; x < width; ++x) {
			unsigned long pixel = XGetPixel(image2_1, x, y);
			regions[0][0] += (pixel & 0x00ff0000) >> 16;
			regions[0][1] += (pixel & 0x0000ff00) >> 8;
			regions[0][2] += (pixel & 0x000000ff);
		}
	}

	for (int y = 0; y < regionY; ++y) {
		for (int x = 0; x < width; ++x) {
			unsigned long pixel = XGetPixel(image2_2, x, y);
			regions[1][0] += (pixel & 0x00ff0000) >> 16;
			regions[1][1] += (pixel & 0x0000ff00) >> 8;
			regions[1][2] += (pixel & 0x000000ff);
		}
	}

	for (int y = 0; y < (height - 2 * regionY); ++y) {
		for (int x = 0; x < regionX; ++x) {
			unsigned long pixel = XGetPixel(image2_3, x, y);
			regions[2][0] += (pixel & 0x00ff0000) >> 16;
			regions[2][1] += (pixel & 0x0000ff00) >> 8;
			regions[2][2] += (pixel & 0x000000ff);
		}
	}

	for (int y = 0; y < (height - 2 * regionY); ++y) {
		for (int x = 0; x < regionX; ++x) {
			unsigned long pixel = XGetPixel(image2_4, x, y);
			regions[3][0] += (pixel & 0x00ff0000) >> 16;
			regions[3][1] += (pixel & 0x0000ff00) >> 8;
			regions[3][2] += (pixel & 0x000000ff);
		}
	}

	for (int y = 0; y < 4; ++y) {
		int size = 0;

		if (y == 0 || y == 2) {
			size = width * regionY;
		} else {
			size = regionX * (height - 2 * regionY);
		}

		for (int x = 0; x < 3; ++x) {
			regions[y][x] /= size;
		}
	}

	endTime = clock();

	cout << "4 Sections Capture: " << (endTime - startTime) << " FPS:" << (1000000 / (endTime - startTime)) << endl;

	XFree(image2_1);
	XFree(image2_2);
	XFree(image2_3);
	XFree(image2_4);

}

void benchmarkTests3() {
	/*
	 * Test getting the screen each section at a time
	 */
	clock_t startTime;
	clock_t endTime;
	int repeatX = round((double) width / 200);
	int regionX = width / repeatX;
	int repeatY = round(height / 200);
	int regionY = height / repeatY;

	for (int y = 0; y < repeatY; y++) {
		for (int x = 0; x < repeatX; x++) {

		}
	}

}
//
//int main(){
//	setupX11();
//	benchmarkTests1();
//}
