#-*-makefile-*-
#------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile (requires GNU make)
#
#
# Authors:
# BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
# HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
# BT    Bodo Teichmann, FhG/IIS <tmn@iis.fhg.de>
# PL    Pierre Lauber, FhG/IIS <lau@iis.fhg.de>
# SE    Sebastien Etienne, CCETT Rennes
# OK    Olaf Kaehler, Fhg/IIS <kaehleof@iis.fhg.de>
# CN    Christian Neukam, Fhg/IIS <christian.neukam@iis.fraunhofer.de>
#
# Changes:
# 11-jun-96   BG    basic concept
# 13-jun-96   HP    first version
# 14-jun-96   HP    test stuff added
# 19-jun-96   HP    indiline.a added
# 20-jun-96   HP    modifications proposed by BG
# 28-jun-96   HP    fixed COPTS bug
# 04-jul-96   HP    restructured makefile, fixed CFLAGS/LIBS bug
# 04-jul-96   HP    included "pre-linked object file directory" by BG
# 05-jul-96   HP    fixed override CFLAGS/LIBS
# 20-aug-96   BG    OS2 modifications
# 26-aug-96   HP    removed PLOBJS, adapted to vm directory structure
# 26-aug-96   HP    CVS
# 13-sep-96   HP    removed -O option for Linux g++
# 25-oct-96   HP    adapted makefile options / added makefile.cfg
# 01-nov-96   HP    added "depend" target
# 03-dec-96   HP    added comment re. include dependency files
#                   added library dependency rules
# 21-jan-97   HP    added audio i/o module
# 31-jan-97   HP    seperated encoder and  -g directories
# 05-feb-97   HP    added COMLIBS
# 12-feb-97   HP    make CFLAGS and LDFLAGS values passed on and evaluated
# 14-feb-97   HP    included PICOLA speed control (by Panasonic)
# 11-mar-97   HP    included PICOLA source code
# 21-mar-97   BT    fixed CC and IRIX things, platform autodetect
# 26-mar-97   CCETT included G729  -g source code
# 20-may-97   HP    clean up
# 22-may-97   HP    added DEBUGPLOT (substitutes PLOTMTV)
# 03-jun-97   HP    disabled DEBUGPLOT as default
# 07-nov-97   HP    merged with FhG mods
# 08-apr-98   HP    added OSF platform
# 09-apr-98   HP/CF added rm before ar when making libs
# 13-oct-98   HP    improved platform detection
# 02-dec-98   HP/nn merged version-dependent directory stuff by FhG
# 11-jan-99   HP    linux_gcc
# 20-jan-99   HP    cleaned up platform dependency stuff and CFLAGS defaults
# 21-jan-99   HP    audio.c & austream.c
# 22-jan-99   HP    USE_AFSP
# 23-apr-99   HP    updated, some files renamed for IM1-compability
# 28-apr-99   HP    using AFSP_INCLUDE_PATH / AFSP_LIBRARY_PATH ...
# 21-mar-00   HP    new default for ISOMP4_PATH
# 27-oct-00   HP    changed CORRIGENDUM1 = 1 to CORRIGENDUM = 1
# 23-jan-01   PL    clean up of refSoft makefiles
# 19-jun-01   OK    removed G723/G729 dummy files
# 26-mar-03   OK    removed fl4_to_mp4, mp4_to_fl4 and latm_convert, replaced with streamfile and convert
# 2003-07-16  HP    USE_SBR=1 -> SBR=1, added "sbr" to path, as in mp4mcDec
# 21-jul-03   AR    Add Ext2 Parametric coding
# 2003-08-27  HP    enabled SBR and SBR_SCALABLE by default
# 2003-11-13  OK    use streamfile objects for encoder
# 2004-01-28  OK    use streamfile objects for decoder
# 2003-04-27  OK    link epTool as library, do not hardwire it
# 2004-10-11  OK    add streamfile_raw
# 2005-08-17  HP    disabled SBR_SCALABLE by default, simplifies testing
# 2017-12-06  CN    rewiring, update libisomediafile library
#----------------------------------------------------------------------

