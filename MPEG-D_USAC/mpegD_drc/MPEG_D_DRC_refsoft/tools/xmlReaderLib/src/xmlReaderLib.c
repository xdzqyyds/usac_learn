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

#ifdef __GNUC__
#define XMLREADER_COMPILER_VERSION (   __GNUC__            * 10000 \
                                   + __GNUC_MINOR__       *   100 \
                                   + __GNUC_PATCHLEVEL__  *     1)
#define XMLREADER_COMPILER_INFO "Compiler: GCC"
#else
#ifdef _MSC_VER
  #define XMLREADER_COMPILER_VERSION _MSC_VER
  #define XMLREADER_COMPILER_INFO "Compiler: Visual C"
#else
  #define XMLREADER_COMPILER_VERSION 0
  #define XMLREADER_COMPILER_INFO "Compiler: unknown"
#endif
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include "xmlReaderLib.h"

#define SEPARATECHARWITHOUTSPACE(A) (A=='\n' || A=='\t' || A=='\r')
#define SEPARATECHAR(A) (A==' ' || SEPARATECHARWITHOUTSPACE(A))
#define NAMECHAR(A) (isalpha(A) || isdigit(A) || A=='-' || A=='_')
#define XMLDECLARAION "xml"
#define ENCODINGSTYLE_ISO_8859_1 "ISO-8859-1"
#define ENCODINGSTYLE_UTF_8 "utf-8"
#define ENCODINGSTYLE_UTF_8_UPPERCASE "UTF-8"

/**********************************************************************//**
kinf of xmltype
**************************************************************************/
typedef enum {
  XMLREADER_XMLTYPE_INSTRUCTION,                                        /**< detect an instruction*/
  XMLREADER_XMLTYPE_NODE,                                               /**< detect a node*/
  XMLREADER_XMLTYPE_ENDOFFILE,                                          /**< detect the end of the file*/
  XMLREADER_XMLTYPE_ERROR,                                              /**< cant detect anything*/
} XMLREADER_XMLTYPE, *XMLREADER_XMLTYPE_HANDLE;

/**********************************************************************//**
definition of data for the attributs of an xmlNode
**************************************************************************/
typedef struct xmlreader_attribute_struct {
              struct xmlreader_attribute_struct  *next_attribute;
              char                                   *name;
              char                                   *value;
}XMLREADERLIB_ATTRIBUTE, *XMLREADERLIB_ATTRIBUTE_HANDLE;

/**********************************************************************//**
definition of data for one xmlNode
**************************************************************************/
typedef struct xmlreader_node_struct {
              struct xmlreader_node_struct       *next_node;
              struct xmlreader_node_struct       *first_child;
              struct xmlreader_node_struct       *parent;
              XMLREADERLIB_ATTRIBUTE_HANDLE       attribute;
              char                                   *name;
              char                                   *content;
}XMLREADER_NODE, *XMLREADER_NODE_HANDLE;

/**********************************************************************//**
definition of data for available encoding-styles
**************************************************************************/
typedef enum {
              XMLREADER_ENCODINGSTYLE_ISO_8859_1,
              XMLREADER_ENCODINGSTYLE_UTF_8,
}XMLREADER_ENCODINGSTYLE, *XMLREADER_ENCODINGSTYLE_HANDLE;

/**********************************************************************//**
definition of data for available XML-Declarations
**************************************************************************/
typedef struct xmlreader_xmldeclaration_struct {
              int                                     version;
              int                                     subversion;
              XMLREADER_ENCODINGSTYLE             encoding;
}XMLREADER_XMLDECLARATION, *XMLREADER_XMLDECLARATION_HANDLE;

/**********************************************************************//**
private data needed for this module (unique for every instance of this module)
**************************************************************************/
/** DOXYGEN [XMLREADER_PRIVATE_DATA] **/ /** DOXYGEN [XMLREADER_PRIVATE_DATA_HANDLE] **/
typedef struct xmlreader_private_data_struct {
  int nSize;                                    /**< Size of struct. Do not remove! */

  XMLREADER_NODE mainXmlNode;                     /**< first node of the xml-file */
  XMLREADER_NODE_HANDLE hActXmlNode;              /**< pointer to the actual node when step through the xml-tree */
  int nextStepToChild;                                /**< if set to 1 and the function xmlReaderLib_GetNextNode is called, it tries to get the child of the actNode */
  char * fileContent;                                 /**< content of the xml-file will be stored, and the nodes only get pointers on this datas */
  int lengthContent;                                  /**< length of the fileContent-array */
  int pContent;                                       /**< point on the actual position from the fileContent */
  XMLREADER_XMLDECLARATION xmldeclaration;        /**< store all available xmldeclataion data */
}XMLREADER_PRIVATE_DATA,*XMLREADER_PRIVATE_DATA_HANDLE;
/** DOXYGEN [XMLREADER_PRIVATE_DATA] **/ /** DOXYGEN [XMLREADER_PRIVATE_DATA_HANDLE] **/

