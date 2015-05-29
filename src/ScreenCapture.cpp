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

#include <fstream>

ScreenCapture::ScreenCapture() {
    init();

    loadConfig();

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

void ScreenCapture::loadConfig() {
    const std::string fileName = "config.amb";
    ifstream configFile(fileName.c_str());

    if (configFile.good()) {
        //Load from configuration file
        string line;

        while (std::getline(configFile, line)) {
            string prefix = line.substr(0, 2);
            if (prefix != "//" && line != "") {
                if(line.find("Xcount=") == 0){
                    x_region_count = atoi(line.substr(7).c_str());
                } else if(line.find("Ycount=") == 0){
                    y_region_count = atoi(line.substr(7).c_str());
                } else if(line.find("refreshRate=") == 0){
                    targetRefreshRate = atoi(line.substr(12).c_str());
                } else if(line.find("blackBarRate=") == 0){
                    blackBarDetectRate = atoi(line.substr(13).c_str());
                } else if(line.find("blackThreshold=") == 0){
                    blackThreshold = atoi(line.substr(15).c_str());
                }
            }

        }

    } else {
        //Create configuration file
        ofstream file;
        file.open(fileName);
        file << "// Example Configuration File For Ambilight Clone" << endl;
        file << "// Specifies the number of LEDs that go across the top and the bottom of the screen" << endl;
        file << "Xcount=20" << endl;
        file << endl;
        file << "// Specifies the number of LEDs that go along the vertical edges of the screen" << endl;
        file << "Ycount=10" << endl;
        file << endl;
        file << "// Specifies the target number of frames to capture the screen at" << endl;
        file << "refreshRate=30" << endl;
        file << endl;
        file << "// Specifies the number of seconds between detecting black bars" << endl;
        file << "blackBarRate=3" << endl;
        file << endl;
        file << "// Specifies the maximum level that the colour of all channels of a pixel must have to be regarded as black" << endl;
        file << "blackThreshold=10" << endl;
        file.close();

    }

    //Load the default values
//    x_region_count = 20;
//    y_region_count = 10;
//
//    targetRefreshRate = 30;
//    blackBarDetectRate = 3;
//    blackThreshold = 10;
}

void ScreenCapture::calculateRegion() {
    screen_width = actual_width;
    screen_height = actual_height;

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
     * Top and bottom image pixel processing
     */
    for (int y = 0; y < region_height; ++y) {
        for (int x = 0; x < screen_width; ++x) {
            int topRegionIndex = (x / region_width) * 3;
            unsigned long topPixel = XGetPixel(topImage, x, y);
            regions[topRegionIndex + 0] += (topPixel & 0x00ff0000) >> 16;
            regions[topRegionIndex + 1] += (topPixel & 0x0000ff00) >> 8;
            regions[topRegionIndex + 2] += (topPixel & 0x000000ff);

            int bottomRegionIndex = ((x / region_width + (region_count - x_region_count)) * 3);
            unsigned long bottomPixel = XGetPixel(bottomImage, x, y);
            regions[bottomRegionIndex + 0] += (bottomPixel & 0x00ff0000) >> 16;
            regions[bottomRegionIndex + 1] += (bottomPixel & 0x0000ff00) >> 8;
            regions[bottomRegionIndex + 2] += (bottomPixel & 0x000000ff);
        }
    }

    /*
     * Side pixel processing
     */
    for (int y = 0; y < (y_region_count - 2) * region_height; ++y) {
        for (int x = 0; x < region_width; ++x) {
            int rightRegionIndex = (((y / region_height) * 2) + (x_region_count + 1)) * 3;
            unsigned long rightPixel = XGetPixel(rightImage, x, y);
            regions[rightRegionIndex + 0] += (rightPixel & 0x00ff0000) >> 16;
            regions[rightRegionIndex + 1] += (rightPixel & 0x0000ff00) >> 8;
            regions[rightRegionIndex + 2] += (rightPixel & 0x000000ff);

            int leftRegionIndex = (((y / region_height) * 2) + x_region_count) * 3;
            unsigned long leftPixel = XGetPixel(leftImage, x, y);
            regions[leftRegionIndex + 0] += (leftPixel & 0x00ff0000) >> 16;
            regions[leftRegionIndex + 1] += (leftPixel & 0x0000ff00) >> 8;
            regions[leftRegionIndex + 2] += (leftPixel & 0x000000ff);
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
            //usleep(sleepTime);
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

            if (top_red < blackThreshold && top_green < blackThreshold && top_blue < blackThreshold && bottom_red < blackThreshold && bottom_green < blackThreshold && bottom_blue < blackThreshold) {
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
