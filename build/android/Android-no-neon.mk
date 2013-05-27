##
## Android.mk -Android build script-
##
##
## Copyright (C) 2010  Belledonne Communications, Grenoble, France
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##

LOCAL_PATH:= $(call my-dir)/../../coreapi

include $(CLEAR_VARS)

include $(linphone-root-dir)/submodules/linphone/build/android/common.mk

ifeq ($(LINPHONE_VIDEO),1)
LOCAL_SHARED_LIBRARIES += \
	libavcodecnoneon \
	libswscale \
	libavcore \
	libavutil
endif

LOCAL_MODULE := liblinphonenoneon
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_MODULE_FILENAME := liblinphonearmv5noneon
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_MODULE_FILENAME := liblinphonearmv7noneon
endif
ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_MODULE_FILENAME := liblinphonex86
endif

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)

