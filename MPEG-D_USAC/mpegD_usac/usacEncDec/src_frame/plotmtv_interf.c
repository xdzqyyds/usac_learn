/**********************************************************************
  MPEG-4 Audio VM


  This software module was originally developed by

  Bodo Teichmann Fraunhofer Institute of Erlangen tmn@iis.fhg.de

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

  Copyright (c) 1997.



  file: plotmtv_interv.c

 

  Authors:
  tmn  Bodo Teichmann  tmn@iis.fhg.de
  HP   Heiko Purnhagen  purnhage@tnt.uni-hannover.de

  Changes:
  xx-apr-97  BT   contributed to VM
  22-may-97  HP   added abs(real,imag) MTV_CPLXFLOAT
                  plotDirect("",MTV_CPLXFLOAT,npts,re1,im1,re2,im2)

**********************************************************************/
/* plotmtv_interf.c is just a C interface to the freeware program plotmtv, which  */
/* is a graphical data-display program for X-windows with a nice user interface. */
/* with this C interface it can be used to display 1-dim arrays (eg. the mdct spectrum)  */
/* directly from the debugger while debugging with the gdb command 
   'call plotDirect("",MTV_DOUBLE,npts,array1,array2,array3,array4). */
/* it is very very usefull for debugging; please do not remove the interface files from the VM frame work  */
/* i will sent an executable (sgi,linux,solaris,sunos) of plotmtv to everybody who wants to use it */
/* the source is available from ftp://ftp.th-darmstadt.de/pub/X11/contrib/applications/Plotmtv1.4.1.tar.Z and other ftp sites */
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdarg.h>          /* variable artgument list */
#include <stdlib.h>
#ifndef WIN32
#include <sys/wait.h>
#include <sys/stat.h>
#endif
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "plotmtv.h"
#include "common_m4a.h"
/**** user changable options : *******************************/
#define PRINTER_OPT "-Pml6" /* should be set to -Pprinter_name eg: -Pml6 */
#define PLOTSET_TMP "/tmp" /* 1st praefix for plotset files , should be /tmp/ */
#define PLOTSET_PRAEF0 "/" /* 1st praefix for plotset files , should be /tmp/ */
#define PLOTSET_PRAEF1 "/_"/* 2nd praefix for plotset files , should be /tmp/_ */
#define BG_COLOR "black" /* background color */
#define ERR_FNAME "plotmtv.err"
#define LOG_FNAME "plotmtv.log"
static char plotFileNameEnc[1024]="plotmtvenc.rc"; /* name of the control file, could be changed from commandline options of user programm */
static char plotFileNameDec[1024]="plotmtvdec.rc"; /* name of the control file, could be changed from commandline options of user programm */
static int plotPaperPlot = 0; /* for colour plot set to  0, for monochrom set to  1 , than the lines are plotted with markers*/ 
int plotChannel = 0; /* channel number that shall be displayed , is used only in the user programm */
static int enable_plotmtv = 0 ; /* if set to zero plotmtv is switched off */
int firstFrame = 9999;
int framePlot = 0; /* from user program , only used to display the frame and granule number */

/* these defines set the window geometry: X-pixel x Y-pixel + X-offset + Y-offset */
#define WIN_GEOM_1 "750x600+1+1" 
#define WIN_GEOM_2 "750x600+1+1"
#define WIN_GEOM_3 "750x900+1+1"
#define WIN_GEOM_4 "1100x600+1+1"
#define WIN_GEOM_5 "1200x980+1+1"
#define WIN_GEOM_6 "1200x980+1+1"

/**** user changable options end ******************************/
static char    errorFilename[]= ERR_FNAME ;
static char    logFilename[]= LOG_FNAME ;
#define MAX_LINE_SIZE 1024
static char  plotsetPreafix[MAX_LINE_SIZE];
static char  plotsetPreafixTmp[MAX_LINE_SIZE];
static char  plotsetPreafix0[MAX_LINE_SIZE];
static char  plotsetPreafix1[MAX_LINE_SIZE];
/* error handling : */