# only variables required in makelevel!=0 should be defined here
# (i.e. outside of the makelevel=0 region)

# include the general makefiles using this directive
GENERALROOT=../../../
MAKEROOT=../
-include $(GENERALROOT)/general/makefile.all

# the following directive is mandatory in order to get all local stuff into this level
ifeq "$(MAKELEVEL)" "0"


ifndef ISOMP4
ISOMP4 = 1
INCL+= -I$(GENERALROOT)/tools/IsoLib/libisomediafile/src/

ifeq "$(TARGET_SYSTEM)" "Linux"
ISOMEDIALIBDIR = linux
ISOMEDIALIBFLAGS = ""
INCL+= -I$(GENERALROOT)/tools/IsoLib/libisomediafile/linux
else
ifeq "$(TARGET_SYSTEM)" "Darwin"
ISOMEDIALIBDIR = macosx
ISOMEDIALIBFLAGS = "-arch $(TARGET_PLATFORM)"
INCL+= -I$(GENERALROOT)/tools/IsoLib/libisomediafile/macosx
endif
endif
endif


ifndef USE_AFSP
USE_AFSP = 1
endif
ifeq "$(USE_AFSP)" "1"
override DEFFLAGS += -DUSE_AFSP
INCL+= -I$(GENERALROOT)/tools/AFsp/include
endif

# CELP Error Concealment
ifndef CELP_EC
CELP_EC = 0
endif
ifeq "$(CELP_EC)" "1"
override DEFFLAGS += -DVERSION2_EC
endif

# 14496-3:1999/COR1:2001
ifndef CORRIGENDUM
CORRIGENDUM = 1
endif
ifeq "$(CORRIGENDUM)" "1"
override DEFFLAGS += -DCORRIGENDUM1
endif

# error sensitivity category (ESC) support
ifndef ESC_SUPPORT
ESC_SUPPORT = 1
endif
ifeq "$(ESC_SUPPORT)" "1"
override DEFFLAGS += -DESC_SUPPORT
endif

# link eptoollib to support epConfig=[23]
ifndef EPTOOLLIB
EPTOOLLIB = 1
endif
ifndef FHG_EPTOOLLIB
FHG_EPTOOLLIB = 0
endif
ifeq "$(FHG_EPTOOLLIB)" "1"
override DEFFLAGS += -DEPCONFIG3 -DFHG_EPTOOL
EPTOOLLIB = 0
endif
ifeq "$(EPTOOLLIB)" "1"
override DEFFLAGS += -DEPCONFIG3
FHG_EPTOOLLIB = 0
endif


# The intention of the variable NO_AL_CONFIG is not exactly clear. The
# corresponding define AL_CONFIG occurs in the encoder source code
# only. It should be defined on default, leaving it out might switch to
# a certain ancient compatibility mode. [sps]

ifndef NO_AL_CONFIG
NO_AL_CONFIG = 0
endif
ifeq "$(NO_AL_CONFIG)" "0"
override DEFFLAGS += -DAL_CONFIG
endif

# Spectral Band Replication
ifndef SBR
SBR = 1
endif
ifeq "$(SBR)" "1"
override DEFFLAGS += -DCT_SBR
endif

ifndef LOW_POWER_SBR
LOW_POWER_SBR = 0
endif
ifeq "$(LOW_POWER_SBR)" "1"
override DEFFLAGS += -DLOW_POWER_SBR
# PS is not permitted in the case of LP (sps@2008-02-28)
override PARAMETRICSTEREO=0
endif

# SBR for AAC scalable
ifndef SBR_SCALABLE
SBR_SCALABLE = 0
endif
ifeq "$(SBR_SCALABLE)" "1"
override DEFFLAGS += -DSBR_SCALABLE
endif

