/************************* MPEG-2 AAC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 AAC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#ifndef	_stdinc_h_
#define	_stdinc_h_

#ifndef _POSIX_SOURCE
#define                 _POSIX_SOURCE   /* stops repeat typdef of ulong */
#endif

#include		<stdio.h>
#include		<stdlib.h>
#include		<sys/types.h>
#include 		<math.h>
#include		<string.h>

#define	PRINT		fprintf
#define	SO		stdout
#define	SE		stderr
#define	ARGBEGIN	for(argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				char _argc;\
				_args = &argv[0][1];\
				if((_args[0]=='-') && (_args[1]==0)){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while((_argc = *_args++))\
				switch(_argc)
#define	ARGEND		}
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc

#define	USED(v)		if(v);else

#endif	/* _stdinc_h */