/* ######################################################################*/
/* ########################### functions to use  ########################*/
/* ######################################################################*/

/* ######################################################################*/
/* #################### prototyping static functions  ###################*/
/* ######################################################################*/



/**********************************************************************//**
retrieves the private data handle
\return a handle to a private data structure, wich is dedicated to the instance of this module
**************************************************************************/
static XMLREADER_PRIVATE_DATA_HANDLE xmlReaderLib_GetPrivateDataHandle(
              XMLREADER_INSTANCE_HANDLE           const hInstance  /**< in: instance handle */
);

/**********************************************************************//**
initialization of an instance of this module, pass a ptr to a hInstance
\return returns an error code
**************************************************************************/
XMLREADER_RETURN xmlReaderLib_Open(
              XMLREADER_INSTANCE_HANDLE                *phInstance        /**< inout: instance handle */
);


/**********************************************************************//**
Read the char-array from privateData and create the nodes
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_readXmlContent(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate
);

/**********************************************************************//**
Read the char-array from privateData and create the next xmlNode (with subNodes)
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getXmlNode(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate, 
              XMLREADER_XMLTYPE_HANDLE                  xmlType, 
              XMLREADER_NODE_HANDLE                 xmlNode
);

/**********************************************************************//**
Read the char-array from privateData and get the start Type and set pContent on name
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getStartType(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              XMLREADER_XMLTYPE_HANDLE                  xmlType
);

/**********************************************************************//**
Go to the end of the name from the private char-array xmlFileContent with position pContent
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_goToEndOfName(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate
);

/**********************************************************************//**
Read the char-array from privateData and create the next attribute (and all following)
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getNextAttribute(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              XMLREADERLIB_ATTRIBUTE_HANDLE         attribute
);

/**********************************************************************//**
Go to next "<" and either set content, or let it NULL if there is no content
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getNodeContent(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              char                                    **content
);

/**********************************************************************//**
Increment pContent, and check if it is still in range
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_incrementPcontent(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              int                                       increment
);

/**********************************************************************//**
Comment was detected in xmlfile
Ignore the rest of the comment
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_ignoreComment(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate
);

/**********************************************************************//**
Free all memory used by a node, but not the node itself
**************************************************************************/
static void xmlReaderLib_FreeMemoryFromNode(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              XMLREADER_NODE_HANDLE                 node
);

/**********************************************************************//**
Set for the privateData the default-values
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_setDefaultValue(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivateData
);

/**********************************************************************//**
Returns an error, if at least one variable is an error
Returns an Warning, if at least one variable is a warning
Returns alway the value from error as long as error_tmp is not more bad (error>warning>noerror)
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_ErrorHandling(
              XMLREADER_RETURN                          error_old,
              XMLREADER_RETURN                          error_new
);

/**********************************************************************//**
retrieves the private data handle
\return a handle to a private data structure, which is dedicated to the instance of this module
**************************************************************************/
static XMLREADER_PRIVATE_DATA_HANDLE xmlReaderLib_GetPrivateDataHandle(
              XMLREADER_INSTANCE_HANDLE           const hInstance  /**< in: instance handle */
)
{
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  hPrivateData = (XMLREADER_PRIVATE_DATA_HANDLE)((char*)hInstance + sizeof(struct xmlreader_instance_struct));
  return hPrivateData;
}

