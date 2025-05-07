/**********************************************************************
MPEG-4 Audio VM
ep encode and decode routines

This software module was originally developed by

Olaf Kaehler (Fraunhofer IIS-A)

and edited by


in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 AAC/MPEG-4 Audio tools
as specified by the MPEG-2 AAC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 AAC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 AAC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
AAC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 AAC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 2004.
Source file: ep_convert.h

 

**********************************************************************/

#ifndef _EP_CONVERT_H_
#define _EP_CONVERT_H_

#include "streamfile.h" /* HANDLE_STREAM_AU */
#include "obj_descr.h"  /* EP_SPECIFIC_CONFIG */
#include "allHandles.h" /* HANDLE_EPCONVERTER */

HANDLE_EPCONVERTER EPconvert_create(int inputTracks,
                                    int inputEpConfig,
                                    EP_SPECIFIC_CONFIG *inputEpInfo,
                                    const char *writePredFile,
                                    int removeLastNClasses,
                                    int outputEpConfig,
                                    EP_SPECIFIC_CONFIG *outputEpInfo,
                                    const char *readPredFile);

/* return the number of valid tracks after all conversions.
   please note there might be cases where actually no valid output is generated
   from any number of input access units. this is namely the case if the
   concatenation tool of the EPTool is activated for encoding, merging multiple
   codec-frames into one epTool-frame.
*/
int EPconvert_processAUs(const HANDLE_EPCONVERTER cfg,
                         const HANDLE_STREAM_AU *input, int validTracksIn,
                         HANDLE_STREAM_AU *output, int maxTracksOut);

/* call this function once to ensure the concatenation of all processed
   access units is byte aligned in each processed frame */
void EPconvert_byteAlignOutput(const HANDLE_EPCONVERTER cfg);

/* return the number of output tracks expected each frame */ 
int EPconvert_expectedOutputClasses(const HANDLE_EPCONVERTER cfg);

/* set splitting parameters to split ep0 access units.
   num determines number of classes to split into
   following parameter gives the lengths in bits
   if this is used to create ep1 from ep0, note the expectedOutputClasses
   will be affected */
void EPconvert_setSplitting(HANDLE_EPCONVERTER cfg, int num, int lengths[]);

/* the concatenation tool of the epTool might en-/decode multiple codec-frames
   to or from one of its epTool-frames. in such a case the extra frames are
   buffered internally in the epTool and we can ask for more codec-frames
   without supplying any new inputs or have to supply extra frames until we
   will get an output. these methods will tell, how many frames are still
   buffered, or whether we need more frames respectively. */
int EPconvert_numFramesBuffered(HANDLE_EPCONVERTER cfg);
int EPconvert_needMoreFrames(HANDLE_EPCONVERTER cfg);

#endif
