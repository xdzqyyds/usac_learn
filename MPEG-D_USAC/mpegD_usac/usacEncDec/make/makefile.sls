#-*-makefile-*- do not remove, it's for emacs!
#----------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile.sls
#
#  
#
# Authors:
# MH    michael.haertl@iis.fraunhofer.de
#
# Changes:
# 23-oct-06   MH    first version
#----------------------------------------------------------------------


# makefile.sls options:

# defaults:

# disable SLS dummy
OBJS_ENC_SLS =
OBJS_DEC_SLS =

ifeq "$(SLS)" "1"
LIBS_ENC_SLS +=
LIBS_DEC_SLS +=
OBJS_ENC_SLS +=	lit_ll_common.o \
		lit_ll_en.o \
                cbac.o \
		acod.o \
                mc_enc.o \
                ll_bitmux.o \
                lit_ms.o \
                int_mdct.o \
                int_win_dctiv.o \
                int_compact_tables.o                

#                int_tns_encoder.o \

OBJS_DEC_SLS +=	lit_ll_common.o \
		lit_ll_dec.o \
		cbac.o \
		acod.o \
		inv_int_mdct.o \
		int_win_dctiv.o \
		lit_ms.o \
		int_tns.o \
		int_compact_tables.o \
		decoder_sls.o

endif

# end of makefile.sls

