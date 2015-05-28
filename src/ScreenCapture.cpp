/*
 * ScreenCapture.cpp
 *
 *  Created on: 24 May 2015
 *      Author: alex
 */
#include <iostream>
using namespace std;

#include <string>

#include <ScreenCapture.h>

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>

#include <CImg.h>
using namespace cimg_library;

#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <math.h>       /* round, floor, ceil, trunc */
#include <vector>

int frameCount = 0;
long frameTime = 0;

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

ScreenCapture::ScreenCapture() {
    init();

    x_region_count = 20;
    y_region_count = 10;
    screen_width = actual_width;
    screen_height = actual_height;

    targetRefreshRate = 30;
    blackBarDetectRate = 3;

    detectBlackBar();

    clock_t start = clock();
    calculateRegion();
    clock_t end = clock();

    cout << "FPS: " << ((end - start)) << "microseconds" << endl;

    //captureScreenSHM();

    cout << "Screen size is " << screen_width << " by " << screen_height << endl;
    cout << "Horizontal Regions: " << y_region_count << endl;
    cout << "Vertical Regions: " << x_region_count << endl;
    cout << "Regions are " << region_width << " by " << region_height << endl;
    cout << "Number of Regions: " << ((2 * x_region_count) + (y_region_count - 2) * 2) << endl;

    displayResult();

    //detectBlackBar();
}

ScreenCapture::~ScreenCapture() {

}

void ScreenCapture::init() {
    display = XOpenDisplay(NULL);
    if (!display) {
        cout << "Display is NULL. Cannot continue exiting" << endl;
        throw "XOpenDisplay returned NULL";
    }

    rootWindow = DefaultRootWindow(display);
    if (!rootWindow) {
        cout << "Root Window is NULL. Cannot continue exiting" << endl;
        throw "DefaultRootWindow returned NULL";
    }

    int gwaResult = XGetWindowAttributes(display, rootWindow, &gwa);
    if (gwaResult == 0) {
        cout << "Window Attributes are NULL. Cannot continue exiting" << endl;
        throw "XGetWindowAttributes returned NULL";
    }

    int ignore, major, minor;
    Bool pixmaps;

    /* Check for the XShm extension */
    if (XQueryExtension(display, "MIT-SHM", &ignore, &ignore, &ignore)) {
        if (XShmQueryVersion(display, &major, &minor, &pixmaps) == True) {
            printf("XShm extention version %d.%d %s shared pixmaps\n", major, minor, (pixmaps == True) ? "with" : "without");
        }
    }

    actual_width = gwa.width;
    actual_height = gwa.height;
}

void ScreenCapture::calculateRegion() {
    region_width = round((double) screen_width / x_region_count);
    region_height = round((double) screen_height / y_region_count);

    targetRefreshInterval = 1000000 / targetRefreshRate;

    region_count = ((2 * x_region_count) + (y_region_count - 2) * 2);

    regions = vector<int>(region_count * 3);
}

void ScreenCapture::captureScreenSHM() {
    XShmSegmentInfo shminfo;
    int x = 1920, y = 1080;

    Screen *screen = XDefaultScreenOfDisplay(display);

    XImage *image1;
    XImage *image2;
    XImage *image3;

    clock_t start1 = clock();
    image1 = XShmCreateImage(display, DefaultVisualOfScreen(screen), DefaultDepthOfScreen(screen), ZPixmap, NULL, &shminfo, x, y);
    clock_t end1 = clock();

    XShmGetImage(display, rootWindow, image3, 1920, 1080, AllPlanes);

    shminfo.shmid = shmget(IPC_PRIVATE, image1->bytes_per_line * image1->height, IPC_CREAT | 0777);
    if (shminfo.shmid < 0) {
        cout << "shmid Error" << endl;
        throw "shmid";
    }

    shminfo.shmaddr = image3->data = (char *) shmat(shminfo.shmid, 0, 0);
    if (shminfo.shmaddr == (char *) -1) {
        cout << "shmaddr Error" << endl;
        throw "shmaddr";
    }

    shminfo.readOnly = False;
    XShmAttach(display, &shminfo);

    cout << image1->bytes_per_line * image1->height << endl;

    cout << "Width:  " << image1->width << endl;
    cout << "Height: " << image1->height << endl;

    cout << image3->data[0] << endl;

//    for (int y = 0; y < image1->height; ++y) {
//        for (int x = 0; x < image1->width; ++x) {
//            image1->data[0]
//        }
//    }

    cout << "Time: " << (end1 - start1) << endl;

}

