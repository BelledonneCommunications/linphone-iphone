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

if(ANDROID)
	find_package(Java REQUIRED COMPONENTS Development)

	set(JNI_CLASSES
		"org.linphone.core.LinphoneAddressImpl"
		"org.linphone.core.LinphoneAuthInfoImpl"
		"org.linphone.core.LinphoneCallImpl"
		"org.linphone.core.LinphoneCallLogImpl"
		"org.linphone.core.LinphoneCallParamsImpl"
		"org.linphone.core.LinphoneCallStatsImpl"
		"org.linphone.core.LinphoneChatMessageImpl"
		"org.linphone.core.LinphoneChatRoomImpl"
		"org.linphone.core.LinphoneCoreFactoryImpl"
		"org.linphone.core.LinphoneCoreImpl"
		"org.linphone.core.LinphoneFriendImpl"
		"org.linphone.core.LinphoneProxyConfigImpl"
		"org.linphone.core.PayloadTypeImpl"
		"org.linphone.core.LpConfigImpl"
		"org.linphone.core.LinphoneInfoMessageImpl"
		"org.linphone.core.LinphoneEventImpl"
		"org.linphone.core.PresenceActivityImpl"
		"org.linphone.core.PresenceModelImpl"
		"org.linphone.core.PresenceNoteImpl"
		"org.linphone.core.PresencePersonImpl"
		"org.linphone.core.PresenceServiceImpl"
		"org.linphone.core.ErrorInfoImpl"
		"org.linphone.core.TunnelConfigImpl"
	)

	# HACK: accessing mediastreamer2 source directory from here is just wrong!
	add_custom_target(linphonecore-jni-header
		COMMAND ${Java_JAVAH_EXECUTABLE} -o "${CMAKE_BINARY_DIR}/linphonecore_jni.h" -classpath "${CMAKE_CURRENT_SOURCE_DIR}/impl:${CMAKE_CURRENT_SOURCE_DIR}/common:${CMAKE_CURRENT_SOURCE_DIR}/../mediastreamer2/java/src" ${JNI_CLASSES}
	)
endif()
