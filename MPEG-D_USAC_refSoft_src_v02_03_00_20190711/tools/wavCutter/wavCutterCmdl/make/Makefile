#######################################################################
#
# Makefile: separator example
#
#######################################################################

# include the general makefiles using this directive
GENERALROOT=../../../../
MAKEROOT=../
-include $(GENERALROOT)/general/makefile.all

# the following directive is mandatory in order to get all local stuff into this level
ifeq "$(MAKELEVEL)" "0"


# DIRECTORIES ##########################################################

SRCDIRS +=  ../src

INCL    +=  -I../../../wavIO/include


# SOURCES ##############################################################

SRCS = wavCutterCmdl.c


# COMLIBS ##############################################################

COMLIBS += -lwavIO


# SUMMARIZE VARIABLES ##################################################

OPTIONS  += SRCDIRS="$(SRCDIRS)" INCL="$(INCL)" LIBPATH="$(LIBPATH)" INCLDEP=1 DEFFLAGS="$(DEFFLAGS)" PLOT="$(PLOT)" GPROF="$(GPROF)"


# MAKEFILE RULES AND DEPENDENCIES ######################################
# the first rule `all' has to depend from makefile.all
all: $(GENERALROOT)/general/makefile.all $(ISOMEDIA_LIB) wavCutterCmdl 


wavCutterCmdl:  
	@$(MAKE) TARGET=$@ $(OPTIONS) COMLIBS="$(COMLIBS)" SYSLIBS="$(ISOMEDIA_LIB)" CFLAGS="$(CFLAGS)" SRCS="$(SRCS)" binary

clean:
	@$(MAKE) TARGET="wavCutterCmdl" SRCS="$(SRCS)" clean

clean_all:
	@$(MAKE) TARGET="wavCutterCmdl" OBJS="$(OBJS)" COMLIBS="$(COMLIBS)" clean


endif

