############################################################################
# LinphoneUtils.cmake
# Copyright (C) 2018  Belledonne Communications, Grenoble France
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

function(check_python_module module_name)
	execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import ${module_name}"
		RESULT_VARIABLE result
		OUTPUT_QUIET
		ERROR_QUIET)
	if(result EQUAL 0)
		message(STATUS "'${module_name}' python module found")
	else()
		message(FATAL_ERROR "'${module_name}' python module not found")
	endif()
endfunction()
