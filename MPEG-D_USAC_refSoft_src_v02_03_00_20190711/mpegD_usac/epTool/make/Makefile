#-*-makefile-*-
# Usage: gmake [<general options>] [MODE=_pur]

# include the general makefiles using this directive
GENERALROOT=../../../
MAKEROOT=../
-include $(GENERALROOT)/general/makefile.all

ifneq "$(MAKELEVEL)" "0"

ifneq "$(MODE)" "_pur"
override DEFFLAGS += -DINTEGRATED
endif

ADDONOBJDIR += $(MODE)

endif


ifeq "$(MAKELEVEL)" "0"

OBJS_BASE = eptool.o outinfo.o bintoint.o srcpc2.o crc.o bch.o dcm_io.o interleave.o reedsolomon.o concat.o
OBJS_ADD  = epenc.o epdec.o cfgparse.o sideconv.o rsideconv.o concatenation.o Rconcatenation.o epenc_old.o epdec_old.o
OBJS_LIB  = libeptool.o calcoverhead.o
OBJS_ADDLIB = sideinfoparse.o feedframeclass.o
OBJS_ALL  = $(OBJS_LIB) $(OBJS_ADD) $(OBJS_BASE) $(OBJS_ADDLIB)

TARGET_LIB     = libeptool.a
TARGET_OLD_PUR = epenc_old_pur epdec_old_pur cfgparse_pur
TARGET_PUR     = epenc_pur epdec_pur
TARGET_SLINK   = predconv infoconv rinfoconv
TARGET_ADD     = $(OBJS_ADD:%.o=%) $(TARGET_SLINK)
TARGET_ALL     = $(TARGET_LIB) $(TARGET_ADD) $(TARGET_OLD_PUR) $(TARGET_PUR)
TARGET_REQ     = epenc epdec concatenation Rconcatenation

LIBS    += -leptool
SRCDIRS += ../src
OPTIONS += SRCDIRS="$(SRCDIRS)" DEFFLAGS="$(DEFFLAGS)" INCLDEP=1

.PHONY: all $(TARGET_ALL) clean

all: $(TARGET_REQ)

reallyall: $(TARGET_ALL)

$(TARGET_LIB):
	@$(MAKE) TARGET=$@ $(OPTIONS) OBJS="$(OBJS_LIB) $(OBJS_BASE) $(OBJS_ADDLIB)" library

$(TARGET_PUR): $(TARGET_LIB)
	@$(MAKE) TARGET=$@ $(OPTIONS) LIBS="-leptool" MODE=_pur OBJS="$(subst _pur,,$@).o" binary

epenc epdec: $(TARGET_LIB)
	@$(MAKE) TARGET=$@ $(OPTIONS) LIBS="-leptool" OBJS="$@.o" binary

concatenation Rconcatenation:
	@$(MAKE) TARGET=$@ $(OPTIONS) OBJS="$@.o outinfo.o dcm_io.o" binary

# old targets, only available for compatibility
$(TARGET_OLD_PUR):
	@$(MAKE) TARGET=$@ $(OPTIONS) MODE=_pur OBJS="$(subst _pur,,$@).o $(OBJS_BASE)" binary

epenc_old epdec_old cfgparse rsideconv:
	@$(MAKE) TARGET=$@ $(OPTIONS) OBJS="$@.o $(OBJS_BASE)" binary

sideconv:
	@$(MAKE) TARGET=$@ $(OPTIONS) OBJS="$@.o outinfo.o" binary

predconv infoconv: sideconv
	@$(MAKE) TARGET=$@ BINARY=$< binary

rinfoconv: rsideconv
	@$(MAKE) TARGET=$@ BINARY=$< binary
# end of old targets

clean:
	$(MAKE)           TARGET="$(TARGET_ALL)" OBJS="$(OBJS_ALL)" clean_mode
	$(MAKE) MODE=_pur TARGET="$(TARGET_ALL)" OBJS="$(OBJS_ALL)" clean_mode

else # ifeq "$(MAKELEVEL)" "0"

clean_mode:
	@$(MAKE)          TARGET="$(TARGET)"     OBJS="$(OBJS)" clean

endif # ifeq "$(MAKELEVEL)" "0"