#define ACTIV      1
#define DELETED    0
#define TRUE       1
#define FALSE      0
#define STRING_SIZE 1024
#define DISAB_MESS                   0x00001
#define INIT_ERROR                   0x00002
#define INV_DATAFORMAT               0x00003
#define FILE_MISSING                 0x00004
#define WRITE_BIN                    0x00005
#define FATAL_ERROR                  0x00006
#define FOR_TESTING                  0x00012
#define MESSAGE1                     0x00013
#define FORK_FAILED                  0x00014
#define PLOT_RC                     0x00015

/* static ERROR_MESSAGE err_mess[ ] = */
/*   { */
/*      {DISAB_MESS,"\n The resource file '%s' does not exist, plotmtv disabled "},   */
/*      {INIT_ERROR,"\n Initialisation error while reading resource file in mplot"},  */
/*      {INV_DATAFORMAT,"\n Invalid data format for plotmtv"}, */
/*      {FILE_MISSING  ,"\n File %s missing for plotmtv"}, */
/*      {WRITE_BIN,"\n Error while writing binary datafile for plotmtv"}, */
/*      {FATAL_ERROR,"\n Fatal error in plotmtv"}, */
/*      {FORK_FAILED,"\n no childprocess could be created"}, */
/*      {PLOT_RC,"\n warning: syntax error in resource file"}, */
/*      {0xFF,""}  end of list */ 
/*   };  */

/*
  ERRORHANDLER2(RECOVER,  FOR_TESTING , _LOOPS_C, 
               err_mess, "test", "west");
  ERRORHANDLER(MESSAGE, MESSAGE1 , _LOOPS_C, 
               err_mess, "test");
  ERRORHANDLER(RECOVER, TEST , _LOOPS_C, 
               err_mess, "test");
  ERRORHANDLER(RESET, TEST , _LOOPS_C, 
               err_mess, "test");
*/

#define SQR(a) ((a)*(a))

static int plotsetCnt;

static struct adressmodel {
  char             plot_set[STRING_SIZE]; /* name of plot_set */
  long               fpos;      /* file pos of add commands is recource file */          
  struct adressmodel *next;     /* pointer to next dataset*/ 
} *adress_ptr = NULL, *amark_ptr = NULL; 

typedef struct location {
  FILE  *fp; 
  int   current_lcolor, status;
  char  bin_filename[1024];
} locationStruct; 

static struct filelist {
  locationStruct  data;
  struct filelist *next;
} *fbegin_ptr = NULL, *fnew_ptr = NULL;

static struct filelist file_list_start={ {NULL,0x0,0x0,""},NULL};
static struct adressmodel adress_list_start={"",0x0,NULL};

static FILE *ControlScript=NULL;


static void adrSave(char buffer[],
                    int strpos,
                    int length,
                    long fpos)
{
  int mark;
  char plot_set[STRING_SIZE]="";

  for (mark = strpos; strpos < length; strpos++)
    plot_set[strpos-mark] = buffer[strpos];
  
  adress_ptr              = (struct adressmodel*)malloc (sizeof(struct adressmodel));
  adress_ptr->fpos        = fpos; 
  strcpy(adress_ptr->plot_set,plot_set);

  adress_ptr->next        = amark_ptr->next;
  amark_ptr->next         = adress_ptr;
}

