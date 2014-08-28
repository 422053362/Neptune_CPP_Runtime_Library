
#Create by Jamie.Yan


LOCAL_PATH := $(call my-dir)

#Build Neptune C++ Runtime Library
#

include $(CLEAR_VARS)
LOCAL_MODULE := libNeptune
LOCAL_SRC_FILES := ${wildcard ../../../../Source/Core/*.cpp ../../../../Source/Data/TLS/*.cpp ../../../../Source/System/Android/*.cpp ../../../../Source/System/Bsd/*.cpp ../../../../Source/System/Null/NptNullSerialPort.cpp ../../../../Source/System/Posix/*.cpp ../../../../Source/System/StdC/NptStdc[!D]*.cpp ../../../../ThirdParty/axTLS/crypto/*.c ../../../../ThirdParty/axTLS/ssl/*.c ../../../../ThirdParty/zlib-1.2.3/*.c ../../../../ThirdParty/android-ifaddrs/*.c}
LOCAL_C_INCLUDES += ../../../../Source/Core ../../../../ThirdParty/axTLS/crypto ../../../../ThirdParty/axTLS/config/Generic ../../../../ThirdParty/axTLS/ssl ../../../../ThirdParty/zlib-1.2.3 ../../../../ThirdParty/android-ifaddrs}
LOCAL_CFLAGS := -DHAVE_USLEEP -DNPT_CONFIG_ENABLE_ZIP -DNPT_CONFIG_ENABLE_TLS -DNPT_CONFIG_HAVE_SYSTEM_LOG_CONFIG -DNPT_DEBUG -I../../../../ThirdParty/android-ifaddrs
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
