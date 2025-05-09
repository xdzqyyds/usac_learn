
/*

This software module was originally developed by

    Kazuyuki Iijima (Sony Corporation)

    and edited by

    Akira Inoue (Sony Corporation) and
    Yuji Maeda (Sony Corporation)

    in the course of development of the MPEG-4 Audio standard (ISO/IEC 14496-3).
    This software module is an implementation of a part of one or more
    MPEG-4 Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
    standard (ISO/IEC 14496-3).
    ISO/IEC gives users of the MPEG-4 Audio standards (ISO/IEC 14496-3)
    free license to this software module or modifications thereof for use
    in hardware or software products claiming conformance to the MPEG-4
    Audio standards (ISO/IEC 14496-3).
    Those intending to use this software module in hardware or software
    products are advised that this use may infringe existing patents.
    The original developer of this software module and his/her company,
    the subsequent editors and their companies, and ISO/IEC have no
    liability for use of this software module or modifications thereof in
    an implementation.
    Copyright is not released for non MPEG-4 Audio (ISO/IEC 14496-3)
    conforming products. The original developer retains full right to use
    the code for his/her own purpose, assign or donate the code to a third
    party and to inhibit third party from using the code for non MPEG-4
    Audio (ISO/IEC 14496-3) conforming products.
    This copyright notice must be included in all copies or derivative works.

    Copyright (c)1996.

*/

#ifndef _hvxcDec_h
#define _hvxcDec_h

#include "dec_hvxc.h"	/* HP 990409 */

#include "allHandles.h"

/* definition of HVXC decoder status struct(AI 990129) */
#include "hvxc_struct.h"	/* AI 990616 */

#ifdef __cplusplus
extern "C" {
#endif

HvxcDecStatus *hvxc_decode_init(
int varMode,		/* in: HVXC variable rate mode */
int rateMode,		/* in: HVXC bit rate mode */
int extentionFlag,	/* in: the presense of verion 2 data */
/* version 2 data (YM 990616) */
int numClass,		/* in: HVXC decode data packing mode */
/* version 2 data (YM 990728) */
int vrScalFlag,		/* in: HVXC VR scalable mode */
int delayMode,		/* in: decoder delay mode */
int testMode);		/* in: test_mode for decoder conformance testing */

void hvxc_decode(
HvxcDecStatus *HDS,	/* in: pointer to decoder status */
unsigned char *encBit,	/* in: bit stream */
float *sampleBuf,	/* out: frameNumSample audio samples */
int *frameBufNumSample,	/* out: num samples in sampleBuf[] */
float speedFact,	/* in: speed change factor */
float pitchFact,	/* in: pitch change factor */
int decMode		/* in: decode rate mode */
/* version 2 data (YM 990616) */
, int epFlag,		/* in: HVXC decode data packing mode */
HANDLE_ESC_INSTANCE_DATA hEscInstanceData	/* in: HVXC decode data packing mode */
);

void hvxc_decode_free(
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_bpf(
float		*bufsf,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_DecLspFBF(
IdLsp		*idLsp,
float		qLsp[10]);

void IPC_DecLspEnh(
IdLsp		*idLsp,
float		qLsp[10]);

void IPC_set_const_lpcVM_dec(void);

void harm_sew_dec(
float 		(*am)[3],
float		*pch,
int		normMode,
int		vuv,
int		*idxS,
int		bitnum,
int		idxG,
float		pchmod,
int		*idAm4k,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void td_synt(
float		*qRes,
int		*VUVs,
float		(*qLsp)[10],
float		*synoutu,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void harm_srew_synt(
float		(*am)[3],
float		*PCHs,
int		*VUVs,
float		(*qLsp)[10],
int		lsUn,
float		*synoutv,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_make_w_celp(void); 

void IPC_make_c_dis(void);

void td_decoder(
int		idVUV,
int		*idSL0,
int		*idGL0,
int		*idSL1,
int		*idGL1,
float		*qRes,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void td_decoderVR(
int             idVUV,
int             *idSL0,
int             *idGL0,
int             *idSL1,
int             *idGL1,
float           *qRes,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_make_f_coef_dec(void);

void IPC_SynthSC(
int		idVUV,
float		*qLsp,
float		mfdpch,
float		(*am)[3],
float		*qRes,
short int	*frmBuf,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

int IPC_DecParams1st(
unsigned char	*encBit,
float	pchmod,
int	*idVUV2,
float	(*qLspQueue)[LPCORDER],
float	*pch2,
float	(*am2)[SAMPLE/2][3],
IdCelp	*idCelp2,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

int IPC_DecParams1stVR(
unsigned char   *encBit,
float   pchmod,
int     *idVUV2,
int     *bgnFlag2,
float   (*qLspQueue)[LPCORDER],
float   *pch2,
float   (*am2)[SAMPLE/2][3],
IdCelp  *idCelp2,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_DecParams2nd(
int	fr0,
int	*idVUV2,
float	(*qLspQueue)[LPCORDER],
IdCelp	*idCelp2,
float	(*qLsp2)[LPCORDER],
float	(*qRes2)[FRM],
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_DecParams2ndVR(
int     fr0,
int     *idVUV2,
float   (*qLspQueue)[LPCORDER],
IdCelp  *idCelp2,
float   (*qLsp2)[LPCORDER],
float   (*qRes2)[FRM],
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_uvExt(
int	*vuv,
float	*suv,
float	*qRes,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_vExt_fft(
float	*pch,
float	(*am)[3],
int	*vuv,
float	*sv,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_UvAdd(
float	*pch,
float	(*am)[3],
int	*vuv,
float	*add_uv,
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_InterpolateParams(
float	rate,
int	*idVUV2,
float	(*lsp2)[LPCORDER],
float	*pch2,
float	(*am2)[128][3],
float	(*uvExt2)[FRM],
int	*modVUV,
float	*modLsp,
float	*modPch,
float	(*modAm)[3],
float	*modUvExt,
int 	testMode);	/* in: test_mode for decoder conformance testing */

void IPC_ip_lsp_LD(
float (*lsp)[11],
float (*lspip)[11]);

void pan_lspdecEP(float out_p[], float out[], 
    float p_factor, float min_gap, long lpc_order, 
    long idx[], float tbl[], float d_tbl[], float rd_tbl[], 
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[],
    long flagStab, long flagPred, long sameVUV, 
    long crcResultEP[], long crcErrCnt);

void pan_lspdec(float out_p[], float out[], 
    float p_factor, float min_gap, long lpc_order, 
    long idx[], float tbl[], float d_tbl[], float rd_tbl[], 
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[],
    long flagStab, long flagPred);

void IPC_posfil_v(
float	*syn,
float	(*alphaip)[11],
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_posfil_u(
float	*syn,
float	(*alphaip)[11],
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

void IPC_posfil_u_LD(
float	*syn,
float	(*alphaip)[11],
HvxcDecStatus *HDS);	/* in: pointer to decoder status(AI 990129) */

#ifdef __cplusplus
}
#endif

#endif		/* _hvxcDec_h */