/**********************************************************************//**
Returns an error, if at least one variable is an error
Returns an Warning, if at least one variable is a warning
Returns alway the value from error as long as error_tmp is not more bad (error>warning>noerror)
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_ErrorHandling(
              XMLREADER_RETURN                          error_old,
              XMLREADER_RETURN                          error_new
)
{
  if(error_old>=XMLREADER_ERROR_FIRST) {
    return error_old;
  }
  if(error_new>=XMLREADER_ERROR_FIRST) {
    return error_new;
  }
  if(error_old!=XMLREADER_NO_ERROR) {
    return error_old;
  }
  if(error_new!=XMLREADER_NO_ERROR) {
    return error_new;
  }
  return XMLREADER_NO_ERROR;
}


/**********************************************************************//**
Read the char-array from privateData and create the nodes
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_readXmlContent(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;
  XMLREADER_XMLTYPE xmlType;
  XMLREADER_NODE xmlDeclaration;
  XMLREADERLIB_ATTRIBUTE_HANDLE hAttribute;

  /* Get first value from the xml-file - this has to be a XML-declaration, this will be checked below */
  error_tmp=xmlReaderLib_getXmlNode(hPrivate, &xmlType, &xmlDeclaration);
  error=xmlReaderLib_ErrorHandling(error, error_tmp);

  if(xmlType != XMLREADER_XMLTYPE_INSTRUCTION) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  }

  if (error<XMLREADER_ERROR_FIRST && xmlDeclaration.name!=NULL && strcmp(xmlDeclaration.name, XMLDECLARAION)!=0) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  }

  /* Set the default Parameters for the xml-declaration */
  hPrivate->xmldeclaration.encoding=XMLREADER_ENCODINGSTYLE_ISO_8859_1;
  hPrivate->xmldeclaration.version=2;
  hPrivate->xmldeclaration.subversion=1;

  hAttribute=xmlDeclaration.attribute;
  while(hAttribute!=NULL && error < XMLREADER_ERROR_FIRST) {
    if(strcmp(hAttribute->name, "version")==0) {
      int num=0;
      char *ptr = strtok(hAttribute->value, ".");

      while(ptr != NULL) {
        if(num==0) {
          hPrivate->xmldeclaration.version=atoi(ptr);
        } else if(num==1) {
          hPrivate->xmldeclaration.subversion=atoi(ptr);
        } else {
          error=xmlReaderLib_ErrorHandling(error, XMLREADER_WARNING_XMLDECLARATION_VERSION_WRONGSTYLE);
        }
        num++;
        ptr = strtok(NULL, ".");
      }
      if(num!=2) {
        error=xmlReaderLib_ErrorHandling(error, XMLREADER_WARNING_XMLDECLARATION_VERSION_WRONGSTYLE);
      }
      /* check here the supported versions - for the moment only 1.0*/
      if(hPrivate->xmldeclaration.version!=1 || hPrivate->xmldeclaration.subversion!=0) {
        error=xmlReaderLib_ErrorHandling(error, XMLREADER_WARNING_UNSUPPORTED_VERSION);
      }
    } else if (strcmp(hAttribute->name, "encoding")==0) {
      if(strcmp(hAttribute->value, ENCODINGSTYLE_ISO_8859_1)==0) {
        hPrivate->xmldeclaration.encoding=XMLREADER_ENCODINGSTYLE_ISO_8859_1;
      } else if (strcmp(hAttribute->value, ENCODINGSTYLE_UTF_8)==0 || strcmp(hAttribute->value, ENCODINGSTYLE_UTF_8_UPPERCASE)==0) {
        hPrivate->xmldeclaration.encoding=XMLREADER_ENCODINGSTYLE_UTF_8;
      } else {
        error=xmlReaderLib_ErrorHandling(error, XMLREADER_WARNING_UNSUPPORTED_ENCODINGSTYLE);
      }
    } else {
      error=xmlReaderLib_ErrorHandling(error, XMLREADER_WARNING_UNKNOWN_ATTRIBUTE_IN_XMLDECLARATION);
    }
    hAttribute=hAttribute->next_attribute;
  }

  /* free memory from local variable */
  xmlReaderLib_FreeMemoryFromNode(hPrivate, &xmlDeclaration);

  if(error<XMLREADER_ERROR_FIRST) {

    error_tmp=xmlReaderLib_getXmlNode(hPrivate, &xmlType, &hPrivate->mainXmlNode);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);

    /* main node has to be a single node, also it should be impossible to be able to set this value */
    if(hPrivate->mainXmlNode.next_node!=NULL)
      error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);

  }

  if(error<XMLREADER_ERROR_FIRST) {
    /* Now the end of the file has to be reached (xmlType == XMLREADER_XMLTYPE_ENDOFFILE ) */
    error_tmp=xmlReaderLib_getXmlNode(hPrivate, &xmlType, &xmlDeclaration);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);

    if(xmlType != XMLREADER_XMLTYPE_ENDOFFILE)
      error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);

    /* free memory from local variable - should nothing be allocated, only to be sure */
    xmlReaderLib_FreeMemoryFromNode(hPrivate, &xmlDeclaration);

  }

  return error;
}


