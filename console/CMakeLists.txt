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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
add_executable(linphonec ${LINPHONEC_SOURCE_FILES})
target_link_libraries(linphonec linphone)

if(WIN32)
	add_executable(linphoned WIN32 ${LINPHONEC_SOURCE_FILES})
	target_link_libraries(linphoned linphone)
endif()

add_executable(linphonecsh ${LINPHONECSH_SOURCE_FILES})
target_link_libraries(linphonecsh linphone)

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