void ScreenCapture::captureScreen() {
    XImage *topImage = XGetImage(display, rootWindow, 0, y_offset, screen_width, region_height, AllPlanes, ZPixmap);
    XImage *leftImage = XGetImage(display, rootWindow, 0, y_offset + region_height, region_width, (screen_height - 2 * region_height), AllPlanes, ZPixmap);
    XImage *rightImage = XGetImage(display, rootWindow, (screen_width - region_width), y_offset + region_height, region_width, (screen_height - 2 * region_height), AllPlanes, ZPixmap);
    XImage *bottomImage = XGetImage(display, rootWindow, 0, (screen_height - region_height) + y_offset, screen_width, region_height, AllPlanes, ZPixmap);

    int regionCount = region_width * region_height;

    /*
     * Top image pixel processing
     */
    for (int y = 0; y < region_height; ++y) {
        for (int x = 0; x < screen_width; ++x) {
            int regionIndex = (x / region_width) * 3;
            unsigned long pixel = XGetPixel(topImage, x, y);
            regions[regionIndex + 0] += (pixel & 0x00ff0000) >> 16;
            regions[regionIndex + 1] += (pixel & 0x0000ff00) >> 8;
            regions[regionIndex + 2] += (pixel & 0x000000ff);
        }
    }

    /*
     * Left hand side pixel processing
     */
    for (int y = 0; y < (y_region_count - 2) * region_height; ++y) {
        for (int x = 0; x < region_width; ++x) {
            int regionIndex = (((y / region_height) * 2) + x_region_count) * 3;
            unsigned long pixel = XGetPixel(leftImage, x, y);
            regions[regionIndex + 0] += (pixel & 0x00ff0000) >> 16;
            regions[regionIndex + 1] += (pixel & 0x0000ff00) >> 8;
            regions[regionIndex + 2] += (pixel & 0x000000ff);
        }
    }

    /*
     * Right hand side pixel processing
     */
    for (int y = 0; y < (y_region_count - 2) * region_height; ++y) {
        for (int x = 0; x < region_width; ++x) {
            int regionIndex = (((y / region_height) * 2) + (x_region_count + 1)) * 3;
            unsigned long pixel = XGetPixel(rightImage, x, y);
            regions[regionIndex + 0] += (pixel & 0x00ff0000) >> 16;
            regions[regionIndex + 1] += (pixel & 0x0000ff00) >> 8;
            regions[regionIndex + 2] += (pixel & 0x000000ff);
        }
    }

    /*
     * Bottom image pixel processing
     */
    for (int y = 0; y < region_height; ++y) {
        for (int x = 0; x < screen_width; ++x) {
            int regionIndex = ((x / region_width + (region_count - x_region_count)) * 3);
            unsigned long pixel = XGetPixel(bottomImage, x, y);
            regions[regionIndex + 0] += (pixel & 0x00ff0000) >> 16;
            regions[regionIndex + 1] += (pixel & 0x0000ff00) >> 8;
            regions[regionIndex + 2] += (pixel & 0x000000ff);
        }
    }

    /*
     * Average the colour channel
     */
    for (int i = 0; i < region_count * 3; ++i) {
        regions[i] /= regionCount;
    }

    XDestroyImage(topImage);
    XDestroyImage(leftImage);
    XDestroyImage(rightImage);
    XDestroyImage(bottomImage);
}

