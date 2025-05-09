#-*-makefile-*- do not remove, it's for emacs!
#----------------------------------------------------------------------
# MPEG-4 Audio VM
# makefile.tf
#
# Authors:
# BG    Bernhard Grill, Uni Erlangen <grl@titan.lte.e-technik.uni-erlangen.de>
# HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
# NI    Naoki Iwakami, NTT <iwakami@splab.hil.ntt.jp>
# TK    Takashi Koike, Sony Corporation <koike@av.crl.sony.co.jp>
# YT    Yasuhiro Toguri, Sony Corporation <toguri@av.crl.sony.co.jp>
# OK    Olaf Kaehler, Fraunhofer IIS <kaehleof@iis.fhg.de>
#
# Changes:
# 20-aug-96   BG      updated version
# 26-aug-96   HP      changed to enc_tf.c and dec_tf.c, removed PLOBJS
# 26-aug-96   HP      CVS
# 28-aug-96   NI      added objects for the NTT's VQ coder
# 25-oct-96   HP      adapted makefile options
# 13-nov-96   BG/HP   possible default: TF_QC = AAC
# 06-dec-96   NI      changed objects for the NTT's VQ encoder
# 09-feb-97   TK      integrated AAC compliant pre-/post- processing modules
# 14-mar-97   HP      merged FhG AAC decoder code
#                     temporary patch to keep encoder "alive" (see HP 970314)
# 18-apr-97   NI      integrated modules for scalable coder
# 01-jul-97   YT      add SSR encoder source codes. disable SSR lib.
# 23-oct-97   HP      added libBSACenc dummy
# 04-nov-97   HP      added ntt_nok_lt_predict_dec.o
# 04-nov-97   HP      merged ntt/mat 971031
# 05-nov-97   HP      merged sam971029
# 20-nov-97   HP      added tf_celp_dmy
# 12-feb-98   HP      override for CFLAGS+=
# 08-apr-98   HP      removed LDFB
# 02-may-02   OK      added PNS_NONDETERMINISTIC_RANDOM
#----------------------------------------------------------------------


# makefile.par options:
# TF_T2F=UER   UER T2F code
# TF_QC=UER    UER QC code
#      =AAC    AAC QC code

# defaults:
ifndef TF_T2F
TF_T2F = UER
endif
ifndef TF_QC
TF_QC = AAC
endif
ifndef PNS_NONDETERMINISTIC_RANDOM
PNS_NONDETERMINISTIC_RANDOM=0
endif

#OBJS for SON_AACPP
# HP 970314
#LIBS_SON_AACPP_ENC = -lpqf_son_AACpp -lgain_son_AACpp
LIBS_SON_AACPP_DEC =
# YT 0701 added 
OBJS_SON_AACPP_ENC = \
	son_gc_common.o\
	son_pqf_common.o\
	son_gc_detecter.o \
	son_gc_detectreset.o \
	son_gc_etc_enc.o\
	son_gc_modifier.o\
	son_gc_pack.o\
	son_pqf.o

OBJS_SON_AACPP_DEC = son_gc_unpack.o son_gc_common.o son_gc_compensate.o son_ipqf.o son_pqf_common.o son_gc_etc.o

# disable TF dummy
OBJS_ENC_TF =
OBJS_DEC_TF =

# HP 970314
#OBJS_MOT_QC  = bstream.o hq.o huffman.o ninject.o q.o sf.o

OBJS_AAC_QC  = aac_qc.o aac_tools.o tns3.o bitmux.o

# objects for the BSAC coder
# YB 971106
LIBS_BSAC =
OBJS_BSAC_ENC = 
OBJS_BSAC_DEC = 

ifeq "$(VERSION)" "2"
OBJS_BSAC_ENC = sam_encode_frame.o sam_encode_bsac.o\
		sam_faencode.o sam_bit_buffer.o
OBJS_BSAC_DEC = sam_decode_frame.o sam_decoder.o sam_decode_bsac.o\
		sam_fadecode.o sam_tns.o sam_bitbuffer.o
endif

