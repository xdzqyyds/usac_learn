/***********************************************************************************
 
 This software module was originally developed by
 
 Fraunhofer IIS
 
 in the course of development of the ISO/IEC 23003-4 for reference purposes and its
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-4 standard. ISO/IEC gives
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC 23003-4 standard
 and which satisfy any specified conformance criteria. Those intending to use this
 software module in products are advised that its use may infringe existing patents.
 ISO/IEC have no liability for use of this software module or modifications thereof.
 Copyright is not released for products that do not conform to the ISO/IEC 23003-4
 standard.
 
 Fraunhofer IIS retains full right to modify and use the code for its own purpose,
 assign or donate the code to a third party and to inhibit third parties from using
 the code for products that do not conform to MPEG-related ITU Recommendations and/or
 ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2014.
 
 ***********************************************************************************/

#ifndef _UNI_DRC_INTERFACE_WRITER_H_
#define _UNI_DRC_INTERFACE_WRITER_H_

/* ====================================================================================
 Get bits from wobitbuf
 ====================================================================================*/

int
writeBitsUniDrcInterface (wobitbufHandle bitstream,
                          const unsigned int data,
                          const int nBits);

/* ====================================================================================
 Writing of uniDrcInterface()
 ====================================================================================*/

/* Writer for uniDrcInterfaceSignature */
int
writeUniDrcInterfaceSignature(wobitbufHandle bitstream,
                              UniDrcInterfaceSignature* uniDrcInterfaceSignature);

/* Writer for systemInterface */
int
writeSystemInterface(wobitbufHandle bitstream,
                     SystemInterface* systemInterface);

/* Writer for loudnessNormalizationControlInterface */
int
writeLoudnessNormalizationControlInterface(wobitbufHandle bitstream,
                                           LoudnessNormalizationControlInterface* loudnessNormalizationControlInterface);

/* Writer for loudnessNormalizationParameterInterface */
int
writeLoudnessNormalizationParameterInterface(wobitbufHandle bitstream,
                                             LoudnessNormalizationParameterInterface* loudnessNormalizationParameterInterface);

/* Writer for dynamicRangeControlInterface */
int
writeDynamicRangeControlInterface(wobitbufHandle bitstream,
                                  DynamicRangeControlInterface* dynamicRangeControlInterface);

/* Writer for dynamicRangeControlParameterInterface */
int
writeDynamicRangeControlParameterInterface(wobitbufHandle bitstream,
                                           DynamicRangeControlParameterInterface* dynamicRangeControlParameterInterface);

/* Writer for uniDrcInterfaceExtension */
int
writeUniDrcInterfaceExtension(wobitbufHandle bitstream,
                              UniDrcInterfaceExtension* uniDrcInterfaceExtension);

/* Writer for uniDrcInterface */
int
writeUniDrcInterface(wobitbufHandle bitstream,
                     UniDrcInterface* uniDrcInterface);

#endif /* _UNI_DRC_INTERFACE_WRITER_H_ */
