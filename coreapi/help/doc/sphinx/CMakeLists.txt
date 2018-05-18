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

if (ENABLE_DOC)
	set(doc_source_dir ${CMAKE_CURRENT_BINARY_DIR}/source)
	set(doc_output_dir ${CMAKE_CURRENT_BINARY_DIR}/build)
	set(reference_doc_source_dir ${doc_source_dir}/reference)
	set(static_documentation_files
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
	)
	configure_file(conf.py.in ${doc_source_dir}/conf.py)
	foreach(file ${static_documentation_files})
		configure_file(${file} ${doc_source_dir}/${file} COPYONLY)
	endforeach(file)
	foreach(source ${LINPHONE_C_EXAMPLES_SOURCE})
		configure_file(${source} ${doc_source_dir}/samples/ COPYONLY)
	endforeach(source)
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${doc_source_dir}/_static ${reference_doc_source_dir})
	add_custom_target(sphinx-doc ALL ${PYTHON_EXECUTABLE} '${CMAKE_CURRENT_SOURCE_DIR}/gendoc.py' '${LINPHONE_DOXYGEN_XML_DIR}' -o '${reference_doc_source_dir}'
		COMMAND ${PYTHON_EXECUTABLE} -msphinx -M html '${doc_source_dir}' '${doc_output_dir}'
		DEPENDS linphone-doc)
	install(DIRECTORY ${doc_output_dir}/html/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/${LINPHONE_VERSION})
endif()