/**********************************************************************//**
Read the char-array from privateData and create the next xmlNode (with subNodes)
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getXmlNode(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate, 
              XMLREADER_XMLTYPE_HANDLE                  xmlType, 
              XMLREADER_NODE_HANDLE                 xmlNode
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;
  char *endName;

  *xmlType=XMLREADER_XMLTYPE_ERROR;
  xmlNode->attribute=NULL;
  xmlNode->content=NULL;
  xmlNode->first_child=NULL;
  xmlNode->name=NULL;
  xmlNode->next_node=NULL;
  xmlNode->parent=NULL;

  error_tmp=xmlReaderLib_getStartType(hPrivate, xmlType);
  error=xmlReaderLib_ErrorHandling(error, error_tmp);

  
  if(error<XMLREADER_ERROR_FIRST && (*xmlType==XMLREADER_XMLTYPE_INSTRUCTION || *xmlType==XMLREADER_XMLTYPE_NODE) ) {
    xmlNode->name=&hPrivate->fileContent[hPrivate->pContent];

    error=xmlReaderLib_goToEndOfName(hPrivate);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);

    /* this char will be \0 in the end */
    endName=&hPrivate->fileContent[hPrivate->pContent];

    if(error<XMLREADER_ERROR_FIRST) {
      /* Skip white space */
      while(SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent])) {
        error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
        error=xmlReaderLib_ErrorHandling(error, error_tmp);
	  }

      /* Possible attributes */
	  if (NAMECHAR(hPrivate->fileContent[hPrivate->pContent])) {
        xmlNode->attribute=(XMLREADERLIB_ATTRIBUTE_HANDLE)calloc(1,sizeof(XMLREADERLIB_ATTRIBUTE));
        error_tmp=xmlReaderLib_getNextAttribute(hPrivate, xmlNode->attribute);
        error=xmlReaderLib_ErrorHandling(error, error_tmp);
      }
    }

    if(error<XMLREADER_ERROR_FIRST) {
      if( *xmlType==XMLREADER_XMLTYPE_INSTRUCTION && strncmp(&hPrivate->fileContent[hPrivate->pContent], "?>", 2)==0 ) {
        /* correct end of instruction */
        *endName='\0';
        error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 2);
        error=xmlReaderLib_ErrorHandling(error, error_tmp);
      } else if( *xmlType==XMLREADER_XMLTYPE_NODE && strncmp(&hPrivate->fileContent[hPrivate->pContent], "/>", 2)==0 ) {
        /* node close directly */
        *endName='\0';
        /*no content and no subNodes, so point on '\0' for the content*/
        xmlNode->content=endName;
        error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 2);
        error=xmlReaderLib_ErrorHandling(error, error_tmp);
      } else if( *xmlType==XMLREADER_XMLTYPE_NODE && strncmp(&hPrivate->fileContent[hPrivate->pContent], ">", 1)==0 ) {
        /* now the content of the node starts */
        char *endContent = NULL;
        char *endNodeName = NULL;
        XMLREADER_NODE_HANDLE *nextNode=&xmlNode->first_child;

        *endName='\0';

        error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
        error=xmlReaderLib_ErrorHandling(error, error_tmp);

        if(error<XMLREADER_ERROR_FIRST) {
          error_tmp=xmlReaderLib_getNodeContent(hPrivate, &xmlNode->content);
          error=xmlReaderLib_ErrorHandling(error, error_tmp);
        }

        endContent=&hPrivate->fileContent[hPrivate->pContent];

        while ( error<XMLREADER_ERROR_FIRST && strncmp(&hPrivate->fileContent[hPrivate->pContent], "</", 2)!=0 ) {
          if( strncmp(&hPrivate->fileContent[hPrivate->pContent], "<!--", 4)==0 ) {
            error_tmp=xmlReaderLib_ignoreComment(hPrivate);
            error=xmlReaderLib_ErrorHandling(error, error_tmp);
          } else if( strncmp(&hPrivate->fileContent[hPrivate->pContent], "<", 1)==0 ) {
            XMLREADER_XMLTYPE new_xmlType = 0;
            if(xmlNode->content!=NULL) {
              error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
            }
            /*new child*/
            
            *nextNode=(XMLREADER_NODE_HANDLE)calloc(1,sizeof(XMLREADER_NODE));
            
            if(error<XMLREADER_ERROR_FIRST) {
              error_tmp=xmlReaderLib_getXmlNode(hPrivate, &new_xmlType, *nextNode);
              error=xmlReaderLib_ErrorHandling(error, error_tmp);
            }

            if(error<XMLREADER_ERROR_FIRST) {
              if(new_xmlType==XMLREADER_XMLTYPE_NODE) {
                (*nextNode)->parent=xmlNode;
                nextNode=&(*nextNode)->next_node;
              } else if(new_xmlType==XMLREADER_XMLTYPE_INSTRUCTION) {
                free(*nextNode);
                error=xmlReaderLib_ErrorHandling(error, XMLREADER_WARNING_INSTRUCTION_UNSUPPORTED);
              } else {
                free(*nextNode);
                error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XMLTYPE);
              }
            }
          } else if(SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent])) {
            error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
            error=xmlReaderLib_ErrorHandling(error, error_tmp);
          } else {
            error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
          }
        }
        if (error<XMLREADER_ERROR_FIRST) {
          *endContent='\0';
          /*remove the NULL-pointer from content*/
          if(xmlNode->content==NULL) {
            xmlNode->content=endContent;
          }
          error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 2);
          error=xmlReaderLib_ErrorHandling(error, error_tmp);
        }
        if (error<XMLREADER_ERROR_FIRST) {
          endNodeName=&hPrivate->fileContent[hPrivate->pContent];
          error_tmp=xmlReaderLib_goToEndOfName(hPrivate);
          error=xmlReaderLib_ErrorHandling(error, error_tmp);
        }

        if(hPrivate->fileContent[hPrivate->pContent]!='>') {
          error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
        }
        if(error<XMLREADER_ERROR_FIRST) {
          hPrivate->fileContent[hPrivate->pContent]='\0';
          error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
          error=xmlReaderLib_ErrorHandling(error, error_tmp);
          if(strcmp(endNodeName, xmlNode->name) != 0) {
            error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
          }
        }
      } else {
        *endName='\0';
        error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
      }
    }
    
  } else if ( *xmlType!=XMLREADER_XMLTYPE_ENDOFFILE ) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XMLTYPE);
  }
  return error;
}


