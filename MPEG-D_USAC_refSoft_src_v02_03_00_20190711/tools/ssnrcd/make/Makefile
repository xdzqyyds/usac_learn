#######################################################################
#
# Makefile: ssnrcd audio conformance verification tool
#
#######################################################################

# include the general makefiles using this directive
GENERALROOT=../../../
MAKEROOT=../
-include $(GENERALROOT)/general/makefile.all

# the following directive is mandatory in order to get all local stuff into this level
ifeq "$(MAKELEVEL)" "0"

# DIRECTORIES ##########################################################

SRCDIRS = ../src
INCL = -I../src
INCL+= -I$(GENERALROOT)/tools/AFsp/include

# SOURCES ##############################################################

SRCS = audiofile_io.c cepstrum_analysis.c mpeg4conf.c pns.c ssnrcd.c transfo.c

# SYSLIBS ##############################################################

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

# SUMMARIZE VARIABLES ##################################################

OPTIONS += SRCDIRS="$(SRCDIRS)" INCL="$(INCL)" LIBPATH="$(LIBPATH)" SYSLIBS="$(AFSP_LIB)" INCLDEP=1 DEFFLAGS="$(DEFFLAGS)"

# MAKEFILE RULES AND DEPENDENCIES ######################################
# the first rule `all' has to depend from makefile.all
all: $(GENERALROOT)/general/makefile.all $(ISOMEDIA_LIB) ssnrcd mpegaudioconf

ssnrcd:
	@$(MAKE) TARGET=$@ $(OPTIONS) CFLAGS="$(CFLAGS)" SRCS="$(SRCS)" binary

mpegaudioconf:
	@$(MAKE) TARGET=$@ $(OPTIONS) CFLAGS="$(CFLAGS)" SRCS="$(SRCS)" binary

clean:
	@$(MAKE) TARGET="ssnrcd" SRCS="$(SRCS)" clean
	@$(MAKE) TARGET="mpegaudioconf"  SRCS="$(SRCS)" clean

clean_all:
	@$(MAKE) TARGET="ssnrcd" SRCS="$(SRCS)" clean
	@$(MAKE) TARGET="mpegaudioconf"  SRCS="$(SRCS)" clean

endif
