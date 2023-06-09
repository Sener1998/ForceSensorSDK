#pragma once

#include "ForceSensor.hpp"

namespace sia {
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
				TempData = (((buffer[i] - '0') << 12)
					+ ((buffer[i + 1] - '0') << 8)
					+ ((buffer[i + 2] - '0') << 4)
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

