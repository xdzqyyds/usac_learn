/*****************************************************************************
 *                                                                           *
"This software module was originally developed by NTT Mobile
Communications Network, Inc. in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and
3. This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4
Audio standard. ISO/IEC gives users of the MPEG-2 AAC/MPEG-4 Audio
standards free license to this software module or modifications
thereof for use in hardware or software products claiming conformance
to the MPEG-2 AAC/MPEG-4 Audio standards. Those intending to use this
software module in hardware or software products are advised that this
use may infringe existing patents. The original developer of this
software module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-2 AAC/MPEG-4 Audio conforming products.The
original developer retains full right to use the code for his/her own
purpose, assign or donate the code to a third party and to inhibit
third party from using the code for non MPEG-2 AAC/MPEG-4 Audio
conforming products. This copyright notice must be included in all
copies or derivative works." Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eptool.h"
#include "concat.h"

/* a maximum of n classes are concatenated */
#define MAX_CONCATENATED_CLASSES 8

/* --- concatenation buffer --- */

struct _t_concat_buffer {
  int classes;
  int frames;
  int current_frame;
  int current_pred;
  /* store the data */
  unsigned char ***buf;
  /* remember the values from the epConfig */
  int **concat_flag;
  unsigned long **len;
};

HANDLE_CONCAT_BUFFER concatenation_create(EPConfig *epcfg)
{
  int i, j, classes = 0;
  HANDLE_CONCAT_BUFFER ret;

  if (epcfg==NULL) return NULL;
  if (epcfg->NConcat==0) return NULL; /* reserved */

  for (i=0; i<epcfg->NPred; i++) {
    if (epcfg->fattr[i].ClassCount>classes) classes=epcfg->fattr[i].ClassCount;
    if (epcfg->NConcat!=1) {
      /* ...then no escape mechanism shall be used for any class parameter */
      for (j=0; j<epcfg->fattr[i].ClassCount; j++) {
        ClassAttribContent *esc = &(epcfg->fattr[i].cattresc[j]);
        if ((esc->ClassBitCount)||(esc->ClassCodeRate)||(esc->ClassCRCCount))
          return NULL;
      }
    }
    /* ...else remember length escapes, which is done elsewhere */
  }

  /* everything okay to prepare the concatenation buffer */
  ret = calloc(1, sizeof(struct _t_concat_buffer));
  if (ret==NULL) return ret;
  ret->classes = classes;
  ret->frames = epcfg->NConcat;
  ret->current_frame = 0;
  ret->current_pred = -1;

  ret->buf = malloc(ret->frames * sizeof(unsigned char**));

  for (i=0; i<ret->frames; i++) {
    ret->buf[i] = malloc(ret->classes * sizeof(unsigned char*));
    for (j=0; j<ret->classes; j++) ret->buf[i][j] = NULL;
  }

  ret->concat_flag = malloc(epcfg->NPred * sizeof(int*));
  ret->len = malloc(epcfg->NPred * sizeof(unsigned long*));
  for (i=0; i<epcfg->NPred; i++) {
    ret->concat_flag[i] = malloc(ret->classes * sizeof(int));
    ret->len[i] = malloc(ret->classes * sizeof(unsigned long));
    for (j=0; j<epcfg->fattr[i].ClassCount; j++) {
      ret->concat_flag[i][j] = epcfg->fattr[i].cattr[j].Concatenate;
      if (epcfg->fattr[i].cattresc[j].ClassBitCount) {
        /* ...and remember length escapes! */
        ret->len[i][j] = -1;
        /*ret->buf[i][j] = NULL;*/
      } else {
        ret->len[i][j] = epcfg->fattr[i].cattr[j].ClassBitCount;
        /*ret->buf[i][j] = malloc((ret->len[i][j]+7)>>3);*/
      }
    }
    for (; j<ret->classes; j++) {
      ret->concat_flag[i][j] = -1;
      ret->len[i][j] = -1;
    }
  }

  return ret;
}

void concatenation_free(HANDLE_CONCAT_BUFFER concat_buffer)
{
  int c,f;
  if (concat_buffer==NULL) return;

  for (f=0; f<concat_buffer->frames; f++) {
    for (c=0; c<concat_buffer->classes; c++) {
      if (concat_buffer->buf[f][c]) free(concat_buffer->buf[f][c]);
      }	
    free(concat_buffer->buf[f]);
    }
  free(concat_buffer->buf);

  /* FIXME borken destructor
  for (c=0; c<NPred; c++) {
    free(concat_buffer->concat_flag[c]);
    free(concat_buffer->len[c]);
  }
  */
  free(concat_buffer->concat_flag);
  free(concat_buffer->len);
  
  free(concat_buffer);
}



/* --- helper functions --- */

