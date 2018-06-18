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

set(jni_sources "${CMAKE_CURRENT_BINARY_DIR}/src/linphone_jni.cc")

add_custom_command(OUTPUT "${jni_sources}"
	COMMAND ${CMAKE_COMMAND} -E remove -f java/org/linphone/core/* src/*
	COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/genwrapper.py" "${PROJECT_BINARY_DIR}/coreapi/help/doc/doxygen/xml" "-o" "${CMAKE_CURRENT_BINARY_DIR}"
	DEPENDS ${PROJECT_SOURCE_DIR}/tools/genapixml.py ${LINPHONE_HEADER_FILES}
	${PROJECT_SOURCE_DIR}/tools/metaname.py
	${PROJECT_SOURCE_DIR}/tools/metadoc.py
	${PROJECT_SOURCE_DIR}/tools/abstractapi.py
	genwrapper.py
	java_class.mustache
	java_enum.mustache
	java_interface_stub.mustache
	java_interface.mustache
	jni.mustache
	linphone-doc
	COMMENT "Generating java wrapper"
)

add_custom_target(linphonej ALL DEPENDS "${jni_sources}")

set(LINPHONE_JNI_SOURCES "${jni_sources}" PARENT_SCOPE)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/java" DESTINATION "${CMAKE_INSTALL_DATADIR}/linphonej/")

#install(DIRECTORY classes/ DESTINATION "${CMAKE_INSTALL_DATADIR}/linphonej/java/org/linphone/core/")