void ScreenCapture::displayResult() {

    int window_width = actual_width / 2;
    int window_height = actual_height / 2;

    int window_region_width = round((double) window_width / x_region_count);
    int window_region_height = round((double) window_height / y_region_count);

    const unsigned char white[] = { 255, 255, 255 };

    CImg<unsigned char> visu(window_width, window_height, 1, 3, 0);
    CImgDisplay draw_disp(visu, "Intensity profile");
    draw_disp.move((actual_width / 2) - (window_width / 2), (actual_height / 2) - (window_height / 2));

    while (!draw_disp.is_closed()) {
        visu.fill(0);

        if (frameCount % (blackBarDetectRate * targetRefreshRate) == 0) {
            detectBlackBar();
            calculateRegion();
        }

        clock_t startTime = clock();
        captureScreen();
        clock_t endTime = clock();

        frameTime += (endTime - startTime);
        frameCount++;
        if ((endTime - startTime) < targetRefreshInterval) {
            unsigned int sleepTime = targetRefreshInterval - (endTime - startTime);
            usleep(sleepTime);
        }

        for (int i = 0; i < x_region_count; ++i) {
            visu.draw_rectangle(window_region_width * i, 0, window_region_width * (i + 1), window_region_height, &regions[i * 3], 1);
            visu.draw_rectangle(window_region_width * i, (window_height - window_region_height), window_region_width * (i + 1), window_height, &regions[((region_count - x_region_count) + i) * 3], 1);
        }

        for (int i = 0; i < (y_region_count - 2); ++i) {
            visu.draw_rectangle(0, window_region_height + (i * window_region_height), window_region_width, window_region_height + ((i + 1) * window_region_height),
                    &regions[(x_region_count + (i * 2)) * 3], 1);
            visu.draw_rectangle((window_width - window_region_width), window_region_height + (i * window_region_height), window_width, window_region_height + ((i + 1) * window_region_height),
                    &regions[(x_region_count + 1 + (i * 2)) * 3], 1);
        }

        std::string frameRateString = std::to_string(1000000 / (frameTime / frameCount));
        char const *frameRateChar = frameRateString.c_str();

        std::string resolutionString = std::to_string(screen_height) + "x" + std::to_string(screen_width);
        char const *resolutionChar = resolutionString.c_str();

        visu.draw_text(window_width / 2, window_height / 2, frameRateChar, white);
        visu.draw_text(window_width / 2, (window_height / 2) + 50, resolutionChar, white);

        visu.display(draw_disp);

        //cout << "Memory Usage: " << getValue() << endl;
    }

    draw_disp.close();
}

void ScreenCapture::detectBlackBar() {
    clock_t s = clock();
    XImage *topImage = XGetImage(display, rootWindow, 0, 0, actual_width, actual_height, AllPlanes, ZPixmap);

    bool continueSearch = true;
    y_offset = 0;

    for (int y = 0; y < actual_height && continueSearch; ++y) {
        for (int x = 0; x < actual_width && continueSearch; x += 2) {
            unsigned long top_pixel = XGetPixel(topImage, x, (actual_height - 1) - y);
            unsigned char top_red = (top_pixel & 0x00ff0000) >> 16;
            unsigned char top_green = (top_pixel & 0x0000ff00) >> 8;
            unsigned char top_blue = (top_pixel & 0x000000ff);

            unsigned long bottom_pixel = XGetPixel(topImage, x, y);
            unsigned char bottom_red = (bottom_pixel & 0x00ff0000) >> 16;
            unsigned char bottom_green = (bottom_pixel & 0x0000ff00) >> 8;
            unsigned char bottom_blue = (bottom_pixel & 0x000000ff);

            if (top_red < 10 && top_green < 10 && top_blue < 10 && bottom_red < 10 && bottom_green < 10 && bottom_blue < 10) {
                if (y > y_offset) {
                    y_offset = y;
                }
            } else {
                continueSearch = false;
            }
        }
    }

    XDestroyImage(topImage);

    screen_height = actual_height - 2 * y_offset;

    cout << "The resultant y_offset is " << y_offset << endl;
    cout << "Time taken: " << (clock() - s) << endl;
}

int main() {
    try {
        ScreenCapture();
    } catch (char *message) {
        cout << "Error: " << message << endl;
        return 1;
    }

    return 0;
}
