#-*-makefile-*-
#for spatial (sub)project
#ifdef MPEG4V1
# Usage: gmake [<general options>] [USE_AFSP=0] [ENABLE_RTPLAY=1] [ISOMP4=0] [RESET_TEST=1] [TORTURE_TEST=1] [PNS_NONDETERMINISTIC_RANDOM=1] [SBR=0] [LOW_POWER_SBR=1] [PARAMETRICSTEREO=0]
#else
# Usage: gmake [<general options>] [USE_AFSP=0] [ENABLE_RTPLAY=1] [RESET_TEST=1] [TORTURE_TEST=1] [PNS_NONDETERMINISTIC_RANDOM=1] [SBR=0] [LOW_POWER_SBR=1] [PARAMETRICSTEREO=0]
#endif

# include the general makefiles using this directive
GENERALROOT=../../../
MAKEROOT=../
-include $(GENERALROOT)/general/makefile.all

TOTAL_PATH_ADD =

ifndef USE_AFSP
USE_AFSP = 1
endif

ifneq "$(USE_AFSP)" "0"
TOTAL_PATH_ADD += AFsp
endif


ifneq  "$(MAKELEVEL)" "0"
override  MODULELIBS += -lmp4spatialenc
endif # ifneq  "$(MAKELEVEL)" "0"

override INCL := $(INCL) -I../include -I../src

ifeq "$(MAKELEVEL)" "0"

# Common
OBJS = sac_enc.o sac_polyphase.o sac_hybfilter.o sac_huff_cld.o sac_huff_cpc.o sac_huff_icc.o sac_stream.o sac_nlc_enc.o bitstream.o sac_bd_embedder.o sac_ipd.o sac_huff_ipd.o

OBJS_SAENC = sac_encoder.o
OBJS_LIB       = $(OBJS)
SPATIAL_OBJS_ALL   = $(OBJS_LIB) $(OBJS_SAENC)

ALL_BIN   = sac_encoder
SPATIAL_LIBS = libmp4spatialenc.a

ifeq "$(SPATIALAUDIO)" "1"
COMLIBS  += -lmp4spatialenc
endif

SRCDIRS = ../src
OPTIONS = $(MFLAGS) SRCDIRS="$(SRCDIRS)" USE_AFSP=$(USE_AFSP) COMLIBS="$(COMLIBS)" EXTERNLIBS="$(EXTERNLIBS)" DEFFLAGS="$(DEFFLAGS)" LIBS="$(LIBS)" INCL="$(INCL)" INCLDEP=1

.PHONY: all allafsp $(ALL_BIN) $(SPATIAL_LIBS)  clean cleanafsp

all: $(ALL_BIN)

allafsp:
	$(MAKE) USE_AFSP=0 all
	$(MAKE) USE_AFSP=1 all


libmp4spatialenc.a:
	$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_LIB)" library

sac_encoder: $(SPATIAL_LIBS)
	$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_SAENC)" binary

clean:
	@$(MAKE) TARGET="sac_encoder libmp4spatialenc.a"   OBJS="$(SPATIAL_OBJS_ALL)"  clean_mode

cleanafsp:
	@$(MAKE) USE_AFSP=0 TARGET="sac_encoder libmp4spatialenc.a"  OBJS="$(SPATIAL_OBJS_ALL)"  clean_mode
	@$(MAKE) USE_AFSP=1 TARGET="sac_encoder libmp4spatialenc.a"  OBJS="$(SPATIAL_OBJS_ALL)"  clean_mode

else # ifeq "$(MAKELEVEL)" "0"

clean_mode:
	@$(MAKE) USE_AFSP=$(USE_AFSP) TARGET="$(TARGET)" OBJS="$(OBJS)" clean

endif # ifeq "$(MAKELEVEL)" "0"