/* mangle bits... lovely ascii-trash once again */
static void concatenate_classes(unsigned char **in_classes, unsigned long len, int num, unsigned char *out_buffer)
{
  unsigned int bitsfilled = len&7;
  unsigned long bytepos = len>>3;
  unsigned long bytelen = (len+7)>>3;
  int i;
  memcpy(out_buffer, in_classes[0], bytelen);
  for (i=1; i<num; i++) {
    unsigned int j;
    for (j=0; j<bytelen; j++) {
      out_buffer[bytepos] &= ((unsigned char)((unsigned short)0xff00>>bitsfilled));
      out_buffer[bytepos]|=in_classes[i][j] >> bitsfilled;
      if ((bitsfilled)&&(!((j==bytelen-1)&&(bitsfilled<=(8-(len&7)))))) {
        out_buffer[bytepos+1]=in_classes[i][j] << (8-bitsfilled);
      }
      bytepos++;
    }
    bitsfilled += (len&7);
    if (bitsfilled>=8) { bitsfilled-=8; bytepos--; }
  }
  }

/* yet more mangling with an ascii-junkyard */
static void unconcatenate_classes(const unsigned char *in_buffer, unsigned long len, int num, unsigned char **out_classes)
{
  int i;
  int bitsread = 0;
  unsigned int j, bytepos = 0;
  unsigned long bytelen = (len+7)>>3;

  for (i=0; i<num; i++) {
    for (j=0; j<bytelen; j++) {
      out_classes[i][j]=in_buffer[bytepos]<<bitsread;
      if ((bitsread)&&(!((j==bytelen-1)&&((len&7)<=(unsigned)(8-bitsread))))) {
        out_classes[i][j]|=in_buffer[bytepos+1]>>(8-bitsread);
      }
      bytepos++;
    }
    bitsread-=(8-(len&7));
    if (bitsread<8) { bitsread+=8; bytepos--; }
  }
}


/* --- encoding --- */

int concatenation_add(
	int choiceOfPred,
	HANDLE_CONCAT_BUFFER concat_buffer,
	const HANDLE_CLASS_FRAME inFrame)
{
  int i, f, p;
  if ((concat_buffer==NULL)||(inFrame==NULL)) return -1;

  /* The same pre-defined set shall be used for all concatenated frames */
  p = concat_buffer->current_pred;
  if ((p != choiceOfPred)&&(p != -1)) return -1;
  p = choiceOfPred;

  /* buffer is full? */
  f = concat_buffer->current_frame;
  if (f >= concat_buffer->frames) return -2;

  for (i=0; i<concat_buffer->classes; i++) {
    if (i<inFrame->nrValidBuffer) {
      long len = concat_buffer->len[p][i];
      if (len<0) {
        /* we can savly store this here, because there is only one frame when
           escaping is used */
        concat_buffer->len[p][i] = -inFrame->classLength[i]-1;
        len = inFrame->classLength[i];
      }
      concat_buffer->buf[f][i] = realloc(concat_buffer->buf[f][i], (len+7)>>3);
      memcpy(concat_buffer->buf[f][i],
             inFrame->classBuffer[i],
             (len+7)>>3);
    }
  }

  concat_buffer->current_pred = choiceOfPred;
  concat_buffer->current_frame++;
  return concat_buffer->frames - concat_buffer->current_frame;
}

int concatenation_classes_after_concat(
	const HANDLE_CONCAT_BUFFER concat_buffer)
{
  int pred, i, num=0;
  if (concat_buffer == NULL) return -1;
  if (concat_buffer->current_frame != concat_buffer->frames) return -2;
  pred = concat_buffer->current_pred;

  for (i=0; i<concat_buffer->classes; i++) {
    if (concat_buffer->concat_flag[pred][i]==-1) break;
    num += concat_buffer->concat_flag[pred][i] ? 1:concat_buffer->frames;
  }

  return num;
}


int concatenation_concat_buffer(
	HANDLE_CONCAT_BUFFER concat_buffer,
	unsigned char **out_classes,
	long *out_lengths,
	int *out_class_id)
{
  int pred, i, j, num=0;
  if ((concat_buffer==NULL)||(out_classes==NULL)) return -1;
  if (concat_buffer->current_frame != concat_buffer->frames) return -2;
  pred = concat_buffer->current_pred;

  for (i=0; i<concat_buffer->classes; i++) {
    int next;
    if (concat_buffer->concat_flag[pred][i]==-1) break;

    next = concat_buffer->concat_flag[pred][i] ? 1:concat_buffer->frames;
    if ((next == 1)&&(concat_buffer->frames>1)) {
      /* do a concatenation */
      unsigned char *in_classes[MAX_CONCATENATED_CLASSES];
      out_classes[num] = realloc(out_classes[num],
          ((concat_buffer->len[pred][i] * concat_buffer->frames)+7)>>3);
      for (j=0; j<concat_buffer->frames; j++) in_classes[j] = concat_buffer->buf[j][i];
      /*fprintf(stderr,"do a concat for class %i new len %i\n",i, concat_buffer->len[pred][i]* concat_buffer->frames);*/
      concatenate_classes(in_classes, concat_buffer->len[pred][i], concat_buffer->frames, out_classes[num]);
      out_lengths[num] = concat_buffer->len[pred][i] * concat_buffer->frames;
      out_class_id[num++] = i;
    } else {
      /* no concatenation, separate classes */
      /*fprintf(stderr,"copying class %i %i times, len %i\n",i,concat_buffer->frames, concat_buffer->len[pred][i]);*/
      for (j=0; j<concat_buffer->frames; j++) {
        long len = concat_buffer->len[pred][i];
        if (len<0) len = -len-1; /* escaped length stored as 2-complement */
        out_classes[num] = realloc(out_classes[num], (len+7)>>3);
        memcpy(out_classes[num], concat_buffer->buf[j][i], (len+7)>>3);
        out_lengths[num] = len;
        out_class_id[num++] = i;
      }
    }
  }

  concat_buffer->current_pred = -1;
  concat_buffer->current_frame = 0;

  return num;
}

