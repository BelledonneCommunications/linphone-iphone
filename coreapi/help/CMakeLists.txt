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

find_package(Doxygen)

if(DOXYGEN_FOUND)
	if(DOXYGEN_DOT_FOUND)
		set(top_srcdir ${CMAKE_SOURCE_DIR})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
		file(GLOB DOC_INPUT_FILES
			[^.]*.c
			[^.]*.dox
			../[^.]*.h
			../[^.]*.c
		)
		add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html"
			COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${DOC_INPUT_FILES}
		)
		add_custom_target(doc ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html")
		install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/html" "${CMAKE_CURRENT_BINARY_DIR}/doc/xml"
			DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/linphone-${LINPHONE_VERSION}")
	else()
		message(WARNING "The dot program is needed to generate the linphone documentation. You can get it from http://www.graphviz.org/.")
	endif()
endif()
