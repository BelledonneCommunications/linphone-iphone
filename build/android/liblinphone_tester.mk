LOCAL_PATH := $(call my-dir)/../../tester

common_SRC_FILES := \
	accountmanager.c \
	call_tester.c \
	dtmf_tester.c \
	eventapi_tester.c \
	flexisip_tester.c \
	liblinphone_tester.c \
	log_collection_tester.c \
	message_tester.c \
	multi_call_tester.c \
	offeranswer_tester.c \
	player_tester.c \
	presence_tester.c \
	proxy_config_tester.c \
	quality_reporting_tester.c \
	register_tester.c \
	remote_provisioning_tester.c \
	setup_tester.c \
	stun_tester.c \
	tester.c \
	tunnel_tester.c \
	upnp_tester.c \
 	multicast_call_tester.c \
 	vcard_tester.c \
	complex_sip_call_tester.c \

common_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../coreapi \
        $(LOCAL_PATH)/../oRTP/include \
        $(LOCAL_PATH)/../mediastreamer2/include 


include $(CLEAR_VARS)

LOCAL_MODULE := liblinphone_tester
LOCAL_MODULE_FILENAME := liblinphone_tester-$(TARGET_ARCH_ABI)
LOCAL_SRC_FILES += $(common_SRC_FILES)
LOCAL_C_INCLUDES = $(common_C_INCLUDES)
LOCAL_CFLAGS = -DIN_LINPHONE -DBC_CONFIG_FILE=\"config.h\"
LOCAL_LDLIBS := -llog -lz

ifeq ($(BUILD_MATROSKA), 1)
LOCAL_CFLAGS += -DHAVE_MATROSKA -DHAVE_ZLIB
endif

LOCAL_STATIC_LIBRARIES := bctoolbox_tester

LOCAL_SHARED_LIBRARIES := bcunit liblinphone
include $(BUILD_SHARED_LIBRARY)

#end
