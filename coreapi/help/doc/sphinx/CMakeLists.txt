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
	set(STATIC_DOCUMENTATION_FILES
		guides/authentication.rst
		guides/buddy_list.rst
		guides/call_control.rst
		guides/call_logs.rst
		guides/call_misc.rst
		guides/chatroom.rst
		guides/conferencing.rst
		guides/event_api.rst
		guides/initializing.rst
		guides/ios_portability.rst
		guides/linphone_address.rst
		guides/media_parameters.rst
		guides/misc.rst
		guides/network_parameters.rst
		guides/proxies.rst
		index.rst
		logo.png
		samples/samples.rst
		toc.rst
	)
	configure_file(conf.py.in source/conf.py)
	foreach(file ${STATIC_DOCUMENTATION_FILES})
		configure_file(${file} source/${file} COPYONLY)
	endforeach(file)
	foreach(source ${LINPHONE_C_EXAMPLES_SOURCE})
		configure_file(${source} source/samples/ COPYONLY)
	endforeach(source)
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/source/_static)
	add_custom_target(sphinx-doc ALL ${PYTHON_EXECUTABLE} '${CMAKE_CURRENT_SOURCE_DIR}/gendoc.py' '${LINPHONE_DOXYGEN_XML_DIR}' -o 'source'
		COMMAND ${PYTHON_EXECUTABLE} -msphinx -M html 'source' 'build'
		DEPENDS linphone-doc)
endif()