/**********************************************************************//**
Read the char-array from privateData and get the start Type and set pContent on name
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getStartType(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              XMLREADER_XMLTYPE_HANDLE                  xmlType
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;

  while( error<XMLREADER_ERROR_FIRST && *xmlType!=XMLREADER_XMLTYPE_INSTRUCTION && *xmlType!=XMLREADER_XMLTYPE_NODE && *xmlType!=XMLREADER_XMLTYPE_ENDOFFILE) {
    if( hPrivate->fileContent[hPrivate->pContent]=='\0' ) {
      *xmlType=XMLREADER_XMLTYPE_ENDOFFILE;
    } else if( strncmp(&hPrivate->fileContent[hPrivate->pContent], "<!--", 4)==0 ) {
      /* comment */
      error_tmp=xmlReaderLib_ignoreComment(hPrivate);
      error=xmlReaderLib_ErrorHandling(error, error_tmp);
    } else if( strncmp(&hPrivate->fileContent[hPrivate->pContent], "<?", 2)==0 ) {
      /* instruction */
      error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 2);
      error=xmlReaderLib_ErrorHandling(error, error_tmp);
      *xmlType=XMLREADER_XMLTYPE_INSTRUCTION;
    } else if( strncmp(&hPrivate->fileContent[hPrivate->pContent], "<", 1)==0 ) {
      /* node */
      error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
      error=xmlReaderLib_ErrorHandling(error, error_tmp);
      *xmlType=XMLREADER_XMLTYPE_NODE;
    } else if( SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
      error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
      error=xmlReaderLib_ErrorHandling(error, error_tmp);
    } else {
      *xmlType=XMLREADER_XMLTYPE_ERROR;
      error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
    }
  }

  return error;
}


/**********************************************************************//**
Go to the end of the name from the private char-array xmlFileContent with position pContent
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_goToEndOfName(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;

  if(!NAMECHAR(hPrivate->fileContent[hPrivate->pContent])) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  }
  while(error<XMLREADER_ERROR_FIRST && NAMECHAR(hPrivate->fileContent[hPrivate->pContent])) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  return error;
}


/**********************************************************************//**
Read the char-array from privateData and create the next attribute (and all following)
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getNextAttribute(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              XMLREADERLIB_ATTRIBUTE_HANDLE         attribute
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;

  attribute->next_attribute=NULL;
  attribute->name=NULL;
  attribute->value=NULL;

  while(error<XMLREADER_ERROR_FIRST && SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if(NAMECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    attribute->name=&hPrivate->fileContent[hPrivate->pContent];
  } else {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  }

  while(error<XMLREADER_ERROR_FIRST && NAMECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  while(error<XMLREADER_ERROR_FIRST && SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    hPrivate->fileContent[hPrivate->pContent]='\0';
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if(hPrivate->fileContent[hPrivate->pContent]!='=' ) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  } else if(error<XMLREADER_ERROR_FIRST) {
    hPrivate->fileContent[hPrivate->pContent]='\0';
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  while(error<XMLREADER_ERROR_FIRST && SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if(hPrivate->fileContent[hPrivate->pContent]!='"' && hPrivate->fileContent[hPrivate->pContent]!='\'' ) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  } else if(error<XMLREADER_ERROR_FIRST) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if( error<XMLREADER_ERROR_FIRST ) {
    attribute->value=&hPrivate->fileContent[hPrivate->pContent];
  }

  while(error<XMLREADER_ERROR_FIRST && hPrivate->fileContent[hPrivate->pContent]!='"' && hPrivate->fileContent[hPrivate->pContent]!='\'' ) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if( error<XMLREADER_ERROR_FIRST ) {
    hPrivate->fileContent[hPrivate->pContent]='\0';
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }


  while(error<XMLREADER_ERROR_FIRST && SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if(error<XMLREADER_ERROR_FIRST) {
    if(NAMECHAR(hPrivate->fileContent[hPrivate->pContent])) {
      attribute->next_attribute=(XMLREADERLIB_ATTRIBUTE_HANDLE)calloc(1,sizeof(XMLREADERLIB_ATTRIBUTE));
      error_tmp=xmlReaderLib_getNextAttribute(hPrivate, attribute->next_attribute);
      error=xmlReaderLib_ErrorHandling(error, error_tmp);
    }
  }

  return error;
}


/**********************************************************************//**
Go to next "<" and either set content, or let it NULL if there is no content
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_getNodeContent(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              char                                    **content
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;
  *content=NULL;

  while(error<XMLREADER_ERROR_FIRST && hPrivate->fileContent[hPrivate->pContent]!='<' && SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if(error<XMLREADER_ERROR_FIRST && hPrivate->fileContent[hPrivate->pContent]!='<')
    *content=&hPrivate->fileContent[hPrivate->pContent];

  while(error<XMLREADER_ERROR_FIRST && hPrivate->fileContent[hPrivate->pContent]!='<' && !SEPARATECHARWITHOUTSPACE(hPrivate->fileContent[hPrivate->pContent]) ) {
    if(hPrivate->fileContent[hPrivate->pContent]=='\0') {
      error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
    }
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  while(error<XMLREADER_ERROR_FIRST && hPrivate->fileContent[hPrivate->pContent]!='<' && SEPARATECHAR(hPrivate->fileContent[hPrivate->pContent]) ) {
    hPrivate->fileContent[hPrivate->pContent]='\0';
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }

  if(error<XMLREADER_ERROR_FIRST && hPrivate->fileContent[hPrivate->pContent]!='<') {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
  }

  return error;
}


/**********************************************************************//**
Increment pContent, and check if it is still in range
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_incrementPcontent(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              int                                       increment
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  hPrivate->pContent+=increment;
  /* the array is at least hPrivate->lengthContent+1, so you can go up to hPrivate->pContent==hPrivate->lengthContent */
  if(hPrivate->pContent>hPrivate->lengthContent) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_CONTENT_HANDLING_ERROR);
  }
  return error;
}


