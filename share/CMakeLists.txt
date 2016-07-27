############################################################################
# CMakeLists.txt
# Copyright (C) 2014  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

if(APPLE)
	find_program(OPENSSL_PROGRAM openssl)
	execute_process(
		COMMAND ${OPENSSL_PROGRAM} version -d
		OUTPUT_VARIABLE OPENSSL_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(REGEX REPLACE "OPENSSLDIR: \"(.*)\"" "\\1" HTTPS_CA_DIR "${OPENSSL_VERSION}")
endif()

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/rootca.pem PROPERTIES GENERATED TRUE)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rootca.pem
	COMMAND ${CMAKE_COMMAND} -DHTTPS_CA_DIR=${HTTPS_CA_DIR} -DWORK_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/rootca.cmake)
add_custom_target(rootca ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rootca.pem)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rootca.pem
	DESTINATION ${PACKAGE_DATA_DIR}/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

set(SOUND_FILES
	hello16000.wav
	hello8000.wav
	incoming_chat.wav
	ringback.wav
	dont_wait_too_long.mkv
	toy-mono.wav
)

install(FILES ${SOUND_FILES}
	DESTINATION ${PACKAGE_SOUND_DIR}
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if(ENABLE_GTK_UI)
	set(prefix "${CMAKE_INSTALL_PREFIX}")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/audio-assistant.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/audio-assistant.desktop)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linphone.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/linphone.desktop)

	set(FREEDESKTOP_FILES
		${CMAKE_CURRENT_BINARY_DIR}/audio-assistant.desktop
		${CMAKE_CURRENT_BINARY_DIR}/linphone.desktop
	)

	install(FILES ${FREEDESKTOP_FILES}
		DESTINATION ${PACKAGE_FREEDESKTOP_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)
endif()

add_subdirectory(rings)
