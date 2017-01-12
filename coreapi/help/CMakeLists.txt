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

if (ENABLE_DOC)
	find_package(Doxygen)
	if(DOXYGEN_FOUND)
		if(DOXYGEN_DOT_FOUND)
			set(top_srcdir "${CMAKE_CURRENT_LIST_DIR}/../../")
			configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
			file(GLOB DOC_INPUT_FILES
				[^.]*.c
				[^.]*.dox
				../../include/linphone/[^.]*.h
			)
			add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html"
				COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
				DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${DOC_INPUT_FILES}
			)
			add_custom_target(linphone-doc ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/doc/html/index.html")
			install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/html" "${CMAKE_CURRENT_BINARY_DIR}/doc/xml"
				DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/linphone-${LINPHONE_VERSION}")
		else()
			message(WARNING "The dot program is needed to generate the linphone documentation. You can get it from http://www.graphviz.org/.")
		endif()
	endif()
endif()

if(ENABLE_JAVADOC)
	find_package(Java REQUIRED)
	set(JAVADOC_PACKAGES "org.linphone.core org.linphone.mediastream")
	set(JAVADOC_CLASSPATHS
		"${PROJECT_SOURCE_DIR}/java/common"
		"${PROJECT_SOURCE_DIR}/java/j2se"
		"${PROJECT_SOURCE_DIR}/mediastreamer2/java/src"
	)
	string(REPLACE ";" ":" JAVADOC_CLASSPATHS "${JAVADOC_CLASSPATHS}")
	set(JAVADOC_TITLE "Linphone SDK ${PROJECT_VERSION} reference documentation")
	set(JAVADOC_JAVA_REFERENCE "http://docs.oracle.com/javase/8/docs/api/")
	set(JAVADOC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc/java")
	set(JAVADOC_LOGFILE "${CMAKE_CURRENT_BINARY_DIR}/javadoc.log")
	configure_file("generate_javadoc.sh.in" "generate_javadoc.sh" @ONLY)
	add_custom_target(javadoc ALL
		COMMAND "${CMAKE_CURRENT_BINARY_DIR}/generate_javadoc.sh"
	)
endif()

if (ENABLE_TOOLS)
	set(USE_BUNDLE )
	if (IOS)
		set(USE_BUNDLE MACOSX_BUNDLE)
	endif()
	file(GLOB EXECUTABLES_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.c")
	foreach(EXECUTABLE ${EXECUTABLES_SOURCE})
		string(REPLACE ".c" "" EXECUTABLE_NAME ${EXECUTABLE})
		apply_compile_flags(${EXECUTABLE} "CPP" "C")
		add_executable(${EXECUTABLE_NAME} ${USE_BUNDLE} ${EXECUTABLE})
		target_link_libraries(${EXECUTABLE_NAME} ${LINPHONE_LIBS_FOR_TOOLS} ${MEDIASTREAMER2_LIBRARIES})
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
