#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
CFLAGS += -Wno-format

ARDUINO_ULIBRARIES_LIST := $(patsubst $(COMPONENT_PATH)/../lib/%,%,$(wildcard $(COMPONENT_PATH)/../lib/*))
ARDUINO_SINGLE_ULIBRARY_FILES = $(patsubst $(COMPONENT_PATH)/%,%,$(sort $(dir $(wildcard $(COMPONENT_PATH)/../lib/$(MODULE)/*)) $(dir $(wildcard $(COMPONENT_PATH)/../lib/$(MODULE)/src/*/)) $(dir $(wildcard $(COMPONENT_PATH)/../lib/$(MODULE)/src/*/*/)) $(dir $(wildcard $(COMPONENT_PATH)/../lib/$(MODULE)/src/*/*/*/)) $(dir $(wildcard $(COMPONENT_PATH)/../lib/$(MODULE)/src/*/*/*/*/))))
ARDUINO_USER_LIBS := $(foreach MODULE,$(ARDUINO_ULIBRARIES_LIST), $(ARDUINO_SINGLE_ULIBRARY_FILES))

COMPONENT_ADD_INCLUDEDIRS := $(ARDUINO_USER_LIBS) main .
COMPONENT_SRCDIRS := $(ARDUINO_USER_LIBS) main .
