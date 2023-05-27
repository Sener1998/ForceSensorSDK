#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // WIN32
#include <iostream>
#include "SerialPort.hpp"

#ifdef WIN32
#define SIA_SLEEP(x) Sleep(x)
#else
#define SIA_SLEEP(x) usleep(x*1000)
#endif // WIN32

int main() {
#ifdef WIN32
	sia::SerialConfig cfg = { "COM5", 921600u, 8, 1, 'N' };
#else
	sia::SerialConfig cfg = { "/dev/ttyS5", 921600u, 8, 1, 'N' };
#endif // WIN32
	sia::SerialPort sdev(cfg);
	if (sdev.Open()) {
		std::cout << "Success!\n";

		char cmd = 'R';
		char buffer[50] = { 0 };
		int count = 0;
		while (1) {
			sdev.Write(&cmd, 1);
			count = sdev.Read(buffer, 49);
			if (count > 0) {
				buffer[count - 1] = 0;
				std::cout << "Read: " << buffer;
			}
			SIA_SLEEP(500);
		}
	}
	else {
		std::cout << "Fail!\n";
	}

	return 0;
}
