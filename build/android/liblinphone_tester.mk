LOCAL_PATH := $(call my-dir)/../../tester

common_SRC_FILES := \
	call_tester.c \
	liblinphone_tester.c \
	message_tester.c \
	presence_tester.c \
	register_tester.c \
	setup_tester.c \
	upnp_tester.c \
	eventapi_tester.c
 
# neon
common_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../coreapi \
        $(LOCAL_PATH)/../oRTP/include \
        $(LOCAL_PATH)/../mediastreamer2/include 


include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_MODULE := liblinphone_tester 
LOCAL_SRC_FILES += $(common_SRC_FILES) 
LOCAL_C_INCLUDES = $(common_C_INCLUDES)
LOCAL_CFLAGS = -DIN_LINPHONE
LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := cunit liblinphone
include $(BUILD_SHARED_LIBRARY)
endif

# noneon

include $(CLEAR_VARS)

LOCAL_MODULE := liblinphone_testernoneon 
ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_MODULE_FILENAME := liblinphone_testerarmv5
endif
LOCAL_SRC_FILES += $(common_SRC_FILES) 
LOCAL_C_INCLUDES = $(common_C_INCLUDES)
LOCAL_CFLAGS = -DIN_LINPHONE
LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := cunit liblinphone
include $(BUILD_SHARED_LIBRARY)

#end
