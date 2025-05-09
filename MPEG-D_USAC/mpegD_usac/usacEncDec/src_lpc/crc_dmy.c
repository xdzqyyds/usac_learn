/*
This software module was originally developed by
Souich Ohtsuka and Masahiro Serizawa (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 AAC/MPEG-4 Audio tools as specified by the MPEG-2 AAC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 AAC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 AAC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 AAC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 AAC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*
 * crc dummy
 */
#include	<stdio.h>
#include	<stdlib.h>

#define	CRCFILE_INPUT

long
crc_dmy( void )
{
	static char	*crc_file;
	static int	init_flag = 1;
	static FILE	*crc_fd = NULL;
	long		crc;

	if( init_flag ){
		init_flag = 0;
#ifdef	CRCFILE_INPUT
		crc_file = getenv( "MP4_CRCFILE" );
		if( crc_file && crc_file[0] != '\0' ){
			if( (crc_fd = fopen( crc_file, "r" )) == NULL )
				fprintf( stderr, "%s: No such file\n",
					 crc_file );
		}
#endif
	}

	crc = 0;
	if( crc_fd ){
		char crc_flag[2];

		if( fscanf( crc_fd, "%1s", crc_flag ) != 1 ){
			fprintf( stderr, "%s: End of file\n", crc_file );
			fclose( crc_fd );
			crc_fd = NULL;
		} else if( crc_flag[0] == '1' )
			crc = 1;
	}
	return crc;
}
