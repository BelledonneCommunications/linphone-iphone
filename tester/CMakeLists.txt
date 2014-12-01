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

set(SOURCE_FILES
	accountmanager.c
	call_tester.c
	dtmf_tester.c
	eventapi_tester.c
	flexisip_tester.c
	liblinphone_tester.c
	log_collection_tester.c
	message_tester.c
	player_tester.c
	presence_tester.c
	quality_reporting_tester.c
	register_tester.c
	remote_provisioning_tester.c
	setup_tester.c
	stun_tester.c
	tester.c
	transport_tester.c
	upnp_tester.c
)

add_executable(liblinphone_tester ${SOURCE_FILES})
target_include_directories(liblinphone_tester PUBLIC ${CUNIT_INCLUDE_DIRS})
target_link_libraries(liblinphone_tester linphone ${CUNIT_LIBRARIES})
