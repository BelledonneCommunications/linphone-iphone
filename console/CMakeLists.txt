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

set(LINPHONEC_SOURCE_FILES
	linphonec.c
	linphonec.h
	commands.c
)
set(LINPHONECSH_SOURCE_FILES
	shell.c
)

apply_compile_flags(LINPHONEC_SOURCE_FILES "CPP" "C")
if(MSVC)
	get_source_file_property(COMMANDS_C_COMPILE_FLAGS commands.c COMPILE_FLAGS)
	set(COMMANDS_C_COMPILE_FLAGS "${COMMANDS_C_COMPILE_FLAGS} /wd4996") # Disable "was declared deprecated" warnings
	set_source_files_properties(commands.c PROPERTY COMPILE_FLAGS "${COMMANDS_C_COMPILE_FLAGS}")
endif()

add_executable(linphonec ${LINPHONEC_SOURCE_FILES})
target_link_libraries(linphonec ${LINPHONE_LIBS_FOR_TOOLS} ${BCTOOLBOX_CORE_LIBRARIES} ${ORTP_LIBRARIES} ${MEDIASTREAMER2_LIBRARIES})
set_target_properties(linphonec PROPERTIES LINK_FLAGS "${LINPHONE_LDFLAGS}")

if(INTL_FOUND)
	target_link_libraries(linphonec ${INTL_LIBRARIES})
endif()

if(WIN32)
	add_executable(linphoned WIN32 ${LINPHONEC_SOURCE_FILES})
	target_link_libraries(linphoned ${LINPHONE_LIBS_FOR_TOOLS} ${BCTOOLBOX_CORE_LIBRARIES} ${ORTP_LIBRARIES} ${MEDIASTREAMER2_LIBRARIES})
	if(INTL_FOUND)
		target_link_libraries(linphoned ${INTL_LIBRARIES})
	endif()
endif()

add_executable(linphonecsh ${LINPHONECSH_SOURCE_FILES})
target_link_libraries(linphonecsh ${LINPHONE_LIBS_FOR_TOOLS} ${ORTP_LIBRARIES})
set_target_properties(linphonecsh PROPERTIES LINK_FLAGS "${LINPHONE_LDFLAGS}")

set(INSTALL_TARGETS linphonec linphonecsh)
if(WIN32)
	list(APPEND INSTALL_TARGETS linphoned)
endif()

install(TARGETS ${INSTALL_TARGETS}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
