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
 
 Copyright (c) ISO/IEC 2016.
 
 ***********************************************************************************/

#ifndef _XML_READER_LIB_H
#define _XML_READER_LIB_H

/* --------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif
/* --------------------------------------------------------------------- */

/* ######################################################################*/
/* ########################### structure alignment  #####################*/
/* ######################################################################*/
#if defined(WIN32) || defined(WIN64)
#define XMLREADER_API __stdcall
#pragma pack(push, 1)
#else
#define XMLREADER_API
#endif

/* ######################################################################*/
/* ########################### module  defines  #########################*/
/* ######################################################################*/
#define XMLREADER_ERROR_FIRST                                   1000                    /**< defines the smallest value to interprete the return-value as error */
#define XMLREADER_MAX_CHAR_LENGTH                               255                     /**< all node-names content attribute-names attribute-values and instruction-names has to be smaller than this number */

/*! The error return values */
typedef enum {
  XMLREADER_NO_ERROR                      = 0,                                          /**< defines the return-value interpreted as no error */
  XMLREADER_WARNING_UNKNOWN_ATTRIBUTE_IN_XMLDECLARATION,                                /**< this attribute from the xmldeclaration is not known */
  XMLREADER_WARNING_UNSUPPORTED_ENCODINGSTYLE,                                          /**< the encoding style is not supported */
  XMLREADER_WARNING_XMLDECLARATION_VERSION_WRONGSTYLE,                                  /**< the value for the xml-version has not the style x.y */
  XMLREADER_WARNING_UNSUPPORTED_VERSION,                                                /**< the version is not supported */
  XMLREADER_WARNING_INSTRUCTION_UNSUPPORTED,                                            /**< there is an instruction in the xml-file, but instructions are unsupported up to now */
  XMLREADER_WARNING_UNKOWN_ATTRIBUTE_FOR_ACT_NODE,                                      /**< the attribute asking for is not known in the node */
  XMLREADER_WARNING_HAS_CHILD_NODE_BUT_ASKED_FOR_CONTENT,                               /**< asked for content, also a child-node is available */
  XMLREADER_WARNING_HAS_CONTENT_BUT_ASKED_CHILD,                                        /**< asked for child-nodes, also content is available */
  XMLREADER_WARNING_NEXTSTEPTOCHILD_IS_SET_UNDEFINED_RETURN,                            /**< after the function xmlReaderLib_GoToChilds is called only the function xmlReaderLib_GetNextNode shall be called */
  XMLREADER_WARNING_CALL_NEXT_NODE_AFTER_RETURNED_TO_MAIN_NODE,                         /**< also the hole xml-file is parsed, it is asked for the next node. Start with a new round */
  XMLREADER_ERROR_INVALID_HANDLE                              = XMLREADER_ERROR_FIRST,  /**< an pointer-parameter is NULL */
  XMLREADER_ERROR_MEMORY,                                                               /**< error with the memory */
  XMLREADER_ERROR_WRONG_CALL_FUNCTION,                                                  /**< if the function was not allowed to be called in this situation */
  XMLREADER_ERROR_INVALID_XML_FILE,                                                     /**< the xml-file is no correct xml-sytax */
  XMLREADER_ERROR_CONTENT_HANDLING_ERROR,                                               /**< error with handling the char-array which contains the content of the xml-file */
  XMLREADER_ERROR_INVALID_XMLTYPE,                                                      /**< type is only defined for node and instruction */

  XMLREADER_ERROR_INVALID_START_VALUES_FOR_FILLING_XML_NODE,                            /**< the values before filling an node are invalid */
  XMLREADER_ERROR_CANT_OPEN_FILE,                                                       /**< error while open xml-file */
  XMLREADER_ERROR_HANDLING_FILE,                                                        /**< error while handling with xml-file */
  XMLREADER_ERROR_INVALID_FILEREADERSTATE,                                              /**< the internal state from the filereader is corrupted */
  XMLREADER_ERROR_TO_BIG_CONTENT,                                                       /**< a content, nodename, ... is larger than XMLREADER_MAX_CHAR_LENGTH */
} XMLREADER_RETURN;

/* ######################################################################*/
/* ########################### module  data  ############################*/
/* ######################################################################*/

/**********************************************************************//**
definiton of public data for this module
**************************************************************************/
typedef struct xmlreader_instance_struct {
              char                                              infoModuleVersion[64];  /**< module and version information */
}XMLREADER_INSTANCE, *XMLREADER_INSTANCE_HANDLE;

/**********************************************************************//**
initialization of an instance of this module, pass a ptr to a hInstance
\return returns a ERROR_INFO struct in case errorhnd.h is included prior to this headerfile
**************************************************************************/
XMLREADER_RETURN xmlReaderLib_New( 
              XMLREADER_INSTANCE_HANDLE                        *phInstance,             /**< inout: instance handle */
              char                                       const *filename                /**< in: set the name of the xml-file */
);

/**********************************************************************//**
deletes an instance of this module
**************************************************************************/
XMLREADER_RETURN xmlReaderLib_Delete(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */            
);

/**********************************************************************//**
get name from the actual XML-node
if hInstance is NULL it returns NULL
**************************************************************************/
const char * xmlReaderLib_GetNodeName(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
);

/**********************************************************************//**
get content from the XML-node
if hInstance is NULL it returns NULL
**************************************************************************/
const char * xmlReaderLib_GetContent(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
);

/**********************************************************************//**
goes to the deeper hierarchy, but it didn't return the first child
to get the first child call xmlReaderLib_GetNextNode
**************************************************************************/
void xmlReaderLib_GoToChilds(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
);

/**********************************************************************//**
get next node in the same hierarchy
if there is no next node, it goes to the parent-node and returns NULL
if hInstance is NULL it returns NULL
otherwise it returns the nodename
**************************************************************************/
const char * xmlReaderLib_GetNextNode(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
);


#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif


/* --------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
/* --------------------------------------------------------------------- */

#endif /* _XML_READER_LIB_H */

