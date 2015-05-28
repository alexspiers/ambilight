/*
 * ScreenCapture.h
 *
 *  Created on: 24 May 2015
 *      Author: alex
 */

#include <X11/Xlib.h>

#include <vector>

#ifndef SCREENCAPTURE_H_
#define SCREENCAPTURE_H_

class ScreenCapture {
public:
	ScreenCapture();
	~ScreenCapture();
	void start();
	void stop();
private:
	int actual_width;
	int actual_height;

	int screen_width;
	int screen_height;

	int y_region_count;
	int x_region_count;

	int region_width;
	int region_height;

	Display *display;
	Window rootWindow;
	XWindowAttributes gwa;

	int region_count;
	vector<int> regions;

	int targetRefreshRate;
	int targetRefreshInterval;

	int blackBarDetectRate;
	int y_offset;

	void init();
	void calculateRegion();
	void captureScreen();
	void displayResult();

	void detectBlackBar();

	void captureScreenSHM();
};

#endif /* SCREENCAPTURE_H_ */
