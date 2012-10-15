# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
# http://www.gnu.org/software/make/manual/make.html
#

#
# Project information
#
# These variables store project specific settings for the project name
# build flags, files to copy or install.  In the examples it is typically
# only the list of sources and project name that will actually change and
# the rest of the makefile is boilerplate for defining build rules.
#
PROJECT:=decrypt_revelation
# -lnosys is to prevent a linker error: signalr.c:(.text+0x3c): undefined # -reference to `kill'
LDFLAGS:=-lppapi_cpp -lppapi -lnosys
CXX_SOURCES:=$(PROJECT).cc


#
# Get pepper directory for toolchain and includes.
#
# If NACL_SDK_ROOT is not set, then assume it can be found a two directories up,
# from the default example directory location.
#
THIS_MAKEFILE:=$(abspath $(lastword $(MAKEFILE_LIST)))
NACL_SDK_ROOT?=$(abspath $(dir $(THIS_MAKEFILE))../..)

# Project Build flags
WARNINGS:=-Wno-long-long -Wall -Wswitch-enum -pedantic -Werror
CXXFLAGS:=-pthread -std=gnu++98 $(WARNINGS)

#
# Compute tool paths
#
#
OSNAME:=$(shell python $(NACL_SDK_ROOT)/tools/getos.py)
TC_PATH:=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_newlib)
CXX:=$(TC_PATH)/bin/i686-nacl-g++

TC_LIB_PATH_32:=$(TC_PATH)/i686-nacl/usr/lib
TC_LIB_PATH_64:=$(TC_PATH)/x86_64-nacl/usr/lib
TC_INCLUDE_PATH_32:=$(TC_PATH)/i686-nacl/usr/include
TC_INCLUDE_PATH_64:=$(TC_PATH)/x86_64-nacl/usr/include
TC_LIBS_32:=$(TC_LIB_PATH_32)/libz.a $(TC_LIB_PATH_32)/libtomcrypt.a
TC_LIBS_64:=$(TC_LIB_PATH_64)/libz.a $(TC_LIB_PATH_64)/libtomcrypt.a

#
# Disable DOS PATH warning when using Cygwin based tools Windows
#
CYGWIN ?= nodosfilewarning
export CYGWIN


# Declare the ALL target first, to make the 'all' target the default build
all: $(PROJECT)_x86_32.nexe $(PROJECT)_x86_64.nexe

clean:
	rm -rf *.o *.nexe

# Define 32 bit compile and link rules for main application
x86_32_OBJS:=$(patsubst %.cc,%_32.o,$(CXX_SOURCES))
$(x86_32_OBJS) : %_32.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m32 -O0 -g $(CXXFLAGS) -I$(TC_INCLUDE_PATH_32)

$(PROJECT)_x86_32.nexe : $(x86_32_OBJS) $(TC_LIBS_32)
	$(CXX) -o $@ $^ -m32 -O0 -g $(CXXFLAGS) $(LDFLAGS)

# Define 64 bit compile and link rules for C++ sources
x86_64_OBJS:=$(patsubst %.cc,%_64.o,$(CXX_SOURCES))
$(x86_64_OBJS) : %_64.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m64 -O0 -g $(CXXFLAGS) -I$(TC_INCLUDE_PATH_64)

$(PROJECT)_x86_64.nexe : $(x86_64_OBJS) $(TC_LIBS_64)
	$(CXX) -o $@ $^ -m64 -O0 -g $(CXXFLAGS) $(LDFLAGS)

# Define a phony rule so it always runs, to build nexe and start up server.
.PHONY: RUN 
RUN: all
	python ../httpd.py
