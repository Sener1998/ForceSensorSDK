set(DEMO_LIST
	SerialPortDemo
	ForceSensorDemo
)

foreach(DEMO ${DEMO_LIST})
	add_executable (${DEMO} ${DEMO}.cpp)
endforeach()
