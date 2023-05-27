#pragma once

#include <iostream>
#include <array>
#include <memory>
#include "SerialPort.hpp"


namespace sia {
	static constexpr uint8_t DATA_COUNT = 6;
	static constexpr uint8_t BUFFER_MAX_SIZE = 60;
	using ForceSensorData = std::array<float, DATA_COUNT>;
	using ForceSensorBuffer = std::array<char, BUFFER_MAX_SIZE>;
	using ForceSensorCmd = std::string;

	// 力传感器通用指令
	enum ForceSensorCommonCmd {
		REQUEST_SEND_DATA_ONCE = 0,     // 请求发送一次数据
		FORCE_SENSOR_COMMON_CMD_USER_1, // [预留] 用户指令1
		FORCE_SENSOR_COMMON_CMD_USER_2, // [预留] 用户指令2
		FORCE_SENSOR_COMMON_CMD_USER_3, // [预留] 用户指令3
		FORCE_SENSOR_COMMON_CMD_MAX
	};

	// 使用串口通讯的力传感器类
	class ForceSensor {
	public:
		explicit ForceSensor() : m_pSerialPort(std::make_unique<SerialPort>()), m_Buffer({}), m_Data({}) {}
		explicit ForceSensor(const SerialConfig& cfg) : m_pSerialPort(std::make_unique<SerialPort>(cfg)), m_Buffer({}), m_Data({}) {}
		ForceSensor(const ForceSensor& other) = delete;
		ForceSensor& operator=(const ForceSensor& other) = delete;
		virtual ~ForceSensor() { Close(); }

		// 打开传感器
		bool Open() {
			return m_pSerialPort->Open();
		}
		bool IsOpen() {
			return m_pSerialPort->IsOpen();
		}
		// 关闭传感器
		void Close() {
			m_pSerialPort->Close();
		}

		// 读取一次力传感器的原始数据，存储到m_Buffer，不做格式检查
		bool UpdateBufferUntilCorrect(const ForceSensorCommonCmd& commCmd) {
			return UpdateBuffer(commCmd);
		}
		// 读取一次力传感器的数据，存储到m_Data，若失败或格式错误则重复尝试，最多maxCount次
		bool UpdateDataUntilCorrect(const uint8_t& maxCount = 10) {
			bool Ret = false;
			uint8_t Count = 0;
			do {
				Ret = UpdateData();
			} while (!Ret && Count++ < maxCount);
			return Ret;
		}

		// 读取力传感器的原始数据
		ForceSensorBuffer ReadBuffer(const ForceSensorCommonCmd& commCmd = REQUEST_SEND_DATA_ONCE) {
			if (UpdateBufferUntilCorrect(commCmd)) {
				return m_Buffer;
			}
			return {};
		}
		void ReadBuffer(ForceSensorBuffer* pBuffer, const ForceSensorCommonCmd& commCmd = REQUEST_SEND_DATA_ONCE) {
			if (UpdateBufferUntilCorrect(commCmd)) {
				*pBuffer = m_Buffer;
			}
		}
		// 读取力传感器的数据
		ForceSensorData ReadData() {
			if (UpdateDataUntilCorrect()) {
				return m_Data;
			}
			return {};
		}
		void ReadData(ForceSensorData* pData) {
			if (UpdateDataUntilCorrect()) {
				*pData = m_Data;
			}
		}

		// 发送指令
		bool SendCmd(const ForceSensorCommonCmd& commCmd) {
			if (commCmd < 0 || commCmd >= FORCE_SENSOR_COMMON_CMD_MAX) {
				return false;
			}
			if (!m_pSerialPort->IsOpen()) {
				return false;
			}

			ForceSensorCmd Cmd = ConvertCmd(commCmd);
			if (m_pSerialPort->Write(Cmd.c_str(), Cmd.size()) < 0) {
				return false;
			}
			return true;
		}

		// 获取原始数据
		ForceSensorBuffer GetBuffer() const {
			return m_Buffer;
		}
		void GetBuffer(ForceSensorBuffer* pBuffer) const {
			*pBuffer = m_Buffer;
		}
		// 获取数据
		ForceSensorData GetData() const {
			return m_Data;
		}
		void GetData(ForceSensorData* pData) const {
			*pData = m_Data;
		}

		// [可选] 初始化接口，可根据具体传感器选择是否要重写
		virtual bool Init() { return true; };

