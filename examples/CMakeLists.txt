include_directories(${FORCE_SENSOR_INC_DIR})
set(DEMO_LIST
	SerialPortDemo
)

foreach(DEMO ${DEMO_LIST})
	add_executable (${DEMO} ${DEMO}.cpp)
endforeach()
