cmake_minimum_required(VERSION 2.6)
project(LINPHONE C)

option(LINPHONE_ENABLE_VIDEO "Build linphone with video support." ON)

include_directories(
	include/
	coreapi/
	${CMAKE_CURRENT_BINARY_DIR}/coreapi/
	${CMAKE_CURRENT_SOURCE_DIR}/../cmake/libxml2/
)

include_directories(
	${CMAKE_INSTALL_PREFIX}/include
	${CMAKE_INSTALL_PREFIX}/include/libxml2
)

add_subdirectory(coreapi)
add_subdirectory(share)
