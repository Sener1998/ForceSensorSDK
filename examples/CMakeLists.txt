set(DEMO_LIST
	SerialPortDemo
	DynPickDemo
)

foreach(DEMO ${DEMO_LIST})
	add_executable (${DEMO} ${DEMO}.cpp)
endforeach()
