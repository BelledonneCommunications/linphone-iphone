################################################################################
# CMakeLists.txt
# Copyright (C) 2017  Belledonne Communications, Grenoble France
#
################################################################################
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
################################################################################

if (ENABLE_DOC OR ENABLE_CXX_WRAPPER OR ENABLE_CSHARP_WRAPPER OR ENABLE_JAVA_WRAPPER)
	find_package(Doxygen)
	if(DOXYGEN_FOUND)
		if(DOXYGEN_DOT_FOUND)
			set(top_srcdir "${PROJECT_SOURCE_DIR}")
			set(DOXYGEN_INPUT "")
			foreach (HEADER_FILE ${LINPHONE_HEADER_FILES})
				string(CONCAT DOXYGEN_INPUT ${DOXYGEN_INPUT} " \"${HEADER_FILE}\"")
			endforeach ()
			string(CONCAT DOXYGEN_INPUT ${DOXYGEN_INPUT} " \"${CMAKE_CURRENT_SOURCE_DIR}\"")
			string(CONCAT DOXYGEN_INPUT ${DOXYGEN_INPUT} " \"${PROJECT_SOURCE_DIR}/coreapi/help/examples/C\"")
			configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
			set(DOC_INPUT_FILES ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
				${CMAKE_CURRENT_SOURCE_DIR}/doxygen.dox
				${LINPHONE_HEADER_FILES}
			)
			set(XML_DIR "${CMAKE_CURRENT_BINARY_DIR}/xml")
			set(LINPHONE_DOXYGEN_XML_DIR ${XML_DIR} PARENT_SCOPE)
			add_custom_command(OUTPUT "${XML_DIR}/index.xml"
				COMMAND ${CMAKE_COMMAND} -E remove -f xml/*
				COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
				DEPENDS ${DOC_INPUT_FILES}
			)
			add_custom_target(linphone-doc ALL DEPENDS "${XML_DIR}/index.xml")
		else()
			if (ENABLE_CXX_WRAPPER)
				message(FATAL_ERROR "The dot program is needed to generate the linphone documentation. You can get it from http://www.graphviz.org/.")
			else()
				message(WARNING "The dot program is needed to generate the linphone documentation. You can get it from http://www.graphviz.org/.")
			endif()
		endif()
	endif()
endif()

