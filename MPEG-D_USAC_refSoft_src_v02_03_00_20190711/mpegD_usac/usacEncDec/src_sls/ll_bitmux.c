
#include <stdio.h>

#include "all.h"  /* MIN() */
#include "allHandles.h"
#include "block.h"
#include "interface.h"

#include "nok_ltp_common.h"      /* structs */

#include "bitstream.h"
#include "common_m4a.h"
#include "nok_ltp_enc.h"
#include "tns3.h"

#include "aac_qc.h"
#include "bitmux.h"

#include "resilience.h"

#ifdef I2R_LOSSLESS
#include "ll_bitmux.h"
#endif

#undef DEBUG_TNS_BITS
#undef DEBUG_FSS_BITS
#undef DEBUG_ESC

struct tagAACbitMux {
  HANDLE_BSBITSTREAM *streams;
  HANDLE_RESILIENCE hResilience;
  HANDLE_ESC_INSTANCE_DATA hEscInstanceData;
  int numStreams;
  int usedAssignmentScheme;
  int currentChannel;
};

#ifdef I2R_LOSSLESS

void write_ll_sce(
  HANDLE_AACBITMUX bitmux,
  LIT_LL_Info *ll_Info 
  )
{
	unsigned char *stream = ll_Info->stream;
	short stream_len = (ll_Info->streamLen-1)/8+1;
	int i;
	
        /* ByteAlign(bitmux->streams[0]); */   /* No byte_align available yet */
	BsPutBit(bitmux->streams[0],(stream_len&0xff),8);
	BsPutBit(bitmux->streams[0],stream_len>>8,8);

	for (i=0;i<stream_len;i++)
          BsPutBit(bitmux->streams[0],(unsigned char)stream[i],8);	

	return;
}
#endif

