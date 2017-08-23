############################################################################
# CMakeLists.txt
# Copyright (C) 2017  Belledonne Communications, Grenoble France
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
if (ENABLE_TOOLS)
	set(USE_BUNDLE )
	if (IOS)
		set(USE_BUNDLE MACOSX_BUNDLE)
	endif()
	file(GLOB EXECUTABLES_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.c")
	foreach(EXECUTABLE ${EXECUTABLES_SOURCE})
		string(REPLACE ".c" "" EXECUTABLE_NAME ${EXECUTABLE})
		bc_apply_compile_flags(${EXECUTABLE} STRICT_OPTIONS_CPP STRICT_OPTIONS_C)
		add_executable(${EXECUTABLE_NAME} ${USE_BUNDLE} ${EXECUTABLE})
		target_link_libraries(${EXECUTABLE_NAME} ${LINPHONE_LIBS_FOR_TOOLS} ${MEDIASTREAMER2_LIBRARIES} ${ORTP_LIBRARIES} ${BCTOOLBOX_CORE_LIBRARIES})
		set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS "${LINPHONE_LDFLAGS}")
		if (NOT IOS)
			install(TARGETS ${EXECUTABLE_NAME}
				RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
				LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
				ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()
	endforeach()
endif()

