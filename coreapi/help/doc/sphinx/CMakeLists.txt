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

if (ENABLE_SPHINX_DOC)
	set(GENERATED_LANGUAGES c cpp csharp)
	set(DOCUMENTATION_DIRS )
	set(GENERATED_SPHINX_SOURCES )
	foreach(LANGUAGE_ ${GENERATED_LANGUAGES})
		set(DOCUMENTATION_DIR ${CMAKE_CURRENT_BINARY_DIR}/source/${LANGUAGE_})
		list(APPEND DOCUMENTATION_DIRS ${DOCUMENTATION_DIR})
		list(APPEND GENERATED_SPHINX_SOURCES ${DOCUMENTATION_DIR}/index.rst)
	endforeach(LANGUAGE_)
	set(PYTHON_SCRIPTS gendoc.py
		${linphone_SOURCE_DIR}/tools/abstractapi.py
		${linphone_SOURCE_DIR}/tools/genapixml.py
		${linphone_SOURCE_DIR}/tools/metadoc.py
		${linphone_SOURCE_DIR}/tools/metaname.py
	)
	set(MUSTACHE_TEMPLATES class_page.mustache
		enum_page.mustache
		index_page.mustache
	)
	set(STATIC_DOCUMENTATION_FILES authentication.rst
		buddy_list.rst
		call_control.rst
		call_logs.rst
		call_misc.rst
		chatroom.rst
		conferencing.rst
		event_api.rst
		index.rst
		initializing.rst
		ios_portability.rst
		linphone_address.rst
		logo.png
		media_parameters.rst
		misc.rst
		network_parameters.rst
		proxies.rst
	)
	configure_file(conf.py.in source/conf.py)
	foreach(file ${STATIC_DOCUMENTATION_FILES})
		configure_file(${file} source/${files} COPYONLY)
	endforeach(file)
	add_custom_target(sphinx-doc ALL ${PYTHON_EXECUTABLE} '${CMAKE_CURRENT_SOURCE_DIR}/gendoc.py' '${LINPHONE_DOXYGEN_XML_DIR}' -o 'source'
		COMMAND ${PYTHON_EXECUTABLE} -msphinx -M html 'source' 'build'
		DEPENDS linphone-doc)
endif()
