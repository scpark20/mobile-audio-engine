LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos)

LOCAL_MODULE := cocos2dcpp_shared

LOCAL_MODULE_FILENAME := libcocos2dcpp

FILE_LIST := $(wildcard $(LOCAL_PATH)/../../Classes/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/*.cc)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Audio/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Audio/android/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Audio/AudioCommon/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Audio/Plugin/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Component/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Manager/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/ODSocket/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Platform/android/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/UI/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Core/Utils/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/Model/*.cpp)
FILE_LIST += $(wildcard $(LOCAL_PATH)/../../Classes/loader/*.cpp)

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Classes \
					$(LOCAL_PATH)/../../Classes/Core/Audio \
					$(LOCAL_PATH)/../../Classes/Core/Component \
					$(LOCAL_PATH)/../../Classes/Core/Manager \
					$(LOCAL_PATH)/../../Classes/Core/ODSocket \
					$(LOCAL_PATH)/../../Classes/Core/Platform \
					$(LOCAL_PATH)/../../Classes/Core/UI \
					$(LOCAL_PATH)/../../Classes/Core/Utils \
					$(LOCAL_PATH)/../../Classes/Model \
					$(LOCAL_PATH)/../../Classes/loader \
					$(LOCAL_PATH)/../../izsoft/Superpowered

LOCAL_LDLIBS := -ldl -llog -lz -landroid -lOpenSLES

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_ARM_NEON := true

LOCAL_STATIC_LIBRARIES := cocos2dx_static
LOCAL_STATIC_LIBRARIES += tremolo
LOCAL_STATIC_LIBRARIES += libProtobuf
LOCAL_STATIC_LIBRARIES += Superpowered

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)
$(call import-module,../../izsoft/tremolo)
$(call import-module,../../izsoft/protobuf)
$(call import-module,../../izsoft/Superpowered)