/* --- decoding --- */

/* tell the epTool, how many classes to be found in his input */
int concatenation_classes_in_frame(
	const HANDLE_CONCAT_BUFFER concat_buffer,
	int pred_set)
{
  return concatenation_class_order_in_frame(concat_buffer, pred_set, NULL);
}

/* tell the epTool, which classes are where to be found in his input */
int concatenation_class_order_in_frame(
	const HANDLE_CONCAT_BUFFER concat_buffer,
	int pred_set,
	int *out_class_ids)
{
  int i, j=0, num=0;
  if (concat_buffer==NULL) return -1;

  for (i=0; i<concat_buffer->classes; i++) {
    if (concat_buffer->concat_flag[pred_set][i]==-1) break;
    num += concat_buffer->concat_flag[pred_set][i] ? 1:concat_buffer->frames;
    if (out_class_ids) {
      for (; j<num; j++) out_class_ids[j] = i;
    }
  }

  return num;
}

/*
  demultiplex a concatenated frame, fill the buffer
    return number of frames in concatenation
    if buffer was not yet empty, return -2
*/
int concatenation_demux(
	HANDLE_CONCAT_BUFFER concat_buffer,
	int pred_set,
	const unsigned char **class_data,
	const long *lengths)
{
  int i, j, num=0, ret=0;
  if ((concat_buffer==NULL)||(class_data==NULL)) return -1;
  if (concat_buffer->current_frame!=concat_buffer->frames) ret = -2;

  for (i=0; i<concat_buffer->classes; i++) {
    int next;
    if (concat_buffer->concat_flag[pred_set][i]==-1) break;
    next = concat_buffer->concat_flag[pred_set][i] ? 1:concat_buffer->frames;
    if ((next==1)&&(concat_buffer->frames>1)) {
      /* demux a concatenated frame */
      unsigned char *out_classes[MAX_CONCATENATED_CLASSES];
      int tmp_len = concat_buffer->len[pred_set][i];
      for (j=0; j<concat_buffer->frames; j++) {
        concat_buffer->buf[j][i] = realloc(concat_buffer->buf[j][i], (tmp_len+7)/8);
        out_classes[j] = concat_buffer->buf[j][i];
      }
      unconcatenate_classes(class_data[num], tmp_len, concat_buffer->frames, out_classes);
      num++;
    } else {
      /* no concatenation, just copy */
      long len = concat_buffer->len[pred_set][i];
      if (len<0) { /* escaped lengths really suck in a way */
        len = lengths[num];
        concat_buffer->len[pred_set][i] = -len-1;
      }
      for (j=0; j<concat_buffer->frames; j++) {
        concat_buffer->buf[j][i] = realloc(concat_buffer->buf[j][i], (len+7)>>3);
        memcpy(concat_buffer->buf[j][i], class_data[num++], (len+7)>>3);
      }
    }
  }

  concat_buffer->current_frame=0;
  concat_buffer->current_pred = pred_set;

  return ret?ret:num;
}

/*
  retrieve the next frame from the buffer
    if successful, return number of frames still in buffer
                   i.e. return 0 if buffer is empty
    if buffer was already empty, return -2
*/
int concatenation_retrieve(
	HANDLE_CONCAT_BUFFER concat_buffer,
	HANDLE_CLASS_FRAME outFrame)
{
  int i, f, pred, ret=0;
  if ((concat_buffer==NULL)||(outFrame==NULL)) return -1;
  if (concat_buffer->current_frame>=concat_buffer->frames) return -2;

  /* make class frame out of buf[concat_buffer->current_frame]
     the class frame was allocated... somewhere... i hope...
     ...did anyone tell you this epTool is desparately obscure? */
  f = concat_buffer->current_frame;
  pred = concat_buffer->current_pred;
  for (i=0; i<concat_buffer->classes; i++) {
    int len;
    if (concat_buffer->concat_flag[pred][i]==-1) break;
    if (i>=outFrame->maxClasses) { ret=-3; break; }
    len = concat_buffer->len[pred][i];
    if (len<0) len = -len-1;
    memcpy(outFrame->classBuffer[i], concat_buffer->buf[f][i], (len+7)>>3);
    outFrame->classLength[i] = len;
  }

  outFrame->nrValidBuffer = concat_buffer->classes;

  concat_buffer->current_frame++;
  return concat_buffer->frames - concat_buffer->current_frame;
}