static int search(char plot_set[])
{
  int not_found ;
  char filename[STRING_SIZE];
  not_found = 1 ;

  strcpy(filename, plotsetPreafix);         
  strcat(filename,plot_set);         

  if (fbegin_ptr->next != NULL)
    { 
      fnew_ptr = fbegin_ptr->next;
      while (fnew_ptr != NULL)
        {
          not_found = strcmp(fnew_ptr->data.bin_filename, filename); /* 0 if equal */
          if ( (not_found != 0) && (fnew_ptr->next != NULL) ) /* found */
            fnew_ptr = fnew_ptr->next;
          else if ( (not_found == 0) || (fnew_ptr->next == NULL) )
            break;
        }
    } 
   
  return not_found;
}
static char magicString[20]={0x0};
static int initFlag=0;
static char *identStr;
static char plotFileName[STRING_SIZE];
int plotInit(int    frameL,
             char * fileName, 
             int    encFlag)
{
  char plot_set[STRING_SIZE], line[STRING_SIZE];
  char tmpString[1024];
  int counter,magic;
  struct filelist *fdel_ptr;
  struct adressmodel *adel_ptr;
  if (initFlag==0){
    initFlag=1;
    identStr=(char*)malloc(strlen(fileName)+1);
    strcpy(identStr,fileName);
    srand(time (0));
    magic=rand();
    sprintf(magicString,"%d",magic);
    if (getenv("TMP")!=NULL){
      strcpy(plotsetPreafixTmp,getenv("TMP"));
    } else {
      strcpy(plotsetPreafixTmp,PLOTSET_TMP);
    }   

    strcpy(plotsetPreafix0,plotsetPreafixTmp);         
    strcat(plotsetPreafix0,PLOTSET_PRAEF0);         
    strcat(plotsetPreafix0, magicString);

    strcpy(plotsetPreafix1,plotsetPreafixTmp);         
    strcat(plotsetPreafix1,PLOTSET_PRAEF1);         
    strcat(plotsetPreafix1, magicString);

    strcpy(plotsetPreafix,plotsetPreafix0);         

    if (encFlag)
      strcpy(plotFileName,plotFileNameEnc);
    else 
      strcpy(plotFileName,plotFileNameDec);
  }


  if (frameL>=firstFrame)
    enable_plotmtv=1;
  else
    enable_plotmtv=0;
    
  if (enable_plotmtv)
    {      
      plotsetCnt=0;
      fbegin_ptr = &file_list_start;
      if (ControlScript!=0)
        fclose (ControlScript);   /* don't forget to close !! */
      if (( ControlScript = fopen(plotFileName, "r")) == NULL){
	strcpy(tmpString,plotFileName);
	strcpy(plotFileName,"../");
	strcat(plotFileName,tmpString);
        if (( ControlScript = fopen(plotFileName, "r")) == NULL) {	  
          enable_plotmtv = 0;
          /* ERRORHANDLER(MESSAGE,DISAB_MESS,_PLOTMTV_INTERF_C,err_mess,plotFileName); */
        }
      }
      if (ControlScript != NULL)  {          
          /*delete filelist */
          while (fbegin_ptr->next != NULL)
            {
              fdel_ptr = fbegin_ptr->next;
              fbegin_ptr->next = (fbegin_ptr->next)->next;
              free (fdel_ptr);
            }
          amark_ptr               = &adress_list_start;
          while (amark_ptr->next != NULL)
            {
              adel_ptr = amark_ptr->next;
              
              amark_ptr->next = (amark_ptr->next)->next;
              free (adel_ptr);
            }
          
          /*to do: copy control script to a tempor. file to ensure that the file positions are not changed by editing  */ 
          
          while (!feof(ControlScript)) 
            { 
              fscanf(ControlScript, "%1024[^\n]\n", line);
              for (counter = 0; counter < (int)strlen(line); counter++)
                {
                  if (line[counter] == '#') /* Die Kommandozeile ist ein Kommentar-> Abbruch */
                    break;
                  if (line[counter] == ':') /* Name des Plotsets */
                    {
                      counter++;
                      sscanf(&(line[counter]),"%1024[^# \f\n\r\t\v]",plot_set);
                      if (ftell(ControlScript) != -1L)
                        {                  
                          if (strlen(plot_set) < 2)
			    CommonExit(-1,"\nplotmtv error");
/*                             ERRORHANDLER(RESET,INIT_ERROR,_PLOTMTV_INTERF_C,err_mess," "); */
                          adrSave(plot_set, 0, strlen(plot_set), ftell(ControlScript)); 
                          break;
                        }
                      else if ( ftell(ControlScript) == -1L) {
/*                         ERRORHANDLER(MESSAGE,INIT_ERROR,_PLOTMTV_INTERF_C,err_mess," "); */
		      }
                    }
                } /* end of for */                    
            } /*  end of while */
          rewind(ControlScript);        /* statt fclose */
        }
    }
  return 0;
}

