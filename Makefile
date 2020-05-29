#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := cuv

include $(IDF_PATH)/make/project.mk

ifeq ($(FEATURE), 1)  #at this point, the makefile checks if FEATURE is enabled
	OPTS = -DINCLUDE_FEATURE #variable passed to g++
endif

object:
	g++ $(OPTS) Sd.c -o executable #OPTS may contain -DINCLUDE_FEATURE

