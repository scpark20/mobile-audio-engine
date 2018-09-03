LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := Superpowered
LOCAL_SRC_FILES := ./android/libSuperpoweredARM.a

include $(PREBUILT_STATIC_LIBRARY)
