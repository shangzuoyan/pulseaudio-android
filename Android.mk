LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

PULSEAUDIO_TOP := $(abspath $(LOCAL_PATH))

CONFIGURE_CC := $(TARGET_CC)
CONFIGURE_CXX := $(TARGET_CXX)
CONFIGURE_INCLUDES :=
CONFIGURE_LDFLAGS := -lc -ldl

LIB := $(TARGET_OUT_SHARED_LIBRARIES)

ifeq ($(ltdl_TOP),)
ltdl_TOP := $(PULSEAUDIO_TOP)/libtool
endif
ifeq ($(json_c_TOP),)
json_c_TOP := $(PULSEAUDIO_TOP)/json-c
endif
ifeq ($(libsndfile_TOP),)
libsndfile_TOP := $(PULSEAUDIO_TOP)/libsndfile
endif
#ifeq ($(salsa_lib_TOP),)
#salsa_lib_TOP := $(PULSEAUDIO_TOP)/salsa-lib
#endif
ifeq ($(alsa_lib_TOP),)
alsa_lib_TOP := $(PULSEAUDIO_TOP)/alsa-lib
endif
ifeq ($(alsa_utils_TOP),)
alsa_utils_TOP := $(PULSEAUDIO_TOP)/alsa-utils
endif
ifeq ($(pulseaudio_TOP),)
pulseaudio_TOP := $(PULSEAUDIO_TOP)/pulseaudio
endif

CONFIGURE_CC := $(patsubst %,$(PWD)/%,$(TARGET_CC))
CONFIGURE_CXX := $(patsubst %,$(PWD)/%,$(TARGET_CXX))
CONFIGURE_LDFLAGS += -L$(PWD)/$(TARGET_OUT_INTERMEDIATE_LIBRARIES)

CONFIGURE_CFLAGS := \
    -DANDROID \
    -nostdlib -Bdynamic \
    -Wl,-dynamic-linker,/system/bin/linker \
    -Wl,--gc-sections \
    -Wl,-z,nocopyreloc
CONFIGURE_CXXFLAGS := $(CONFIGURE_CFLAGS)

CONFIGURE_LDFLAGS += \
    $(PWD)/$(TARGET_CRTBEGIN_DYNAMIC_O) \
    $(call link-whole-archives,$(PRIVATE_WHOLE_STATIC_LIBRARIES))\
    $(PRIVATE_STATIC_LIBRARIES) \
    $(PWD)/$(TARGET_LIBGCC) \
    $(PRIVATE_SHARED_LIBRARIES) \
		$(PWD)/$(TARGET_CRTEND_O)


CONFIGURE_CPP := $(PWD)/$(TARGET_TOOLS_PREFIX)cpp
CONFIGURE_CXXCPP := $(PWD)/$(TARGET_TOOLS_PREFIX)cpp

CONFIGURE_INCLUDES += \
		$(foreach incdir, $(realpath $(C_INCLUDES) $(TARGET_C_INCLUDES)), \
				-I$(incdir))

CONFIGURE_CPPFLAGS := \
	$(CONFIGURE_INCLUDES)
CONFIGURE_CXXCPPFLAGS := \
	$(CONFIGURE_CPPFLAGS)

#CONFIGURE_PKG_CONFIG_LIBDIR := $(json_c_TOP):$(libsndfile_TOP):$(salsa_lib_TOP)
CONFIGURE_PKG_CONFIG_LIBDIR := $(json_c_TOP):$(libsndfile_TOP):$(alsa_lib_TOP)/utils

PKG_CONFIG := PKG_CONFIG_LIBDIR=$(CONFIGURE_PKG_CONFIG_LIBDIR) PKG_CONFIG_TOP_BUILD_DIR="/" pkg-config

CONFIGURE := autogen.sh

.SECONDARYEXPANSION:
PA_CONFIGURE_TARGETS :=

ifeq ($(TARGET_ARCH),x86)
CONFIGURE_HOST := i686-android-linux
endif
ifeq ($(TARGET_ARCH),arm)
CONFIGURE_HOST := arm-linux-androideabi
endif

include $(ltdl_TOP)/Android.mk
include $(json_c_TOP)/Android.configure.mk
include $(libsndfile_TOP)/Android.mk
#include $(salsa_lib_TOP)/Android.mk
include $(alsa_lib_TOP)/Android.mk
include $(alsa_utils_TOP)/Android.mk
include $(pulseaudio_TOP)/Android.mk
include $(PULSEAUDIO_TOP)/tests/Android.mk

#include $(PULSEAUDIO_TOP)/libslang/Android.mk
#include $(PULSEAUDIO_TOP)/powertop/Android.mk

TARGETS:
	@echo $(PA_CONFIGURE_TARGETS)

.PHONY: pulseaudio-aggregate-configure
pulseaudio-aggregate-configure: $(TARGET_CRTBEGIN_DYNAMIC_O) $(TARGET_CRTEND_O) $(LIB)/libc.so $(PA_CONFIGURE_TARGETS)
