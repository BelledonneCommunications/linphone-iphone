##
## Android.mk -Android build script-
##
##
## Copyright (C) 2013  Belledonne Communications, Grenoble, France
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

LOCAL_PATH:= $(call my-dir)/../../tools

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
	xml2lpc.c \
	xml2lpc_jni.cc  \

LOCAL_CFLAGS += -DIN_LINPHONE

LOCAL_C_INCLUDES = \
	$(LOCAL_PATH)/../coreapi \
	$(LOCAL_PATH)/../oRTP/include \
	$(LOCAL_PATH)/../mediastreamer2/include \
	$(LOCAL_PATH)/../../externals/libxml2/include \
	$(LOCAL_PATH)/../../externals/build/libxml2 \

LOCAL_SHARED_LIBRARIES = \
#	liblinphonenoneon \
#	liblinphone \

LOCAL_MODULE := libxml2lpc

include $(BUILD_STATIC_LIBRARY)
