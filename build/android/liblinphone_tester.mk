LOCAL_PATH := $(call my-dir)/../../tester

common_SRC_FILES := \
	call_tester.c \
	liblinphone_tester.c \
	message_tester.c \
	presence_tester.c \
	register_tester.c \
	setup_tester.c \
	upnp_tester.c \
	eventapi_tester.c \
	stun_tester.c \
	flexisip_tester.c \
	tester.c \
	remote_provisioning_tester.c \
	quality_reporting_tester.c \
	log_collection_tester.c \
	transport_tester.c \
	player_tester.c \
	dtmf_tester.c \
	accountmanager.c

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
LOCAL_CFLAGS = -DIN_LINPHONE
LOCAL_LDLIBS := -llog

ifeq ($(BUILD_MATROSKA), 1)
LOCAL_CFLAGS += -DHAVE_MATROSKA
endif

LOCAL_SHARED_LIBRARIES := cunit liblinphone
include $(BUILD_SHARED_LIBRARY)

#end