# objects for the NTT_VQ coder
#LIBS_VQ      = -lVQenc
OBJS_VQ_ENC  = \
	ntt_BitPack.o \
	ntt_TfInit.o \
	ntt_check.o \
	ntt_cnst_chk.o \
	ntt_dec_bark_env.o \
	ntt_dec_gain.o \
	ntt_dec_lpc_spectrum_inv.o \
	ntt_dec_pit_seq.o \
	ntt_dec_pgain.o \
	ntt_denormalizer_spectrum.o \
	ntt_extend_pitch.o \
	ntt_fwex.o \
	ntt_get_code.o \
	ntt_get_cdbk.o \
	ntt_lsp_decw.o \
	ntt_lsptowt_int.o \
	ntt_movdd.o \
	ntt_mulawinv.o \
	ntt_post_process.o \
	ntt_redec.o \
	ntt_scale_coder.o \
	ntt_scale_init.o \
	ntt_scale_message.o \
	ntt_scale_tf_proc_spectrum_d.o \
	ntt_scale_vq_coder.o \
	ntt_scale_vq_decoder.o \
	ntt_set_isp.o \
	ntt_tf_perceptual_model.o \
	ntt_tf_proc_spectrum.o \
	ntt_tf_quantize_spectrum.o \
	ntt_tf_requantize_spectrum.o \
	ntt_tools.o \
	ntt_vec_lenp.o \
	ntt_vex_pn.o \
	ntt_vq_coder.o \
	ntt_win_sw.o \
	ntt_zerod.o \
	ntt_scale_dec_bark_env.o \
	ntt_dec_sq_gain.o \
	mat_scale_set_shift_para.o \
	ntt_vq_decoder.o \
	ntt_tf_proc_spectrum_d.o \
	ntt_dec_pitch.o \
	mat_scale_tf_requantize_spectrum.o


OBJS_VQ_DEC  = \
	ntt_BitUnPack.o \
	ntt_SclBitUnPack.o \
	ntt_TfInit.o \
	ntt_check.o \
	ntt_cnst_chk.o \
	ntt_dec_bark_env.o \
	ntt_dec_gain.o \
	ntt_dec_lpc_spectrum_inv.o \
	ntt_dec_pgain.o \
	ntt_dec_pit_seq.o \
	ntt_dec_pitch.o \
	ntt_denormalizer_spectrum.o \
	ntt_extend_pitch.o \
	ntt_fwex.o \
	ntt_get_cdbk.o \
	ntt_get_code.o \
	ntt_lsp_decw.o \
	ntt_lsptowt_int.o \
	ntt_movdd.o \
	ntt_mulawinv.o \
	ntt_post_process.o \
	ntt_redec.o \
	ntt_scale_init.o \
	ntt_scale_message.o \
	ntt_scale_tf_proc_spectrum_d.o \
	ntt_scale_vq_decoder.o \
	ntt_set_isp.o \
	ntt_tf_proc_spectrum_d.o \
	ntt_tf_requantize_spectrum.o \
	ntt_vec_lenp.o \
	ntt_vex_pn.o \
	ntt_vq_decoder.o \
	ntt_zerod.o \
	ntt_scale_dec_bark_env.o \
	ntt_dec_sq_gain.o \
	mat_scale_set_shift_para.o \
	mat_scale_tf_requantize_spectrum.o \
	tvqAUDecode.o \
	ntt_headerdec.o 



ifeq "$(TF_T2F)" "UER"
LIBS_T2F_ENC = 
LIBS_T2F_DEC =
endif

ifeq "$(TF_QC)" "UER"
endif

ifeq "$(TF_QC)" "AAC"
# override DEFFLAGS +=  -DSRS -DDOUBLE_WIN -DDOLBY_MDCT -DOLD_PRED_PARAMS 
# HP 970408 new predictor attenuation factors since Nov. 96
override DEFFLAGS +=  -DSRS -DDOUBLE_WIN -DDOLBY_MDCT -DPNS -DDRC
endif

