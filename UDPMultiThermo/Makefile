BOARD_TAG    = mega # uno
BOARD_SUB    = atmega2560
ARDUINO_LIBS = SPI Ethernet SD DS3231M Wire OneWire DallasTemperature NTPClient

#CXXFLAGS += -Wall -Wextra -pedantic -Wshadow -Wconversion
CFLAGS += -std=gnu99
#CPPFLAGS += -O3 -DVERSION=\"$(shell LC_ALL=C hg tip --template '{rev}'|sed -e 's/ //')\" -DVERSIONPLUS=\"$(shell LC_ALL=C \( hg identify -n ; echo -n ":" ; hg identify --id \)| tr -d "\n")\" -DLASTDATE=\"$(shell LC_ALL=C hg tip --template '{date|isodate}'|sed -e 's/ /T/'|sed -e 's/ //')\" -D__TIMESTAMPISO__=\"$(shell date +%Y-%m-%dT%H:%M:%S.%NZ)\" -DCPP_VERSION=\"$(shell LC_ALL=C $(CXX) -dumpversion)\" -DCPP_PATH=\"$(shell LC_ALL=C which $(CXX))\"

CPPFLAGS += -O3 -DVERSION=\"$(shell LC_ALL=C hg tip --template '{rev}' ; echo -n ":" ; hg identify --id)\" -DLASTDATE=\"$(shell LC_ALL=C hg tip --template '{date|isodate}'|sed -e 's/ /T/'|sed -e 's/ //')\" -D__TIMESTAMPISO__=\"$(shell date +%Y-%m-%dT%H:%M:%S.%NZ)\" -DCPP_VERSION=\"$(shell LC_ALL=C $(CXX) -dumpversion)\" -DCPP_PATH=\"$(shell LC_ALL=C which $(CXX))\"
CPPFLAGS += -DNO_RTC
#CPPFLAGS += -DDEBUG

include /usr/share/arduino/Arduino.mk

# target: display - Display build parameters.
display: $(TARGET_ELF)
	strings $^|grep -E "(source_version|source_versionplus|source_lastdate|build_timestamp|compiler_version|comp_version|compiler_path):"|sort
	size $^

# target: info - Display callable targets.
info::
	@egrep "^# target:" $(MAKEFILE_LIST)

# target: validate_typo - check number of typo mistakes ridden files, by category.
validate_typo: $(LOCAL_SRCS)
	grep -Er '[[:alnum:]]\(' $^|wc  # glued parenthesis
	grep -Er ',[[:alnum:]]'  $^|wc  # glued comma
	grep -Er '\( +' $^|wc           # wrongly spaced parenthesis
	grep -Er ' +\)' $^|wc           # wrongly spaced parenthesis
	grep -Er ' +,'  $^|wc           # wrongly spaced comma
	grep -r '	' $^|wc          # tabulation
	grep -Er ' +$$'  $^|wc          # trailing white-space
	grep -Er '	+$$'  $^|wc      # trailing tab
	grep -Er '\)[[:alnum:]]' $^|wc  # glued parenthesis
	grep -Er '\)[^[:blank:]\.,;\)\"\-]' $^|wc  # glued parenthesis
	grep '.\{73\}' $^|wc          # any lines over 72 characters
