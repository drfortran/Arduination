BOARD_TAG    = mega # uno
BOARD_SUB    = atmega2560
ARDUINO_LIBS = SPI Ethernet

CXXFLAGS += -Wall -Wextra -pedantic -Wconversion -Wshadow
# CPPFLAGS += -O3 -DVERSION=\"$(shell LC_ALL=C hg tip --template '{parent}'|sed -e 's/ //')\" -DLASTDATE=\"$(shell LC_ALL=C hg tip --template '{date|isodate}'|sed -e 's/ /T/'|sed -e 's/ //')\" -D__TIMESTAMPISO__=\"$(shell date +%Y-%m-%dT%H:%M:%S.%NZ)\" -DCPP_VERSION=\"$(shell LC_ALL=C $(CXX) -dumpversion)\" -DCPP_PATH=\"$(shell LC_ALL=C which $(CXX))\"
CPPFLAGS += -O3 -DVERSION=\"$(shell LC_ALL=C hg tip --template '{rev}' ; echo -n ":" ; hg identify --id)\" -DLASTDATE=\"$(shell LC_ALL=C hg tip --template '{date|isodate}'|sed -e 's/ /T/'|sed -e 's/ //')\" -D__TIMESTAMPISO__=\"$(shell date +%Y-%m-%dT%H:%M:%S.%NZ)\" -DCPP_VERSION=\"$(shell LC_ALL=C $(CXX) -dumpversion)\" -DCPP_PATH=\"$(shell LC_ALL=C which $(CXX))\"

include /usr/share/arduino/Arduino.mk

# target: display - Display build parameters.
display: $(TARGET_ELF)
	strings $^|grep -E "(source_version|source_versionplus|source_lastdate|build_timestamp|compiler_version|comp_version|compiler_path):"|sort
	size $^

# target: info - Display callable targets.
info::
	@egrep "^# target:" $(MAKEFILE_LIST)
