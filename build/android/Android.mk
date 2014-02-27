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

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
	linphonecore.c \
	misc.c  \
	enum.c \
	presence.c \
	proxy.c \
	friend.c \
	authentication.c \
	lpconfig.c \
	chat.c \
	sipsetup.c \
	siplogin.c \
	address.c \
	linphonecore_jni.cc \
	bellesip_sal/sal_address_impl.c \
	bellesip_sal/sal_impl.c \
	bellesip_sal/sal_op_call.c \
	bellesip_sal/sal_op_call_transfer.c \
	bellesip_sal/sal_op_impl.c \
	bellesip_sal/sal_op_message.c \
	bellesip_sal/sal_op_presence.c \
	bellesip_sal/sal_op_registration.c \
	bellesip_sal/sal_op_publish.c \
	bellesip_sal/sal_op_info.c \
	bellesip_sal/sal_op_events.c \
	bellesip_sal/sal_sdp.c \
	sal.c \
	offeranswer.c \
	callbacks.c \
	linphonecall.c \
	conference.c \
	ec-calibrator.c \
	linphone_tunnel_config.c \
	message_storage.c \
	info.c \
	event.c \
	xml.c \
	xml2lpc.c \
	lpc2xml.c \
	remote_provisioning.c

ifndef LINPHONE_VERSION
LINPHONE_VERSION = "Devel"
endif

LOCAL_CFLAGS += \
	-D_BYTE_ORDER=_LITTLE_ENDIAN \
	-DORTP_INET6 \
	-DINET6 \
	-DENABLE_TRACE \
	-DHAVE_CONFIG_H \
	-DLINPHONE_VERSION=\"$(LINPHONE_VERSION)\" \
	-DLINPHONE_PLUGINS_DIR=\"\\tmp\" \
	-DUSE_BELLESIP

LOCAL_CFLAGS += -DIN_LINPHONE

ifeq ($(_BUILD_VIDEO),1)
LOCAL_CFLAGS += -DVIDEO_ENABLED
ifeq ($(BUILD_X264),1)
LOCAL_CFLAGS += -DHAVE_X264
endif
endif

ifeq ($(BUILD_CONTACT_HEADER),1)
LOCAL_CFLAGS += -DSAL_OP_CALL_FORCE_CONTACT_IN_RINGING
endif

ifeq ($(USE_JAVAH),1)
LOCAL_CFLAGS += -DUSE_JAVAH
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../build/android \
	$(LOCAL_PATH)/../oRTP/include \
	$(LOCAL_PATH)/../mediastreamer2/include \
	$(LOCAL_PATH)/../../belle-sip/include \
	$(LOCAL_PATH)/../../../gen \
	$(LOCAL_PATH)/../../externals/libxml2/include \
	$(LOCAL_PATH)/../../externals/build/libxml2

LOCAL_LDLIBS += -llog -ldl

LOCAL_STATIC_LIBRARIES := \
	cpufeatures \
	libmediastreamer2 \
	libortp \
	libbellesip \
	libgsm \
	liblpxml2


ifeq ($(BUILD_TUNNEL),1)
LOCAL_CFLAGS +=-DTUNNEL_ENABLED
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../tunnel/include $(LOCAL_PATH)/../../tunnel/src
LOCAL_SRC_FILES +=  linphone_tunnel.cc TunnelManager.cc
LOCAL_STATIC_LIBRARIES += libtunnelclient
else
LOCAL_SRC_FILES += linphone_tunnel_stubs.c
endif


_BUILD_AMR=0
ifneq ($(BUILD_AMRNB), 0)
_BUILD_AMR=1
endif

ifneq ($(BUILD_AMRWB), 0)
_BUILD_AMR=1
endif

ifneq ($(_BUILD_AMR), 0)
LOCAL_CFLAGS += -DHAVE_AMR
LOCAL_STATIC_LIBRARIES += \
        libmsamr \
        libopencoreamr
endif

ifneq ($(BUILD_AMRWB), 0)
LOCAL_STATIC_LIBRARIES += \
	libvoamrwbenc
endif


ifeq ($(BUILD_SILK),1)
LOCAL_CFLAGS += -DHAVE_SILK
LOCAL_STATIC_LIBRARIES += libmssilk
endif

ifeq ($(BUILD_WEBRTC_ISAC),1)
LOCAL_CFLAGS += -DHAVE_ISAC
LOCAL_STATIC_LIBRARIES += libwebrtc_isacfix_neon
LOCAL_STATIC_LIBRARIES += libwebrtc_spl libwebrtc_isacfix libmsisac
endif

ifeq ($(BUILD_G729),1)
LOCAL_CFLAGS += -DHAVE_G729
LOCAL_STATIC_LIBRARIES += libbcg729 libmsbcg729
endif

ifeq ($(_BUILD_VIDEO),1)
LOCAL_LDLIBS    += -lGLESv2
LOCAL_STATIC_LIBRARIES += libvpx
ifeq ($(BUILD_X264),1)
LOCAL_STATIC_LIBRARIES += \
	libmsx264 \
	libx264
endif
endif

ifeq ($(BUILD_UPNP),1)
LOCAL_CFLAGS += -DBUILD_UPNP
LOCAL_SRC_FILES += upnp.c
endif

LOCAL_STATIC_LIBRARIES += libspeex 

ifeq ($(BUILD_SRTP), 1)
	LOCAL_C_INCLUDES += $(SRTP_C_INCLUDE)
endif

ifneq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_CFLAGS += -DHAVE_ILBC=1
LOCAL_STATIC_LIBRARIES += libmsilbc
endif

LOCAL_C_INCLUDES += $(LIBLINPHONE_EXTENDED_C_INCLUDES)
LOCAL_WHOLE_STATIC_LIBRARIES += $(LIBLINPHONE_EXTENDED_STATIC_LIBS)
LOCAL_SRC_FILES  += $(LIBLINPHONE_EXTENDED_SRC_FILES)
LOCAL_CFLAGS += $(LIBLINPHONE_EXTENDED_CFLAGS)


ifeq ($(BUILD_GPLV3_ZRTP),1)
	LOCAL_SHARED_LIBRARIES += libssl-linphone libcrypto-linphone
	LOCAL_SHARED_LIBRARIES += libzrtpcpp
endif

ifeq ($(BUILD_SRTP),1)
	LOCAL_SHARED_LIBRARIES += libsrtp
endif

ifeq ($(BUILD_SQLITE),1)
LOCAL_CFLAGS += -DMSG_STORAGE_ENABLED
LOCAL_STATIC_LIBRARIES += liblinsqlite
LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../../externals/sqlite3/
endif

ifeq ($(BUILD_OPUS),1)
LOCAL_STATIC_LIBRARIES += libopus
endif
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES) 
LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS) 

ifeq ($(_BUILD_VIDEO),1)
LOCAL_SHARED_LIBRARIES += \
	libavcodec-linphone \
	libswscale-linphone \
	libavutil-linphone
endif

LOCAL_MODULE := liblinphone
LOCAL_MODULE_FILENAME := liblinphone-$(TARGET_ARCH_ABI)

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)

