
#ifndef _H_chandefs
#define _H_chandefs


#ifdef BIGDECODER
enum
{
    /*
     * channels for 5.1 main profile configuration 
     * (modify for any desired decoder configuration)
     */
    FChans	= 15,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 18,	/* side channels: */
    BChans	= 15,	/* back channels: left surround, right surround */
    BCenter	= 1,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 2,	/* scratch space for parsing unused channels */  
    
    Chans	= FChans + SChans + BChans + LChans + XChans
};
#else
enum
{
    /*
     * channels for 5.1 main profile configuration 
     * (modify for any desired decoder configuration)
     */
    FChans	= 3,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 2,	/* side channels: */
    BChans	= 2,	/* back channels: left surround, right surround */
    BCenter	= 0,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 2,	/* scratch space for parsing unused channels */  
    
    Chans	= FChans + SChans + BChans + LChans + XChans
};
#endif


#endif

