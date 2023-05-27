#pragma once

#ifdef WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif // WIN32
#include <iostream>

namespace sia {
	// 串口设备状态
	enum SERIAL_STATE {
		SERIAL_OPEN = 0,
		SERIAL_CLOSE,
		SERIAL_ERROR
	};
	// 串口通信参数
	struct SerialConfig {
		std::string PortName; // 端口名
		uint32_t Baudrate;    // 波特率
		uint8_t Databits;     // 数据位数，5、6、7、8
		uint8_t Stopbits;     // 停止位，0表示1位、1表示1.5位、2表示2位
		char Parity;          // 校验为，'N'表示无、'O'表示奇校验、'E'表示偶校验
	};

#ifdef WIN32
	// windows 串口类
	class SerialPort {
	public:
		explicit SerialPort() : m_Com(INVALID_HANDLE_VALUE), m_SerialState(SERIAL_CLOSE), m_SerialConfig({}) {}
		explicit SerialPort(const SerialConfig& cfg) : m_Com(INVALID_HANDLE_VALUE), m_SerialState(SERIAL_CLOSE), m_SerialConfig(cfg) {}
		~SerialPort() { Close(); }

		bool SetSerialConfig(const SerialConfig& cfg) {
			// 句柄配置
			SetCommMask(m_Com, EV_RXCHAR);
			SetupComm(m_Com, 4096, 4096);
			PurgeComm(m_Com, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR); // 清理串口

			// 串口配置
			DCB ComDcb;
			GetCommState(m_Com, &ComDcb);
			// 波特率
			ComDcb.BaudRate = cfg.Baudrate;
			// 检验位
			switch (cfg.Parity) {
			case 'O':
				ComDcb.Parity = ODDPARITY;
				break;
			case 'E':
				ComDcb.Parity = EVENPARITY;
				break;
			case 'N':
			default:
				ComDcb.Parity = NOPARITY;
				break;
			}
			// 停止位
			switch (cfg.Stopbits) {
			case 3:
				ComDcb.StopBits = ONE5STOPBITS;
				break;
			case 2:
				ComDcb.StopBits = TWOSTOPBITS;
				break;
			case 1:
			default:
				ComDcb.StopBits = ONESTOPBIT;
				break;
			}
			// 数据位
			switch (cfg.Databits)
			{
			case 5:
			case 6:
			case 7:
				ComDcb.ByteSize = cfg.Databits;
				break;
			case 8:
			default:
				ComDcb.ByteSize = 8;
				break;
			}

			if (SetCommState(m_Com, &ComDcb)) {
				return true;
			}
			Close();
			return false;
		}

		bool Open() {
			if (m_SerialState == SERIAL_OPEN) {
				return true;
			}

			if (m_Com != INVALID_HANDLE_VALUE) {
				CloseHandle(m_Com);
			}
			m_Com = CreateFile((LPCTSTR)m_SerialConfig.PortName.c_str(), // 端口名
				GENERIC_READ | GENERIC_WRITE,    // 读写
				0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (m_Com == INVALID_HANDLE_VALUE) {
				m_SerialState = SERIAL_ERROR;
				return false;
			}
			else {
				m_SerialState = SERIAL_OPEN;
				return SetSerialConfig(m_SerialConfig);
			}
		}
		void Close() {
			if (m_Com != INVALID_HANDLE_VALUE) {
				CloseHandle(m_Com);
				m_Com = INVALID_HANDLE_VALUE;
			}
			m_SerialState = SERIAL_CLOSE;
		}
		bool IsOpen() {
			return (m_SerialState == SERIAL_OPEN);
		}

		int Read(char* data, int count) {
			if (m_Com == INVALID_HANDLE_VALUE) {
				return -1;
			}

			DWORD ErrorFlags;
			COMSTAT ComStat;
			DWORD ReadByteCount = 0;
			ClearCommError(m_Com, &ErrorFlags, &ComStat); // 查询串口状态
			if (ComStat.cbInQue > 0) {                        // 若读缓冲区有数据则取出
				if ((DWORD)count > ComStat.cbInQue) {
					count = ComStat.cbInQue;
				}
				if (!ReadFile(m_Com, data, count, &ReadByteCount, NULL)) {
					if (GetLastError() != ERROR_IO_PENDING) { // 若阻塞则不能读
						ReadByteCount = (DWORD)-1;
					}
				}
			}
			return ReadByteCount;
		}
		int Write(const char* data, int count) {
			if (m_Com == INVALID_HANDLE_VALUE) {
				return -1;
			}

			DWORD ErrorFlags;
			COMSTAT ComStat;
			DWORD SendByteCount = 0;
			ClearCommError(m_Com, &ErrorFlags, &ComStat); // 查询串口状态
			if (ComStat.cbOutQue == 0) {                  // 若写缓冲区没有数据则发送
				if (!WriteFile(m_Com, data, count, &SendByteCount, NULL)) {
					if (GetLastError() != ERROR_IO_PENDING) { // 若阻塞则不能发送
						SendByteCount = (DWORD)-1;
					}
				}
			}
			return SendByteCount;
		}

	private:
		HANDLE m_Com;
		SERIAL_STATE m_SerialState;
		SerialConfig m_SerialConfig;
	};
#else
	// linux 串口类
	class SerialPort {
	public:
		explicit SerialPort() : m_Fd(-1), m_SerialState(SERIAL_CLOSE), m_SerialConfig({}) {}
		explicit SerialPort(const SerialConfig& cfg) : m_Fd(-1), m_SerialState(SERIAL_CLOSE), m_SerialConfig(cfg) {}
		~SerialPort() { Close(); }

