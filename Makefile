# This target will compile all of the files
all: screencapture	
screencapture:
		g++ -Isrc src/ScreenCapture.cpp -o ScreenCapture -lX11 -lXext -lpthread -std=c++11

clean:
	rm -rf *o ScreenCapture
