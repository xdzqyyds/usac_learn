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

ifndef PARTIALLY_COMPLEX
PARTIALLY_COMPLEX = 0
endif

ifndef HRTF_DYNAMIC_UPDATE
HRTF_DYNAMIC_UPDATE = 1
endif

ifneq "$(PARTIALLY_COMPLEX)" "0"
TOTAL_PATH_ADD += lpmps
endif


ifndef USE_AFSP
USE_AFSP = 1
endif

ifneq "$(USE_AFSP)" "0"
INCL+= -I$(GENERALROOT)/tools/AFsp/include
endif

ifndef ENABLE_RTPLAY
ENABLE_RTPLAY = 0
endif

ifneq "$(ENABLE_RTPLAY)" "0"
USE_AFSP = 0
TOTAL_PATH_ADD += RTPlay
endif

ifneq  "$(MAKELEVEL)" "0"

override ADDONDIR = $(TOTAL_PATH_ADD)
override  MODULELIBS += -lmp4spatialdec

endif # ifneq  "$(MAKELEVEL)" "0"


override INCL := $(INCL) -I../include -I../src -I../../usacEncDec/src_frame -I../../usacEncDec/include -I../../usacEncDec/src_ic

ifeq "$(MAKELEVEL)" "0"

# Common
OBJS = sac_dec.o sac_polyphase.o sac_bitdec.o sac_process.o sac_reshuffdec1.o \
	sac_reshuffdec2.o sac_reshuffdec3.o sac_reshuffinit.o sac_bitinput.o \
	sac_calcM1andM2.o sac_resdecode.o sac_config.o sac_reshufftables.o \
	sac_resintrins.o sac_restns.o sac_hybfilter.o sac_smoothing.o \
	sac_tonality.o sac_mdct2qmf.o sac_decor.o \
	sac_nlc_dec.o sac_TPprocess.o sac_mdct2qmf_wf_tables.o \
	sac_nodes_cpc.o sac_nodes_icc.o sac_nodes_cld.o sac_nodes_pt0.o \
	sac_reshapeBBEnv.o sac_nodes_reshape.o sac_blind.o \
	sac_hrtffilter.o sac_hrtf.o \
	sac_arbitraryTree.o sac_parallelReverb.o sac_partiallyComplex.o \
	sac_nodes_ipd.o sac_ipd_dec.o

OBJS_SADEC = spatial_frontend.o
OBJS_LIB       = $(OBJS)
SPATIAL_OBJS_ALL   = $(OBJS_LIB) $(OBJS_SADEC)

ifneq "$(ENABLE_RTPLAY)" "0"
override DEFFLAGS += -DENABLE_RTPLAY
EXTERNLIBS  += -laudio
endif

ALL_BIN   = sac_dec_standalone
SPATIAL_LIBS = libmp4spatialdec.a

ifeq "$(SPATIALAUDIO)" "1"
COMLIBS  += -lmp4spatialdec
endif

ifneq "$(SPACE_DEBUG)" "0" 
override DEFFLAGS += -DSPACE_DEBUG
endif

# kk20050330
override DEFFLAGS += -DTP_BUG_FIX

ifeq "$(PARTIALLY_COMPLEX)" "1"
override DEFFLAGS += -DPARTIALLY_COMPLEX
endif	

ifeq "$(HRTF_DYNAMIC_UPDATE)" "1"
override DEFFLAGS += -DHRTF_DYNAMIC_UPDATE
endif	

SRCDIRS = ../src
OPTIONS = $(MFLAGS) SRCDIRS="$(SRCDIRS)" USE_AFSP=$(USE_AFSP) COMLIBS="$(COMLIBS)" EXTERNLIBS="$(EXTERNLIBS)" DEFFLAGS="$(DEFFLAGS)" LIBS="$(LIBS)" INCL="$(INCL)" INCLDEP=1

.PHONY: all allafsp $(ALL_BIN) $(SPATIAL_LIBS)  clean cleanafsp

all: $(ALL_BIN)

allafsp:
	$(MAKE) USE_AFSP=0 all
	$(MAKE) USE_AFSP=1 all


libmp4spatialdec.a:
	$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_LIB)" library

sac_dec_standalone: $(SPATIAL_LIBS)
	$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_SADEC)" binary

clean:
	@$(MAKE) TARGET="spatialdec libmp4spatialdec.a"   OBJS="$(SPATIAL_OBJS_ALL)"  clean_mode

cleanafsp:
	@$(MAKE) USE_AFSP=0 TARGET="spatialdec libmp4spatialdec.a"  OBJS="$(SPATIAL_OBJS_ALL)"  clean_mode
	@$(MAKE) USE_AFSP=1 TARGET="spatialdec libmp4spatialdec.a"  OBJS="$(SPATIAL_OBJS_ALL)"  clean_mode

else # ifeq "$(MAKELEVEL)" "0"

clean_mode:
	@$(MAKE) USE_AFSP=$(USE_AFSP) TARGET="$(TARGET)" OBJS="$(OBJS)" clean

endif # ifeq "$(MAKELEVEL)" "0"