/**********************************************************************//**
Comment was detected in xmlfile
Ignore the rest of the comment
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_ignoreComment(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  XMLREADER_RETURN error_tmp;
  if(strncmp(&hPrivate->fileContent[hPrivate->pContent], "<!--", 4)!=0) {
    error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_WRONG_CALL_FUNCTION);
  } else {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 4);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
  }
  while(error<XMLREADER_ERROR_FIRST && strncmp(&hPrivate->fileContent[hPrivate->pContent], "-->", 3) && hPrivate->fileContent[hPrivate->pContent]!='\0') {
    error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 1);
    error=xmlReaderLib_ErrorHandling(error, error_tmp);
    if (hPrivate->fileContent[hPrivate->pContent]=='\0') {
      error=xmlReaderLib_ErrorHandling(error, XMLREADER_ERROR_INVALID_XML_FILE);
    }
  }

  error_tmp=xmlReaderLib_incrementPcontent(hPrivate, 3);
  error=xmlReaderLib_ErrorHandling(error, error_tmp);
  
  return error;
}


/**********************************************************************//**
Free all memory used by a node, but not the node itself
**************************************************************************/
static void xmlReaderLib_FreeMemoryFromNode(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivate,
              XMLREADER_NODE_HANDLE                 node
)
{
  if(node != NULL) {
    XMLREADERLIB_ATTRIBUTE_HANDLE actAttribute=node->attribute;
    XMLREADER_NODE_HANDLE actNode=node->first_child;
    node->attribute=NULL;
    node->first_child=NULL;
    while(actAttribute!=NULL) {
      XMLREADERLIB_ATTRIBUTE_HANDLE nextAttribute=actAttribute->next_attribute;
      actAttribute->next_attribute=NULL;
      free(actAttribute);
      actAttribute=nextAttribute;
    }

    while(actNode!=NULL) {
      XMLREADER_NODE_HANDLE nextNode=actNode->next_node;
      actNode->next_node=NULL;
      xmlReaderLib_FreeMemoryFromNode(hPrivate, actNode);
      free(actNode);
      actNode=nextNode;
    }
    assert(node->first_child==NULL);
    assert(node->next_node==NULL);
    assert(node->attribute==NULL);
  }
}