		bool SetSerialConfig(const SerialConfig& cfg) {
			struct termios Opt;
			tcgetattr(m_Fd, &Opt);

			// 波特率
			cfsetispeed(&Opt, cfg.Baudrate);
			cfsetospeed(&Opt, cfg.Baudrate);
			Opt.c_cflag |= (CLOCAL | CREAD);
			// 检验位
			switch (cfg.Parity) {
			case 'O':
				Opt.c_cflag |= PARENB;
				Opt.c_cflag |= PARODD;
				break;
			case 'E':
				Opt.c_cflag |= PARENB;
				Opt.c_cflag &= ~PARODD;
				break;
			case 'N':
			default:
				Opt.c_cflag &= ~PARENB;;
				break;
			}
			// 停止位
			switch (cfg.Stopbits) {
			case 2:
				Opt.c_cflag |= CSTOPB;
				break;
			case 3:
			case 1:
			default:
				Opt.c_cflag &= ~CSTOPB;
				break;
			}
			// 数据位
			Opt.c_cflag &= ~CSIZE;
			switch (cfg.Databits)
			{
			case 5:
				Opt.c_cflag |= CS5;
				break;
			case 6:
				Opt.c_cflag |= CS6;
				break;
			case 7:
				Opt.c_cflag |= CS7;
				break;
			case 8:
			default:
				Opt.c_cflag |= CS8;
				break;
			}
			// 输入输出
			Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
			Opt.c_oflag &= ~OPOST;

			tcflush(m_Fd, TCIOFLUSH);
			if (tcsetattr(m_Fd, TCSANOW, &Opt) != 0) {
				Close();
				return false;
			}
			return true;
		}

		bool Open() {
			if (m_SerialState == SERIAL_OPEN) {
				return true;
			}

			m_Fd = open(m_SerialConfig.PortName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
			if (m_Fd < 0) {
				m_SerialState = SERIAL_ERROR;
				return false;
			}

			m_SerialState = SERIAL_OPEN;
			return SetSerialConfig(m_SerialConfig);
		}
		void Close() {
			if (m_Fd >= 0) {
				close(m_Fd);
				m_Fd = -1;
			}
			m_SerialState = SERIAL_CLOSE;
		}
		bool IsOpen() {
			return (m_SerialState == SERIAL_OPEN);
		}

		int Read(char* data, int count) {
			if (m_SerialState != SERIAL_OPEN) {
				return -1;
			}
			return read(m_Fd, data, count);
		}
		int Write(const char* data, int count) {
			if (m_SerialState != SERIAL_OPEN) {
				return -1;
			}
			return write(m_Fd, data, count);
		}

	private:
		int m_Fd;
		SERIAL_STATE m_SerialState;
		SerialConfig m_SerialConfig;
	};
#endif // WIN32
}



