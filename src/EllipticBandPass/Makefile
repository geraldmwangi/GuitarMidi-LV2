#!/usr/bin/make -f
# Makefile for StateVariableFilter-DPF Plugin #
# ---------------------------- #
# Created by JimsonDrift, Based on Distrho Plugins by falktx
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_ellipticbandpass

# --------------------------------------------------------------
# Files to build

OBJS_DSP = \
	ellipticbandpass.cpp.o ../../DSPFilters/shared/DSPFilters/source/Cascade.cpp.o\
	../../DSPFilters/shared/DSPFilters/source/Biquad.cpp.o ../../DSPFilters/shared/DSPFilters/source/Elliptic.cpp.o\
	../../DSPFilters/shared/DSPFilters/source/PoleFilter.cpp.o



# --------------------------------------------------------------
# Do some magic

include ../Makefile.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(BUILD_JACK),true)
ifeq ($(HAVE_JACK),true)
TARGETS += jack
endif
endif

ifeq ($(BUILD_LADSPA),true)
TARGETS += ladspa
endif

ifeq ($(BUILD_DSSI),true)
ifeq ($(HAVE_DGL),true)
ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif
endif
endif

ifeq ($(BUILD_LV2),true)
ifeq ($(HAVE_DGL),true)
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif
endif

ifeq ($(BUILD_VST2),true)
TARGETS += vst
endif

all: $(TARGETS)

# --------------------------------------------------------------
