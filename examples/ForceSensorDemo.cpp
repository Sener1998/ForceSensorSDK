#ifdef WIN32
#include <windows.h>
#define SERIAL_PROTNAME "COM5" // 串口名
#else
#include <unistd.h>
#define SERIAL_PROTNAME "/dev/ttyS5" // 串口名
#endif // WIN32

#include <iostream>
#include <memory>
#include "ForceSensor.hpp"

int main() {
	sia::SerialConfig cfg = { SERIAL_PROTNAME, 921600u, 8, 1, 'N' };
	std::unique_ptr<sia::ForceSensor> pfs = std::make_unique<sia::DynPick>(cfg);
	if (pfs->Open()) {
		std::cout << "Open Success！\n";

		if (pfs->Init()) {
			std::cout << "Init Success！\n";
			while (1) {
				std::cout << "Data: " << pfs->ReadData() << '\n';
				SIA_MSLEEP(500); // 最小为20
			}
		}
		else {
			std::cout << "Init Fail！\n";
		}
	}
	else {
		std::cout << "Open Fail!\n";
	}

	std::cin.get();
	return 0;
}