#ifndef PARAMETRICSTEREO
#PARAMETRICSTEREO=1
#endif
#ifeq "$(PARAMETRICSTEREO)" "1"
#override DEFFLAGS += -DPARAMETRICSTEREO
#endif

ifndef SSC
SSC = 0
endif
ifeq "$(SSC)" "1"
override DEFFLAGS += -DEXT2PAR
endif

ifndef SLS
SLS=0
endif
ifeq "$(SLS)" "1"
override DEFFLAGS += -DI2R_LOSSLESS
endif

ifndef AAC_ELD
AAC_ELD=0
endif
ifeq "$(AAC_ELD)" "1"
override DEFFLAGS += -DAAC_ELD
endif

ifndef MPEG12
MPEG12 = 0
endif
ifeq "$(MPEG12)" "1"
override DEFFLAGS += -DMPEG12
endif


# enables features for MPEG-4 Audio Version 2. Should be enabled on
# default.
ifndef VERSION
VERSION = 2
endif
ifeq "$(VERSION)" "2"
override DEFFLAGS += -DVERSION2
endif

# HP 991119
ifeq "$(MOUSECHANGE)" "1"
override DEFFLAGS += -DMOUSECHANGE
LIBPATH  += -L/usr/X11R6/lib
SYSLIBS  += -lX11
ifeq "$(OSTYPE)" "solaris"
INCL    += -I/sol/X11R6/Global/include
LIBPATH += -L/sol/X11R6/Global/lib
SYSLIBS += -lsocket
endif
endif

OBJS_ENC_PAR   = enc_par_dmy.o
OBJS_ENC_LPC   = enc_lpc_dmy.o
OBJS_ENC_TF    = enc_tf_dmy.o
OBJS_ENC_SLS   =
OBJS_DEC_PAR   = dec_par_dmy.o
OBJS_DEC_LPC   = dec_lpc_dmy.o
OBJS_DEC_TF    = dec_tf_dmy.o tf_tables.o 
OBJS_DEC_USAC  = dec_usac_dmy.o

ifndef PAR
PAR = 1
endif
ifeq "$(PAR)" "1"
include makefile.par
endif

ifndef LPC
LPC = 1
endif
ifeq "$(LPC)" "1"
include makefile.lpc
endif

ifndef TF
TF = 1
endif
ifeq "$(TF)" "1"
include makefile.tf
endif

ifndef USAC
override USAC = 1
endif
ifeq "$(USAC)" "1"
override DEFFLAGS += -DSAIT -DSONY_PVC -DUNIFIED_RANDOMSIGN
include makefile.usac
COMLIBS_mp4spatialdec = -lmp4spatialdec
COMLIBS_mp4spatialenc = -lmp4spatialenc
INCL += -I../../mp4spatialdec/src -I../../mp4spatialenc/src

# USAC Reference software, Corrigendum 1 >>>
ifeq "$(USAC_REFSOFT_COR1)" "0"
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_01
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_02
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_03
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_04
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_05
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_06
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_07
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_08
override DEFFLAGS += -DUSAC_REFSOFT_COR1_NOFIX_09

endif
# <<< USAC Reference software, Corrigendum 1

endif

ifeq "$(SSC)" "1"
include makefile.ssc
endif

ifeq "$(SLS)" "1"
include makefile.sls
SRCDIRS := $(SRCDIRS):src_sls
endif



OBJS_DEC_PICOLA = 
LIBS_DEC_PICOLA = 
ifndef PICOLA
PICOLA = 0
endif
ifeq "$(PICOLA)" "1"
OBJS_DEC_PICOLA = pan_picola.o
else
# OBJS_DEC_PICOLA = picola_dmy.o
endif
ifeq "$(MPEG12)" "1"
OBJS_DEC_MPEG12 = dec_mpeg12.o
COMLIBS_mpeg12dec = -lmpeg12dec
OPTIONS += "COMLIBS_libmpeg12dec.a_OPTIONS=\"MPEG_2_5=1\""
SRCDIRS := $(SRCDIRS):src_mpeg12
endif



