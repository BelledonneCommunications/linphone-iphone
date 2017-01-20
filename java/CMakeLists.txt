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

if(ANDROID)
	find_package(Java REQUIRED COMPONENTS Development)

	set(JNI_CLASSES
		"org.linphone.core.ErrorInfoImpl"
		"org.linphone.core.LinphoneAddressImpl"
		"org.linphone.core.LinphoneAuthInfoImpl"
		"org.linphone.core.LinphoneBufferImpl"
		"org.linphone.core.LinphoneCallImpl"
		"org.linphone.core.LinphoneCallLogImpl"
		"org.linphone.core.LinphoneCallParamsImpl"
		"org.linphone.core.LinphoneCallStatsImpl"
		"org.linphone.core.LinphoneChatMessageImpl"
		"org.linphone.core.LinphoneChatRoomImpl"
		"org.linphone.core.LinphoneConferenceImpl"
		"org.linphone.core.LinphoneConferenceParamsImpl"
		"org.linphone.core.LinphoneContentImpl"
		"org.linphone.core.LinphoneCoreFactoryImpl"
		"org.linphone.core.LinphoneCoreImpl"
		"org.linphone.core.LinphoneEventImpl"
		"org.linphone.core.LinphoneFriendImpl"
		"org.linphone.core.LinphoneFriendListImpl"
		"org.linphone.core.LinphoneInfoMessageImpl"
		"org.linphone.core.LinphoneNatPolicyImpl"
		"org.linphone.core.LinphonePlayerImpl"
		"org.linphone.core.LinphoneProxyConfigImpl"
		"org.linphone.core.LpConfigImpl"
		"org.linphone.core.PayloadTypeImpl"
		"org.linphone.core.PresenceActivityImpl"
		"org.linphone.core.PresenceModelImpl"
		"org.linphone.core.PresenceNoteImpl"
		"org.linphone.core.PresencePersonImpl"
		"org.linphone.core.PresenceServiceImpl"
		"org.linphone.core.TunnelConfigImpl"
	)

	# HACK: accessing mediastreamer2 source directory from here is just wrong!
	add_custom_target(linphonecore-jni-header
		COMMAND ${Java_JAVAH_EXECUTABLE} -o "${CMAKE_CURRENT_BINARY_DIR}/linphonecore_jni.h" -classpath "${CMAKE_CURRENT_SOURCE_DIR}/impl:${CMAKE_CURRENT_SOURCE_DIR}/common:${CMAKE_CURRENT_SOURCE_DIR}/../mediastreamer2/java/src" ${JNI_CLASSES}
	)
endif()
