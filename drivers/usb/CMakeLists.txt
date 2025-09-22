add_library(usb INTERFACE)

target_sources(usb INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/xinput_host.c
	${CMAKE_CURRENT_LIST_DIR}/hid_app.cpp
	${CMAKE_CURRENT_LIST_DIR}/joy_hid.cpp
)

target_link_libraries(usb INTERFACE)

target_include_directories(usb INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)
