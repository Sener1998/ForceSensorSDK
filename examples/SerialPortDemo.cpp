#ifdef WIN32
#include <windows.h>
#define SERIAL_PROTNAME "COM5" // 串口名
#else
#include <unistd.h>
#define SERIAL_PROTNAME "/dev/ttyS5" // 串口名
#endif // WIN32

#include <iostream>
#include "SerialPort.hpp"

int main() {
	sia::SerialPort sdev({ SERIAL_PROTNAME, 921600u, 8, 1, 'N' });
	if (sdev.Open()) {
		std::cout << "Open Success!\n";

		char cmd = 'R';
		char buffer[50] = { 0 };
		int count = 0;
		while (1) {
			sdev.Write(&cmd, 1);
			count = sdev.Read(buffer, 49);
			if (count > 0) {
				buffer[count] = 0;
				std::cout << "Read: " << buffer;
			}
			SIA_MSLEEP(500);
		}
	}
	else {
		std::cout << "Open Fail!\n";
	}

	std::cin.get();
	return 0;
}
