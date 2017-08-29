
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= zs_touch.c

LOCAL_SHARED_LIBRARIES := libcrypto libcutils libselinux

LOCAL_MODULE:= touchser


#LOCAL_POST_INSTALL_CMD := $(hide) $(foreach t,$(filter-out $(LOCAL_MODULE),$(ALL_TOOLS)),ln -sf $(LOCAL_MODULE) $(TARGET_OUT)/bin/$(t);)
include $(BUILD_EXECUTABLE)