int plotSend(char           legend[],
             char           plot_set[],
             enum DATA_TYPE dtype,
             long           npts,
             const void     *dataPtr,
             const void     *dataPtr2)
{
  char    buffer[STRING_SIZE], line[STRING_SIZE],keyword[STRING_SIZE];
  int     found = FALSE, length,endFound;
  long    counter;
  double  *x, *y;   

  if (enable_plotmtv){
    if (npts > 0) {
      
      x = (double *)malloc(npts*sizeof(double));
      y = (double *)malloc(npts*sizeof(double));
      
      if (dtype==MTV_DOUBLE)
        {
          for (counter = 0; counter < npts; counter++)
            y[counter] = ((double*)dataPtr)[counter];  
        }
      
      else if (dtype==MTV_DOUBLE_SQA)
        {
          for (counter = 0; counter < npts; counter++)
            y[counter] = ((double*)dataPtr)[counter]*((double*)dataPtr)[counter];  
        }
      
      else if (dtype==MTV_FLOAT)
        {
          for (counter = 0; counter < npts; counter++)
            y[counter] = (double) (((float*)dataPtr)[counter]);  
        }
      else if (dtype==MTV_ABSFLOAT)
        {
          for (counter = 0; counter < npts; counter++)
            if (((float*)dataPtr)[counter]>=0)
              y[counter] = (double) (((float*)dataPtr)[counter]);  
            else
              y[counter] = (double) -(((float*)dataPtr)[counter]);  
        }
      else if (dtype==MTV_CPLXFLOAT)
        {
          for (counter = 0; counter < npts; counter++)
	    y[counter] = (double) sqrt(SQR(((float*)dataPtr)[counter])+
				       SQR(((float*)dataPtr2)[counter]));  
        }
      else if (dtype==MTV_SHORT)
        {
          for (counter = 0; counter < npts; counter++)
            y[counter] = (double) (((short*)dataPtr)[counter]);  
        }
      else if (dtype==MTV_INT)
        {              
          for (counter = 0; counter < npts; counter++)
            y[counter] = (double) (((int*)dataPtr)[counter]);                     
        } 
      else if (dtype==MTV_INT_SQA)
        {              
          for (counter = 0; counter < npts; counter++)
            y[counter] = (double) (((int*)dataPtr)[counter])*(double) (((int*)dataPtr)[counter]);                     
        } else {
	  CommonExit(-1,"\nplotmtv error");
          /* ERRORHANDLER(RESET,INV_DATAFORMAT,_PLOTMTV_INTERF_C,err_mess," "); */
        }
      for (counter = 0; counter < npts; counter++)
        x[counter] = counter ;  
      /* search adresslist for the plot_set */
      adress_ptr = amark_ptr->next; 
      found = FALSE;
      while (adress_ptr != NULL) /* last set */
        {
          if (strcmp(adress_ptr->plot_set, plot_set) == 0) 
            {
              fseek(ControlScript, adress_ptr->fpos, SEEK_SET);
              found = TRUE;
              break;
            }
          else          
            adress_ptr = adress_ptr->next;       
        }      
      if (found == TRUE)
        {                                  
          if (search(plot_set) != 0) 
            {
              /* das Plotset ist zum 1. Mal da -> Option w */
	      plotsetCnt++;
              fnew_ptr = (struct filelist *) malloc(sizeof(struct filelist));
              fnew_ptr->next = fbegin_ptr->next;
              fnew_ptr->data.current_lcolor = 1;  
              fbegin_ptr->next = fnew_ptr;

              strcpy(fnew_ptr->data.bin_filename, plotsetPreafix);         
              strcat(fnew_ptr->data.bin_filename, plot_set);
         
              /* open file  */
              if ((fnew_ptr->data.fp = fopen(fnew_ptr->data.bin_filename,"w")) == NULL)
                {
                  CommonExit(-1,"\nplotmtv error");
                  /* ERRORHANDLER(RESET,FILE_MISSING,_PLOTMTV_INTERF_C,err_mess, fnew_ptr->data.bin_filename); */
                }
              else
                fnew_ptr->data.status = ACTIV;
         
              /* Parameter-Kopf */
              fprintf(fnew_ptr->data.fp,"$ DATA=CURVE2D\n");
            }
          else  
            {
              /* open file for append  */
              if ((fnew_ptr->data.fp = fopen(fnew_ptr->data.bin_filename,"a")) == NULL)
                {
                  CommonExit(-1,"\nplotmtv error");
                  /* ERRORHANDLER(RESET,FILE_MISSING,_PLOTMTV_INTERF_C,err_mess, fnew_ptr->data.bin_filename); */
                }
              fnew_ptr->data.current_lcolor = fnew_ptr->data.current_lcolor + 1; 
            }
          /* weitere Parameter... */
          fprintf(fnew_ptr->data.fp,"%% linecolor = %d\n",fnew_ptr->data.current_lcolor);
          if (plotPaperPlot == 1) {
            fprintf(fnew_ptr->data.fp,"%% linetype = %d\n",(fnew_ptr->data.current_lcolor));
            fprintf(fnew_ptr->data.fp,"%% markertype = %d\n",(fnew_ptr->data.current_lcolor)+1);
          }
          fprintf(fnew_ptr->data.fp,"%% linelabel =  \" %s \" \n",legend);     
          fprintf(fnew_ptr->data.fp,"%% toplabel = \" %s %s fr%d gr%d\"\n",identStr,plot_set,framePlot,0);                 
	  endFound=0;
          while (!feof(ControlScript)) { 
            fscanf (ControlScript, "%1024[^\n]\n", line);
            sscanf (line,"%1024[^#]",buffer);
            sscanf (buffer,"%s",keyword);
            keyword[3] = '\000';
            if ((strcmp(keyword,"END") == 0)
                ||(strcmp(keyword,"EOF") == 0)
                ||(strcmp(keyword,"end") == 0)
                ||(strcmp(keyword,"End") == 0)) {
              endFound=1;
              break;
            }  else  {
              length = strlen(buffer);
              fprintf(fnew_ptr->data.fp,"%s\n",buffer);                 
            }
          }
	  /* if (endFound == 0) { */
          /*   ERRORHANDLER(MESSAGE,PLOT_RC,_PLOTMTV_INTERF_C,err_mess, ControlScript); */
	  /* } */
	    
          rewind(ControlScript);  
     
     
          /* write the binary data to plot file */
          fprintf(fnew_ptr->data.fp,"%% binary=True npts=%ld\n",npts);
          if (fwrite((char *)x, sizeof(double), npts, fnew_ptr->data.fp) != (size_t)npts)
            {
              CommonExit(-1,"\nplotmtv error");
              /* ERRORHANDLER(RESET,WRITE_BIN,_PLOTMTV_INTERF_C,err_mess, fnew_ptr->data.bin_filename); */
            }
          if (fwrite((char *)y, sizeof(double), npts, fnew_ptr->data.fp) != (size_t)npts)
            {
              CommonExit(-1,"\nplotmtv error");
              /* ERRORHANDLER(RESET,WRITE_BIN,_PLOTMTV_INTERF_C,err_mess, fnew_ptr->data.bin_filename); */
            }  
          fprintf(fnew_ptr->data.fp,"\n"); 
          fclose(fnew_ptr->data.fp);
     
        } /* found == TRUE */
      else
#if DEBUG_MTV
        CommonWarning("warning: the plot_set '%s' is not available in the resource file. Command 'plotPlot' was not executed. \n",
                plot_set); 
#endif 
      free (x);
      free (y);
    } 
  }
  return 0;
}

