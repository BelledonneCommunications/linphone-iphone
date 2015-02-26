############################################################################
# gitversion.cmake
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

if(GIT_EXECUTABLE)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --always
		WORKING_DIRECTORY ${WORK_DIR}
		OUTPUT_VARIABLE GIT_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E echo "#define LIBLINPHONE_GIT_VERSION \"${GIT_REVISION}\""
		OUTPUT_FILE ${OUTPUT_DIR}/liblinphone_gitversion.h
	)
else()
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E echo "#define LIBLINPHONE_GIT_VERSION \"unknown\""
		OUTPUT_FILE ${OUTPUT_DIR}/liblinphone_gitversion.h
	)
endif()
