############################################################################
# rootca.cmake
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

if(HTTPS_CA_DIR)
	set(ENV{HTTPS_CA_DIR} "${HTTPS_CA_DIR}")
endif()

execute_process(
	COMMAND ${CMAKE_COMMAND} -E remove "fresh-rootca.pem"
	WORKING_DIRECTORY ${OUTPUT_DIR}
)
execute_process(
	COMMAND "../scripts/mk-ca-bundle.pl" "${OUTPUT_DIR}/fresh-rootca.pem"
	WORKING_DIRECTORY ${WORK_DIR}
)
if(EXISTS "${OUTPUT_DIR}/fresh-rootca.pem")
	file(RENAME "${OUTPUT_DIR}/fresh-rootca.pem" "${OUTPUT_DIR}/rootca.pem")
else()
	file(COPY "${WORK_DIR}/archived-rootca.pem" DESTINATION "${OUTPUT_DIR}")
	file(RENAME "${OUTPUT_DIR}/archived-rootca.pem" "${OUTPUT_DIR}/rootca.pem")
endif()
