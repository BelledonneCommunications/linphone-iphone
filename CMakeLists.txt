cmake_minimum_required(VERSION 2.6)
project(LINPHONE C)

if(NOT ORTP_ROOT_DIR)
	set(ORTP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/oRTP)
endif()
if(NOT ORTP_INCLUDE_DIR)
	set(ORTP_INCLUDE_DIR ${ORTP_ROOT_DIR}/include)
endif()
if(NOT MS2_ROOT_DIR)
	set(MS2_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mediastreamer2)
endif()
if(NOT MS2_INCLUDE_DIR)
	set(MS2_INCLUDE_DIR ${MS2_ROOT_DIR}/include)
endif()
if(NOT LIBXML2_ROOT_DIR)
	set(LIBXML2_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libxml2)
endif()
if(NOT LIBXML2_INCLUDE_DIR)
	set(LIBXML2_INCLUDE_DIR ${LIBXML2_ROOT_DIR}/include)
endif()
if(NOT BELLESIP_ROOT_DIR)
	set(BELLESIP_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../belle-sip)
endif()
if(NOT BELLESIP_INCLUDE_DIR)
	set(BELLESIP_INCLUDE_DIR ${BELLESIP_ROOT_DIR}/include)
endif()

include_directories(
	include/
	coreapi/
	${CMAKE_CURRENT_SOURCE_DIR}/../cmake/libxml2/
)

if(USE_INSTALLED_COMPONENTS)
	include_directories(
		${CMAKE_INSTALL_PREFIX}/include
		${CMAKE_INSTALL_PREFIX}/include/libxml2
	)
else()
	include_directories(
		${ORTP_INCLUDE_DIR}
		${MS2_INCLUDE_DIR}
		${LIBXML2_INCLUDE_DIR}
		${BELLESIP_INCLUDE_DIR}
	)
	if(WIN32)
		include_directories(${ORTP_ROOT_DIR}/build/vsx/oRTP/oRTP/)
	endif(WIN32)
endif()

add_subdirectory(coreapi)
add_subdirectory(share)

if(INSTALL_COMPONENT_IN_POSTBUILD)
	add_install_target(INSTALL_liblinphone COMP_liblinphone liblinphone)
endif()
