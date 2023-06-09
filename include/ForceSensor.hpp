#pragma once

#include <iostream>
#include <array>
#include <memory>
#include <iomanip>
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
			if (m_pSerialPort->Write(Cmd.c_str(), (int)Cmd.size()) < 0) {
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
}

std::ostream& operator<<(std::ostream& out, const sia::ForceSensorData& data) {
	out << std::fixed << std::setprecision(6);
	for (auto i : data) {
		out << i << ' ';
	}
	return out;
}
std::ostream& operator<<(std::ostream& out, const sia::ForceSensorBuffer& buffer) {
	out << (char*)(&buffer[0]);
	return out;
}