OBJS_CORE = common_m4a.o flex_mux.o usac_config.o bitstream.o vector_ops.o spatial_bitstreamreader.o spatial_bitstreamreader_buffer.o

ifeq "$(DEBUGPLOT)" "1"
OBJS_CORE += plotmtv_interf.o writevector.o
else
OBJS_CORE += plotmtv_interf_dummy.o
endif

ifeq "$(VERSION)" "2"
    ifeq "$(STATISTICS)" "1"
        OBJS_CORE += statistics.o
    else
        OBJS_CORE += statistics_dummy.o
    endif
    OBJS_CORE += resilience.o
endif

OBJS_COM = $(OBJS_CORE) cmdline.o audio.o austream.o fir_filt.o adif.o
OBJS_ENC = usacEnc.o $(OBJS_COM) $(OBJS_STREAMFILE) \
	$(OBJS_ENC_TF) $(OBJS_ENC_SLS) $(OBJS_ENC_USAC)

OBJS_DEC = usacDec.o decifc.o $(OBJS_COM) $(OBJS_STREAMFILE)\
	$(OBJS_DEC_PAR) $(OBJS_DEC_LPC) $(OBJS_DEC_TF) $(OBJS_DEC_SLS) $(OBJS_DEC_SSC)\
	$(OBJS_DEC_PICOLA) $(OBJS_DEC_MPEG12)  $(OBJS_DEC_USAC) mod_buf.o


OBJS_STREAMFILE = streamfile.o streamfile_helper.o streamfile_diagnose.o streamfile_mp4.o streamfile_fl4.o streamfile_latm.o streamfile_raw.o ep_convert.o
OBJS_CONVERT = convert.o cmdline.o $(OBJS_CORE) $(OBJS_STREAMFILE)
OBJS_LIBSTREAMFILE = $(OBJS_STREAMFILE) bitstream.o common_m4a.o flex_mux.o cmdline.o resilience.o statistics_dummy.o

ifeq "$(ISOMP4)"  "1"
OBJS_DEC += mp4ifc.o
endif

DUMMY_SOURCES = plotmtv_interf_dummy.c concealment_dummy.c statistics_dummy.c statistics_aac_dummy.c rvlcScfConceal_dummy.c

LIBS_ENC = $(LIBS_ENC_PAR) $(LIBS_ENC_LPC) $(LIBS_ENC_TF) $(LIBS_ENC_SSC) $(LIBS_ENC_SLS) 
LIBS_DEC = $(LIBS_DEC_PAR) $(LIBS_DEC_LPC) $(LIBS_DEC_TF) $(LIBS_DEC_SSC) $(LIBS_DEC_SLS) $(LIBS_DEC_PICOLA)

ADDONDIR = v$(VERSION)
ifeq "$(CORRIGENDUM)" "1"
ADDONDIR := $(ADDONDIR)_c$(CORRIGENDUM)
endif
ifeq "$(EPTOOLLIB)" "1"
ADDONDIR := $(ADDONDIR)_eptool
endif
ifeq "$(FHG_EPTOOLLIB)" "1"
ADDONDIR := $(ADDONDIR)_fhgeptool
endif
ifeq "$(SBR)" "1"
ADDONDIR := $(ADDONDIR)_sbr
endif
ifeq "$(LOW_POWER_SBR)" "1"
ADDONDIR := $(ADDONDIR)_lp
endif
ifeq "$(PARAMETRICSTEREO)" "1"
ADDONDIR := $(ADDONDIR)_ps
endif
ifeq "$(MPEG12)" "1"
ADDONDIR := $(ADDONDIR)_mpeg12
endif

TARGETS = usacDec usacEnc

# ifeq "$(ENCODE)" "1"
SRCDIRS += ../src_usac_enc ../src_frame_enc ../src_par_enc ../src_lpc_enc ../src_tf_enc  \
	../src_frame ../src_par       ../src_lpc     ../src_tf      ../src_ssc    ../src_ssc/import ../src_mp4   \
	../src_usac