	private:
		// 读取一次力传感器的数据，存储到m_Data
		bool UpdateBuffer(const ForceSensorCommonCmd& commCmd) {
			if (SendCmd(commCmd)) {
				int ReadByteCount = m_pSerialPort->Read((char*)(&m_Buffer[0]), BUFFER_MAX_SIZE - 1);
				if (ReadByteCount > 0) {
					m_Buffer[ReadByteCount] = 0;
					m_pSerialPort->Flush();
					return true;
				}
			}
			return false;
		}
		bool UpdateData() {
			return (UpdateBuffer(REQUEST_SEND_DATA_ONCE) && ParseData(m_Data, m_Buffer));
		}

		// [纯虚函数] 数据解析函数
		virtual bool ParseData(ForceSensorData& data, const ForceSensorBuffer& buffer) = 0;
		// [纯虚函数] 力传感器指令翻译
		virtual ForceSensorCmd ConvertCmd(const ForceSensorCommonCmd& commCmd) = 0;

	private:
		std::unique_ptr<SerialPort> m_pSerialPort;
		ForceSensorBuffer m_Buffer; // 原始数据
		ForceSensorData m_Data;     // 数据 Fx, Fy, Fz, Tx, Ty, Tz
	};

	// DynPick传感器类
	class DynPick : public ForceSensor {
	public:
		using DynPickLsb = ForceSensorData;

		using ForceSensor::ForceSensor;
		~DynPick() {}

		bool Init() {
			return IsOpen() && UpdateLsb();
		}

	private:
		bool UpdateLsb() {
			bool Ret = false;
			// 获取灵敏度原始数据
			ForceSensorBuffer Buffer;
			constexpr uint8_t maxCount = 5;
			uint8_t Count = 0;
			do {
				Buffer = ReadBuffer(FORCE_SENSOR_COMMON_CMD_USER_1);
				Ret = (Buffer[BUFFER_LSB_CORRECT_SIZE - 1] == '\n');
				SIA_MSLEEP(10);
			} while (!Ret && Count++ < maxCount);
			if (!Ret) {
				return false;
			}
			// 解析灵敏度并更新m_Lsb
			uint8_t Offset = 0;
			uint8_t Index = 0;
			for (uint8_t i = 0; i < Buffer.size(); i++) {
				if (Buffer[i] == ',' || Buffer[i] == '\r') {
					Buffer[i] = 0;
					m_Lsb[Index++] = (float)atof((char*)(&Buffer[Offset]));
					Offset = i + 1;
					if (Index == 6) {
						return true;
					}
				}
			}
			return false;
		}

		bool ParseData(ForceSensorData& data, const ForceSensorBuffer& buffer) override {
			if (buffer[BUFFER_DATA_CORRECT_SIZE - 1] != '\n') {
				return false;
			}

			uint8_t Index = 0;
			uint32_t TempData = 0;
			for (uint8_t i = 1; Index < 6 && i < buffer.size(); i += 4, Index++) {
				TempData = (((buffer[i] - '0') << 4)
					+ ((buffer[i + 1] - '0') << 2)
					+ ((buffer[i + 2] - '0') << 1)
					+ (buffer[i + 3] - '0'));
				data[Index] = TempData / m_Lsb[Index];
			}
			return true;
		}
		ForceSensorCmd ConvertCmd(const ForceSensorCommonCmd& commCmd) override {
			ForceSensorCmd Cmd;
			switch (commCmd)
			{
			case REQUEST_SEND_DATA_ONCE: // 请求发送一次数据
				Cmd = "R";
				break;
			case FORCE_SENSOR_COMMON_CMD_USER_1: // 请求发送灵敏度
				Cmd = "p";
				break;
			default:
				break;
			}
			return Cmd;
		}
	
	private:
		const uint8_t BUFFER_DATA_CORRECT_SIZE = 27;
		const uint8_t BUFFER_LSB_CORRECT_SIZE = 49;
		DynPickLsb m_Lsb = { 1.0f,1.0f,1.0f,1.0f,1.0f,1.0f };
	};


}

std::ostream& operator<<(std::ostream& out, const sia::ForceSensorData& data) {
	for (auto i : data) {
		out << i << ' ';
	}
	return out;
}
std::ostream& operator<<(std::ostream& out, const sia::ForceSensorBuffer& buffer) {
	out << (char*)(&buffer[0]);
	return out;
}