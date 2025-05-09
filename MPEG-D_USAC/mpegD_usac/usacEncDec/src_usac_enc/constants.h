/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.

Initial author:
Vladimir Malenovsky (VoiceAge Corp.)

and edited by:

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.

*******************************************************************************/

/*==================================================================================*/
/*  Global constants                                                                */
/*==================================================================================*/
/*  Last Revision: January 24, 2008                                                 */
/*==================================================================================*/

#define MULT_FACTOR     0.9f              /* multiplying factor for min distance */
#define FACTOR_ST1      0.5f              /* multiply initial min_distance by this factor (stage 1) */
#define FACTOR_ST2      2.5f              /* multiply initial min_distance by this factor (stage 2) */
#define POOR_CELL_RATE  0.1               /* defines the portion of the number of regular vectors in a poor cell */
#define POOR_CELL_FROM_ITER 5             /* iteration, at which the poor cell replacing algorithm is started */

#define ORDER_MAX       80                /* Max vector order     */
#define MAX_SIZE        5000              /* Max size (REPLACE_POOR_CELLS)*/
#define NB_BANDS        25                /* number of critical bands */

#define TOTAL_MEM       60000             /* maximum memory allowed for all codebooks */

#define emalloc(n)      (e_heap-=(n))
#define efree(n)        (e_heap+=(n))

#define PI2             6.2831853072f
#define N               256               /* number of points in DFT  */

/* Definition of initialization constants (if not defined, the compiler would give errors) */
#define INIT_DIST_10    5000000.0f         
#define INIT_DIST_11    5000000.0f         
#define INIT_DIST_12    300000.0f 
#define INIT_DIST_13    300000.0f 
#define INIT_DIST_14    300000.0f
#define INIT_DIST_15    300000.0f

#define INIT_DIST_20    300000.0f         
#define INIT_DIST_21    300000.0f         
#define INIT_DIST_22    1000000.0f         
#define INIT_DIST_23    300000.0f         
#define INIT_DIST_24    300000.0f 
#define INIT_DIST_25    300000.0f        