#include error $(SRCDIRS)
#LIBPATH += -L$(PLTDIR)/lib_enc
# endif

ifeq "$(EPTOOLLIB)" "1"
COMLIBS_eptool = -leptool
INCL+=-I../../epTool/src/
else
COMLIBS_eptool =
endif

# LIBISOMEDIA FILE PATH ################################################

ISOMEDIA_LIB = $(GENERALROOT)/tools/IsoLib/libisomediafile/$(ISOMEDIALIBDIR)/libisomediafile/libisomediafile.a


# AFSP FILE PATH
ifeq "$(TARGET_SYSTEM)" "Linux"
ifeq "$(TARGET_PLATFORM)" "x86_64"
 AFSP_LIB =  $(GENERALROOT)/tools/AFsp/lib/libtsplite_x86_64.a
else
 AFSP_LIB =  $(GENERALROOT)/tools/AFsp/lib/libtsplite.a
endif
endif

ifeq "$(TARGET_SYSTEM)" "Darwin"
 AFSP_LIB = $(GENERALROOT)/tools/AFsp/lib/libtsplite_Mac.a
endif


OPTIONS += $(MFLAGS) SRCDIRS="$(SRCDIRS)" ADDONDIR="$(ADDONDIR)" DEFFLAGS="$(DEFFLAGS)" INCL="$(INCL)"\
	 LIBPATH="$(LIBPATH)" SYSLIBS="$(SYSLIBS) $(ISOMEDIA_LIB) $(AFSP_LIB)" INCLDEP=1\
	 EXTERNLIBS="$(EXTERNLIBS)" EXTERNLIBS_incl="$(EXTERNLIBS_incl)" EXTERNLIBS_libpath="$(EXTERNLIBS_libpath)"


.PHONY: all usacDec usacEnc convert clean dummies

all: $(TARGETS)

usacEnc: $(ISOMEDIA_LIB)
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_ENC)" MODULELIBS="$(LIBS_ENC)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec) $(COMLIBS_mp4spatialenc) $(COMLIBS)" ENCODE=1 binary


usacDec:  $(ISOMEDIA_LIB) dummies
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_DEC)" MODULELIBS="$(LIBS_DEC)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mpeg12dec) $(COMLIBS_mp4spatialdec) $(COMLIBS)" binary

convert: 
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_CONVERT)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec) $(COMLIBS)" binary

libstreamfile.a:
	@$(MAKE) $(OPTIONS) TARGET=$@ OBJS="$(OBJS_LIBSTREAMFILE)" library

$(ISOMEDIA_LIB):
	@$(MAKE) -C $(GENERALROOT)/tools/IsoLib/libisomediafile/$(ISOMEDIALIBDIR)/libisomediafile CFLAGS+=$(ISOMEDIALIBFLAGS)

clean:
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=usacEnc OBJS="$(OBJS_ENC)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec)" ENCODE=1  $@
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=convert  OBJS="$(OBJS_CONVERT)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec)"   $@
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=usacDec  OBJS="$(OBJS_3D_AUDIO_CORE_DECODER)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec)"   $@

clean_all:
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=usacEnc OBJS="$(OBJS_ENC)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec)" ENCODE=1  clean
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=convert OBJS="$(OBJS_CONVERT)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec)"   clean
	@$(MAKE) ADDONDIR="$(ADDONDIR)" TARGET=usacDec OBJS="$(OBJS_3D_AUDIO_CORE_DECODER)" COMLIBS="$(COMLIBS_eptool) $(COMLIBS_mp4spatialdec)"   clean
	@$(MAKE) -C $(GENERALROOT)/tools/IsoLib/libisomediafile/$(ISOMEDIALIBDIR)/libisomediafile CFLAGS+=$(ISOMEDIALIBFLAGS) clean

endif	# ifeq "$(MAKELEVEL)" "0"
