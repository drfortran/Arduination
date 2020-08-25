BOARD_TAG    = uno
ARDUINO_LIBS = SPI Ethernet

# -Wall -Wextra -pedantic
CPPFLAGS += -O3 -DVERSION=\"$(shell LC_ALL=C hg tip --template '{parent}'|sed -e 's/ //')\" -DLASTDATE=\"$(shell LC_ALL=C hg tip --template '{date|isodate}'|sed -e 's/ /T/'|sed -e 's/ //')\" -D__TIMESTAMPISO__=\"$(shell date +%Y-%m-%dT%H:%M:%S.%NZ)\" -DCPP_VERSION=\"$(shell LC_ALL=C $(CXX) -dumpversion)\" -DCPP_PATH=\"$(shell LC_ALL=C which $(CXX))\"

include /usr/share/arduino/Arduino.mk

# target: display - Display build parameters.
display: $(TARGET_ELF)
	strings $^|grep -E "(source_version|source_lastdate|build_timestamp|compiler_version|comp_version|compiler_path):"|sort
	size $^

# target: info - Display callable targets.
info::
	@egrep "^# target:" $(MAKEFILE_LIST)