static pid_t   childProcessId=-1;
static int _switch=0;
void plotDisplay(int noWait)
{
  int     i;
  pid_t   termPid;
  int     termStatus;
  char *  argvP0[1024];
  int     argCount;
  int     errorFile;
  int     logFile;

  if (enable_plotmtv) {

    /* kill childProcessId for future extension */
    /* if (termPid == childProcessId) { */
    /*   strcpy (command, "kill -9 " ); */
    /*   sprintf(pidString,"%9d",childProcessId); */
    /*   strcat (command,pidString); */
    /*   printf("\n kill chld 0 command: %s \n",command); */
    /*   system (command);	 */
    /* }  */

    termPid=0;
    if (!noWait) {
      while ( (termPid!=childProcessId) ) {
        termPid = waitpid ((pid_t) -1, &termStatus,0); /* wait on termiation of any child */
      }
    }
    childProcessId=-1;
    /* call plotmtv --> one instance of plotmtv is created */
    if (fbegin_ptr != NULL) {
      if (fbegin_ptr->next != NULL) { 
        fnew_ptr = fbegin_ptr->next;
        /* first instance of plotmtv */
        i=0;
        argCount=0;
        argvP0[argCount++]="plotmtv";
        argvP0[argCount++]="-mult";
        argvP0[argCount++]="-bg";
        argvP0[argCount++]=BG_COLOR;
        argvP0[argCount++]=PRINTER_OPT;
        argvP0[argCount++]="-clobber";

        while (fnew_ptr != NULL ) {
          argvP0[argCount]= fnew_ptr->data.bin_filename;
          fnew_ptr = fnew_ptr->next;
          i++;
          argCount++;
        }
        argvP0[argCount++]="-geometry";
        switch (i)
          {
          case 1 :	     
            argvP0[argCount++]= WIN_GEOM_1 ; 
            break;
          case 2 :
            argvP0[argCount++]= WIN_GEOM_2; 
            break;
          case 3 :
            argvP0[argCount++]= WIN_GEOM_3; 
            break;
          case 4 :
            argvP0[argCount++]= WIN_GEOM_4 ; 
            break;
          case 5 :
          case 6 :
          case 7 :
          case 8 :
            argvP0[argCount++]= WIN_GEOM_5 ; 
            break;
          default:
            argvP0[argCount++]= WIN_GEOM_6 ; 
          }
        argvP0[argCount++]=NULL;
        if ( i>0  )   {
          childProcessId=fork();      
          /* first child */
          if (childProcessId== 0 ) {
            /******************** child **********************/
            /* redirect stdout and stderr befor exec plotmtv */
            logFile = (open (logFilename, O_RDWR | O_CREAT ,S_IWUSR | S_IRUSR )); 
            if (logFile != -1 ){
              dup2 (logFile, STDOUT_FILENO);
              (close (logFile));
            } else {
              /* ERRORHANDLER(MESSAGE, FORK_FAILED ,  _PLOTMTV_INTERF_C,  */
              /*              err_mess, " Logfile for plotmtv could not be opened "); */
            }
            errorFile = (open (errorFilename, O_RDWR | O_CREAT ,S_IWUSR | S_IRUSR )); 
            if (errorFile != -1 ){
              dup2 (errorFile, STDERR_FILENO);
              (close (errorFile));
            } else {
              /* ERRORHANDLER(MESSAGE, FORK_FAILED ,  _PLOTMTV_INTERF_C,  */
              /* 		  err_mess, " Error file for plotmtv could not be opened "); */
            }	      
            execvp("plotmtv",argvP0); /* this never returns if success */
            exit (-1); 
          } else if (childProcessId == -1) {
            /************************ parent *******************/	      
            /* ERRORHANDLER(MESSAGE, FORK_FAILED ,  _PLOTMTV_INTERF_C,  */
            /* 		err_mess, " fork failed "); */
          } 	    
          /* for the next run use the other praefix for the plotset filenames 
             else plotmtv would drop some files */
          sleep(1); /* give processor time to plotmtv */
          if (_switch==0){
            _switch=1; 
            strcpy(plotsetPreafix,plotsetPreafix1);         
          } else {
            _switch=0; 
            strcpy(plotsetPreafix,plotsetPreafix0);         
          }
        }	  
      }
    }
  }  
}

void plotDirect(char           *label,
                enum DATA_TYPE dtype,
                long           npts,
                void           *vector1,
                void           *vector2,
                void           *vector3,
                void           *vector4)
{
  int save_enable_plotmtv;
  int noWait=1; /* for plotDirect, plotDisplay shall not wait for dead of previous plot window */

  label = label;
  save_enable_plotmtv  = enable_plotmtv;
  enable_plotmtv=1;  
  plotInit(100000,"direct",0);
  if (dtype==MTV_CPLXFLOAT) {
    plotSend("vector1+2","direct1",dtype,npts,vector1,vector2);
    if (vector3 !=NULL)
      plotSend("vector3+4","direct1",dtype,npts,vector3,vector4);
  }
  else {
    plotSend("vector1","direct1",dtype,npts,vector1,NULL);
    if (vector2 !=NULL)
      plotSend("vector2","direct1",dtype,npts,vector2,NULL);
    if (vector3 !=NULL)
      plotSend("vector3","direct2",dtype,npts,vector3,NULL);
    if (vector4 !=NULL)
      plotSend("vector4","direct2",dtype,npts,vector4,NULL);
  }
  plotDisplay(noWait); 
  enable_plotmtv = save_enable_plotmtv;
}