/**********************************************************************//**
Set for the privateData the default-values
**************************************************************************/
static XMLREADER_RETURN xmlReaderLib_setDefaultValue(
              XMLREADER_PRIVATE_DATA_HANDLE             hPrivateData
)
{
  XMLREADER_RETURN error=XMLREADER_NO_ERROR;
  hPrivateData->hActXmlNode=NULL;
  hPrivateData->mainXmlNode.attribute=NULL;
  hPrivateData->mainXmlNode.content=NULL;
  hPrivateData->mainXmlNode.first_child=NULL;
  hPrivateData->mainXmlNode.name=NULL;
  hPrivateData->mainXmlNode.next_node=NULL;
  hPrivateData->mainXmlNode.parent=NULL;
  hPrivateData->nextStepToChild=0;
  hPrivateData->xmldeclaration.encoding=XMLREADER_ENCODINGSTYLE_ISO_8859_1;
  hPrivateData->xmldeclaration.version=1;
  hPrivateData->xmldeclaration.subversion=0;
  hPrivateData->fileContent=NULL;
  hPrivateData->lengthContent=0;
  hPrivateData->pContent=0;
  
  return error;
}


/* ######################################################################*/
/* ######################### public functions  ##########################*/
/* ######################################################################*/

/**********************************************************************//**
initialization of an instance of this module, pass a ptr to a hInstance
\return returns an error code
**************************************************************************/
XMLREADER_RETURN xmlReaderLib_Open(
              XMLREADER_INSTANCE_HANDLE                *phInstance        /**< inout: instance handle */
)
{
  XMLREADER_RETURN retError = XMLREADER_NO_ERROR;
  XMLREADER_RETURN retError_tmp;
  int nSize = 0;
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  
  nSize += sizeof(struct xmlreader_instance_struct);
  nSize += sizeof(struct xmlreader_private_data_struct);
  
  *phInstance = (XMLREADER_INSTANCE_HANDLE)calloc(1, sizeof(struct xmlreader_instance_struct)+sizeof(struct xmlreader_private_data_struct));
  
  if (*phInstance != NULL) {
    memset(*phInstance,0,nSize);
    hPrivateData = (XMLREADER_PRIVATE_DATA_HANDLE)((char*)(*phInstance) + sizeof(struct xmlreader_instance_struct));
    hPrivateData->nSize = nSize;

    /* allocate additional memory below here and fill it with data from xml-file */
    retError_tmp=xmlReaderLib_setDefaultValue(hPrivateData);
    retError=xmlReaderLib_ErrorHandling(retError, retError_tmp);

    hPrivateData->hActXmlNode=NULL;

  } else {
    retError = XMLREADER_ERROR_MEMORY;
  }
  return retError;
}

/**********************************************************************//**
initialization of an instance of this module, pass a ptr to a hInstance
\return returns an error code
**************************************************************************/
XMLREADER_RETURN xmlReaderLib_New(
              XMLREADER_INSTANCE_HANDLE                *phInstance,       /**< inout: instance handle */
              char                               const *filename          /**< in: set the name of the xml-file */
)
{
  XMLREADER_RETURN retError = XMLREADER_NO_ERROR;
  XMLREADER_RETURN retError_tmp;
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData=NULL;
  FILE  *pFile = NULL;
  retError_tmp=xmlReaderLib_Open(phInstance);
  retError=xmlReaderLib_ErrorHandling(retError, retError_tmp);
  if(retError<XMLREADER_ERROR_FIRST) {
    hPrivateData=xmlReaderLib_GetPrivateDataHandle(*phInstance);
    if((pFile=fopen(filename, "r")) == NULL )
      retError=xmlReaderLib_ErrorHandling(retError, XMLREADER_ERROR_CANT_OPEN_FILE);
  }
  if(retError<XMLREADER_ERROR_FIRST) {
    if (fseek(pFile, 0L, SEEK_END) != 0) {
      retError=xmlReaderLib_ErrorHandling(retError, XMLREADER_ERROR_HANDLING_FILE);
    }
  }
  if(retError<XMLREADER_ERROR_FIRST) {
    if( 0 > (hPrivateData->lengthContent = (int)ftell(pFile)) ) {
      retError=xmlReaderLib_ErrorHandling(retError, XMLREADER_ERROR_HANDLING_FILE);
    }
  }
  if(retError<XMLREADER_ERROR_FIRST) {
      hPrivateData->fileContent = (char *)calloc(hPrivateData->lengthContent+1, sizeof(char));
    if( NULL == hPrivateData->fileContent ) {
      retError=xmlReaderLib_ErrorHandling(retError, XMLREADER_ERROR_MEMORY);
    }
  }
  if(retError<XMLREADER_ERROR_FIRST) {
    if (fseek(pFile, 0L, SEEK_SET) != 0) {
      retError=xmlReaderLib_ErrorHandling(retError, XMLREADER_ERROR_HANDLING_FILE);
    }
  }
  if(retError<XMLREADER_ERROR_FIRST) {
    hPrivateData->lengthContent = fread(hPrivateData->fileContent, 1, hPrivateData->lengthContent, pFile);
  }
  if(retError<XMLREADER_ERROR_FIRST) {
    hPrivateData->fileContent[hPrivateData->lengthContent]='\0';
    if( EOF == fclose(pFile) ) {
      retError=xmlReaderLib_ErrorHandling(retError, XMLREADER_ERROR_HANDLING_FILE);
    }
    pFile=NULL;
  }

  if(retError<XMLREADER_ERROR_FIRST) {
    retError_tmp=xmlReaderLib_readXmlContent(hPrivateData);
    retError=xmlReaderLib_ErrorHandling(retError, retError_tmp);
  }

  /*If there is an error, free all memory*/
  if(retError>=XMLREADER_ERROR_FIRST) {
    xmlReaderLib_FreeMemoryFromNode(hPrivateData, &hPrivateData->mainXmlNode);
    if(hPrivateData->fileContent!=NULL) {
      free(hPrivateData->fileContent);
      hPrivateData->fileContent=NULL;
    }
  }
  return retError;
}