# initialize random in pns.c by time()
ifeq "$(PNS_NONDETERMINISTIC_RANDOM)" "1"
override DEFFLAGS += -DPNS_NONDETERMINISTIC_RANDOM
endif

OBJS_PSYCH = psych.o

OBJS_AAC_ENC = util.o buffers.o bitfct.o transfo.o imdct.o \
              nok_bwp_enc.o nok_ltp_enc.o nok_pitch.o decdata.o\
              intrins.o


OBJS_AAC_DEC = config.o coupling.o decdata.o decoder_tf.o pns.o\
               huffdec1.o huffdec2.o huffdec3.o huffinit.o hufftables.o \
               imdct.o \
               intensity.o intrins.o monopred.o nok_lt_prediction.o \
               stereo.o tns.o tns2.o transfo.o \
               util.o buffers.o \
               drc.o


ifeq "$(SBR)" "1"
OBJS_AAC_DEC += ct_envcalc.o ct_envdec.o \
               ct_envextr.o ct_freqsca.o \
               ct_hfgen.o ct_polyphase.o \
               ct_sbrbitb.o ct_sbrcrc.o \
               ct_sbrdec.o ct_sbrdecoder.o HFgen_preFlat.o \
               sony_pvcdec.o sony_pvclib.o
override DEFFLAGS += -DMPEG_SURROUND
endif


ifeq "$(PARAMETRICSTEREO)" "1"
OBJS_AAC_DEC += ct_psdec.o
endif


ifndef STATISTICS
    STATISTICS = 0
endif
ifndef ERRORCONCEALMENT
    ERRORCONCEALMENT = 0
endif
ifndef RVLC_SCF_CONCEAL
    RVLC_SCF_CONCEAL = 0
endif

ifeq "$(VERSION)" "2"
    ifeq "$(STATISTICS)" "1"
        OBJS_AAC_DEC += statistics_aac.o
    else
        OBJS_AAC_DEC += statistics_aac_dummy.o
    endif
    OBJS_AAC_DEC += bitfct.o reorderspec.o rvlcScfResil.o escapeScfResil.o  huffScfResil.o 
    ifeq "$(ERRORCONCEALMENT)" "1"
        OBJS_AAC_DEC += concealment.o
    else
        OBJS_AAC_DEC += concealment_dummy.o
    endif
    ifeq "$(RVLC_SCF_CONCEAL)" "1"
        OBJS_AAC_DEC += rvlcScfConceal.o
    else
        OBJS_AAC_DEC += rvlcScfConceal_dummy.o
    endif
endif

# HP 971120
ifeq "$(LPC)" "0"
OBJS_AAC_DEC += tf_celp_dmy.o
endif

# the next 4 definitions are relevant to the master makefile
#OBJS_ENC_TF = $(OBJS_AAC_ENC) enc_tf.o  $(OBJS_T2F_ENC)  $(OBJS_AAC_QC) $(OBJS_PSYCH) $(OBJS_VQ_ENC) $(OBJS_SON_AACPP_ENC)
# HP 970314
OBJS_ENC_TF = $(OBJS_AAC_ENC) enc_tf.o ms.o  $(OBJS_T2F_ENC)  $(OBJS_AAC_QC) $(OBJS_PSYCH) $(OBJS_VQ_ENC) $(OBJS_SON_AACPP_ENC) $(OBJS_MOT_QC) $(OBJS_BSAC_ENC) \
	 scal_enc_frame.o ct_polyphase_enc.o ct_sbr.o ct_freqsca.o sony_pvcenc.o sony_pvclib.o
OBJS_DEC_TF = $(OBJS_AAC_DEC) dec_tf.o  $(OBJS_T2F_DEC) $(OBJS_VQ_DEC) $(OBJS_SON_AACPP_DEC) $(OBJS_BSAC_DEC) scal_dec_frame.o
LIBS_ENC_TF = $(LIBS_T2F_ENC) $(LIBS_VQ) $(LIBS_BSAC)   $(LIBS_SON_AACPP_ENC)
LIBS_DEC_TF = $(LIBS_T2F_DEC) $(LIBS_SON_AACPP_DEC) 


# end of makefile.tf
