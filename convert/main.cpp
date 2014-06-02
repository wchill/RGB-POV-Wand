#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <iostream>
#include "png.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

int main(int argc, char *argv[]) {
	if(argc < 2 || argc > 3) {
		cout << "usage: convert <image> [serial port]" << endl;
		cout << "if serial port is unspecified default is /dev/ttyACM0" << endl;
		return 0;
	}

	string port("/dev/ttyACM0");
	if(argc == 3) {
		port = string(argv[2]);
		cout << "using serial port " << port << endl;
	}

	string filename(argv[1]);
	PNG image(filename);
	if(image.width() < 1 || image.width() > 127 || image.height() != 8) {
		cerr << "image dimensions out of bounds" << endl;
		cerr << "images should be 8px tall and up to 127px wide" << endl;
		return 1;
	}

	cout << "converting image..." << endl;

	PNG preview(image.width(), image.height());
	int size = image.height() * image.width() + 2;
	unsigned char* buf = new unsigned char[size];
	buf[0] = (unsigned char) ((image.width() & 0xff) >> 8);
	buf[1] = (unsigned char) (image.width() & 0xff);
	for(int i = 0; i < image.width(); ++i) {
		for(int j = 0; j < image.height(); ++j) {
			RGBAPixel* px = image(i,j);
			unsigned char c = 0;
			unsigned char r = (unsigned char) (px->red) & 0xE0;
			unsigned char g = (unsigned char) ((px->green >> 3) & 0x1C);
			unsigned char b = (unsigned char) ((px->blue >> 6) & 0x03);
			c |= r | g | b;
			buf[image.height() * i + j + 2] = c;
			RGBAPixel* px2 = preview(i,j);
			px2->red = r;
			px2->green = g << 3;
			px2->blue = b << 6;
		}
	}
	preview.writeToFile("preview.png");

	FILE* file = fopen( "temp.bin", "wb" );
	if(file == NULL) {
		cerr << "error opening temp.bin" << endl;
		return 1;
	}
	fwrite( buf, 1, size, file );
	fclose(file);

	pid_t pid = fork();
	if(pid < 0) {
		cerr << "error forking process" << endl;
		return 1;
	} else if(pid == 0) {			
		char buff[1024];
   	 	ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
  		if (len != -1) {
      			buff[len] = '\0';
			string path(buff);
			path = path.substr(0, path.find_last_of('/') + 1);
			path += "avrdude.conf";
			cout << "flashing to arduino..." << endl;
			execl("./avrdude", "./avrdude", "-C", path.c_str(), "-p", "atmega32u4", "-c", "avr109", "-P", port.c_str(), "-b", "57600", "-D", "-U", "eeprom:w:temp.bin:i", NULL);
    		} else {
			cerr << "error reading path" << endl;
			return 1;
    		}
	} else {
		pid_t ret = wait(0);
		if(ret == -1) {
			cerr << "error flashing image" << endl;
			return 1;
		}
	}
	return 0;
}