/**********************************************************************//**
deletes an instance of this module
\return returns an error code
**************************************************************************/
XMLREADER_RETURN xmlReaderLib_Delete(
               XMLREADER_INSTANCE_HANDLE                hInstance  /**< in: instance handle */
)
{
  XMLREADER_RETURN retError = XMLREADER_NO_ERROR;
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;

  if (hInstance != NULL) {
    hPrivateData   = xmlReaderLib_GetPrivateDataHandle(hInstance);

    /* free additional memory, if you allocated something */  
    xmlReaderLib_FreeMemoryFromNode(hPrivateData, &hPrivateData->mainXmlNode);

    if (hPrivateData->fileContent != NULL) {
      free(hPrivateData->fileContent);
      hPrivateData->fileContent=NULL;
    }

    free(hInstance);
  } else {
	  retError = XMLREADER_ERROR_INVALID_HANDLE;
  }
  return retError;
}


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************//**
get name from the actual XML-node
if hInstance is NULL it returns NULL
**************************************************************************/
const char * xmlReaderLib_GetNodeName(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
)
{
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  if(hInstance!=NULL) {
    hPrivateData = xmlReaderLib_GetPrivateDataHandle(hInstance);
    if(hPrivateData->hActXmlNode==NULL) {
      hPrivateData->hActXmlNode=&hPrivateData->mainXmlNode;
    }
    return hPrivateData->hActXmlNode->name;
  }
  return NULL;
}


/**********************************************************************//**
get content from the XML-node
if hInstance is NULL it returns NULL
**************************************************************************/
const char * xmlReaderLib_GetContent(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
)
{
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  if(hInstance!=NULL) {
    hPrivateData = xmlReaderLib_GetPrivateDataHandle(hInstance);
    if(hPrivateData->hActXmlNode==NULL) {
      hPrivateData->hActXmlNode=&hPrivateData->mainXmlNode;
    }
    return hPrivateData->hActXmlNode->content;
  }
  return NULL;
}

/**********************************************************************//**
goes to the deeper hierarchy, but it didn't return the first child
to get the first child call xmlReaderLib_GetNextNode
**************************************************************************/
void xmlReaderLib_GoToChilds(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
)
{
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  if(hInstance != NULL) {
    hPrivateData = xmlReaderLib_GetPrivateDataHandle(hInstance);
    hPrivateData->nextStepToChild=1;
  }
}


/**********************************************************************//**
get next node in the same hierarchy
if there is no next node, it goes to the parent-node and returns NULL
if hInstance is NULL it returns NULL
otherwise it returns the nodename
**************************************************************************/
const char * xmlReaderLib_GetNextNode(
              XMLREADER_INSTANCE_HANDLE                         hInstance               /**< inout: instance handle */
)
{
  XMLREADER_PRIVATE_DATA_HANDLE hPrivateData = NULL;
  XMLREADER_NODE_HANDLE parent = NULL;
  if(hInstance != NULL) {
    hPrivateData = xmlReaderLib_GetPrivateDataHandle(hInstance);
    if(hPrivateData->hActXmlNode==NULL) {
      hPrivateData->hActXmlNode=&hPrivateData->mainXmlNode;
      parent=NULL;
    } else if(hPrivateData->nextStepToChild==1) {
      hPrivateData->nextStepToChild=0;
      parent=hPrivateData->hActXmlNode;
      hPrivateData->hActXmlNode=hPrivateData->hActXmlNode->first_child;
    } else {
      parent=hPrivateData->hActXmlNode->parent;
      hPrivateData->hActXmlNode=hPrivateData->hActXmlNode->next_node;
    }
    if(hPrivateData->hActXmlNode==NULL) {
      if(parent==NULL) {
        hPrivateData->hActXmlNode=&hPrivateData->mainXmlNode;
        return hPrivateData->hActXmlNode->name;
      } else {
        hPrivateData->hActXmlNode=parent;
        return NULL;
      }
    } else {
      return hPrivateData->hActXmlNode->name;
    }
  }
  return NULL;
}
