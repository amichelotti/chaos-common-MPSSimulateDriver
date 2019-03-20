/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    N568INTERFACE.C                                                      */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    September 2002   : Rel. 2.6 - Created                                */
/*                                                                         */
/***************************************************************************/
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <time.h>				  /* Rel. 2.3 */
#ifdef UNIX
#include  <errno.h>				  /* For Linux */
#endif
#include  "HSCAENETLib.h"							  
#include  "CAENHVWrapper.h"
#include  "n568interface.h"

#define   MAKE_CODE(ch,cod)      (((ch)<<8) | (cod))

#define   MAX_NR_OF_CARDS     10	      /* It seems more than enough */

/*
  The Caenet Codes
*/
#define   IDENT                                  0x0
#define   READ_ALL_PARAM_ALL_CH_AND_OFFSET       0x1
#define   READ_OFFSET                            0x2
#define   READ_ALL_PARAM_CH                      0x3
#define   READ_MUX_EN_STAT_AND_LAST_CH           0x4

#define   SET_FINE_GAIN 	                 0x10
#define   SET_COARSE_GAIN 	                 0x11
#define   SET_POLE_ZERO_ADJ	                 0x12
#define   SET_SHAPE     	                 0x13
#define   SET_OUT_POL    	                 0x14
#define   SET_OUT_CONF  	                 0x15
#define   SET_OFFSET    	                 0x16
#define   DIS_MUX_OUT   	                 0x20
#define   EN_MUX_OUT    	                 0x21   

#define   PAR_FINE_GAIN 	                 1
#define   PAR_COARSE_GAIN 	                 2
#define   PAR_POLE_ZERO_ADJ	                 3
#define   PAR_SHAPE     	                 4
#define   PAR_OUT_POL    	                 5
#define   PAR_OUT_CONF  	                 6
#define   PAR_OFFSET    	                 7
#define   PAR_MUX_OUT       	                 8
#define   PAR_MUX_CHAN                           9
#define   PAR_NON			         10

#define   NOT_DEV			        -1
#define   N568_DEV			         0
#define   N568B_DEV			         1


/*
  The following structure contains all the useful information about
  the parameters of a channel
*/
struct n568ch
{
	ushort	fineGain;
	ushort  poleZero;
	ushort  status;
};

/*
  The following structure contains all the useful information about
  the parameters of all the channels + the common offset
*/
struct n568allch
{
	struct n568ch AllParCh[16];
	ushort        offset;
};

/*
  The following structure is necessary in order to optimize access to 
  N568 via CAENET
*/
struct n568AllParamsTag
{
	struct n568allch  AllParAllCh;
	time_t			  parTime;
};


/*
  Globals
  Rel. 2.10 - Added const to constant global variables.
            - Deleted the code and cratenum global variables
			  due to interfernce between thread.
            - For WhichDevice, the interferences between threads are avoided 
			  because the same values are written in the same place.
			- n568AllParams protected with a critical section.
*/
/* static int                      cratenum, code; */

static int                      WhichDevice[MAX_NR_OF_CARDS][100];	 /* Are N568 or N568B ?  */

static struct n568AllParamsTag  n568AllParams[MAX_NR_OF_CARDS][100];

static int                      errors[MAX_NR_OF_CARDS][100];	  	 /* The last error code */

/* The list of Channels' Parameters for N568B */
static const char               *ChPar[] = {
	"FineGain", 
	"CoarGain", 
	"PoleZAdj", 
	"Shape", 
	"OutPol",	 
	"OutConf"
};

/* The list of Board's Parameters for N568B */
static const char               *BdPar[] = {
	"Offset",														
	"MuxOut",   						/* Only on N568B */
/*	"MuxChan"	Rel. 2.7				// Only on N568B */
	"LastCh"     						/* Only on N568B */
};

/* For SysProp routines */

static const struct sysPropTag 
{
	unsigned num;
	char     *name[7];
	unsigned mode[7];
	unsigned type[7];
} sysProp = { 
		3, 
	{	
		"ModelName", "SwRelease", "CnetCrNum"
	}, 
	{	
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY
	}, 
	{
		SYSPROP_TYPE_STR, SYSPROP_TYPE_STR, SYSPROP_TYPE_UINT2
	} 
}; 


/*

  Caenet_Comm
  Rel. 2.01 - Added busy support
  Rel. 2.10 - Added the code and cratenum parameters instead of global
              variables to correct interference problem in multithraed

*/
static int caenet_comm(int dev, int cratenum, int code, const void *s, int w, void *d)
{
	int resp;

	do
	{        
		resp = HSCAENETComm(dev, code, cratenum, s, w, d);
		if( resp == -256 )
			Sleep(200L);
		else
			break;
	}
	while( 1 );

	return resp;
}

/* IsTimeElapsed */

static int IsTimeElapsed(int devHandle, int crNum)
{
	time_t now;

	time(&now);

	return !( n568AllParams[devHandle][crNum].parTime + 1 > now );
}

extern void locksem();		/* Rel. 2.11 */
extern void unlocksem();	/* Rel. 2.11 */

/* UpdateValue */

static void UpdateValue(int devHandle, int crNum, void *p)
{
	time_t now;

/*	EnterCriticalSection(&cs);	 Rel. 2.10 */
	locksem();			/* Rel. 2.11 */
	
	time(&now);

	memcpy(&n568AllParams[devHandle][crNum].AllParAllCh, p, 6*16+2);
	n568AllParams[devHandle][crNum].parTime = now;

/*	LeaveCriticalSection(&cs);	 Rel. 2.10 */
	unlocksem();			/* Rel. 2.11 */
}

/* GetValue */

static void GetValue(int devHandle, int crNum, void *p)
{
	memcpy(p, &n568AllParams[devHandle][crNum].AllParAllCh, 6*16+2);
}

/* N568GetChName */

CAENHVRESULT N568GetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							const ushort *ChList, 
							char (*ChNameList)[MAX_CH_NAME])
{
	int                 i;
	CAENHVRESULT        res = CAENHV_OK;

	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else 	
		for( i = 0 ; i < ChNum ; i++ )
		{
			if( ChList[i] >= 16 )
			{
				errors[devHandle][crNum] = -254;
				res = CAENHV_OUTOFRANGE;
				break;
			}
			else
			{
				sprintf(ChNameList[i], "CH0%d", ChList[i]);
			}
		}

	return res;
}

/* N568SetChName */

CAENHVRESULT N568SetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName)
{
	CAENHVRESULT res;

	errors[devHandle][crNum] = -255;
	res = CAENHV_WRITEERR;

	return res;
}

/* N568GetChParamInfo */

CAENHVRESULT N568GetChParamInfo(int devHandle, int crNum, ushort slot, ushort Ch, 
								 char **ParNameList)
{
	int				i;
	CAENHVRESULT	res;

	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( Ch >= 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		char (*p)[MAX_PARAM_NAME] = malloc((6+1)*MAX_PARAM_NAME);

		for( i = 0 ; i < 6 ; i++ )
			strcpy(p[i], ChPar[i]);

		strcpy(p[i],"");
		*ParNameList = (char *)p;
		
		res = CAENHV_OK;
	}

	return res;
}

/* N568GetChParamProp */

CAENHVRESULT N568GetChParamProp(int devHandle, int crNum, ushort slot, ushort Ch, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	CAENHVRESULT  res = CAENHV_OK;
	
	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( Ch >= 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else   
	{
		if( !strcmp( ParName, "FineGain" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 255.0; 
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_COUNT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "CoarGain" ) )
		{
			if( !strcmp(PropName,"Type") )	
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 7.0; 
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_COUNT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "PoleZAdj" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 255.0; 
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_COUNT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "Shape" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 3.0; 
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_COUNT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "OutPol" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_ONOFF;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Onstate") )
				strcpy((char *)retval, "Negative");
			else if( !strcmp(PropName,"Offstate") )				
				strcpy((char *)retval, "Positive");
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "OutConf" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_ONOFF;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Onstate") )
				strcpy((char *)retval, "Inverted");
			else if( !strcmp(PropName,"Offstate") )				
				strcpy((char *)retval, "Direct");
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else
			res = CAENHV_PARAMNOTFOUND;
	}

	return res;
}

/***------------------------------------------------------------------------

  N568GetChParam
	
  Rel 2.10 - Modified for the elimination of the cratenum and 
		     code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
							 ushort ChNum, const ushort *ChList, void *ParValList)
{
	int                 i, par, code;
	char                cnetbuff[MAX_LENGTH_FIFO];
	CAENHVRESULT        res = CAENHV_OK;

	if( !strcmp(ParName, "FineGain") )
		par = PAR_FINE_GAIN;
	else if( !strcmp(ParName, "CoarGain") )
		par = PAR_COARSE_GAIN;
	else if( !strcmp(ParName, "PoleZAdj") )
		par = PAR_POLE_ZERO_ADJ;
	else if( !strcmp(ParName, "Shape") )
		par = PAR_SHAPE;
	else if( !strcmp(ParName, "OutPol") )
		par = PAR_OUT_POL;
	else if( !strcmp(ParName, "OutConf") )
		par = PAR_OUT_CONF;
	else
		par = PAR_NON;

	if( par == PAR_NON )
		res = CAENHV_PARAMNOTFOUND;
	else if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		struct n568allch  AllParAllCh;

		if( IsTimeElapsed(devHandle, crNum) )
		{
			code = READ_ALL_PARAM_ALL_CH_AND_OFFSET;

			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
				
			if( errors[devHandle][crNum] == TUTTOK )
			{
				memcpy(&AllParAllCh, cnetbuff, 6*16+2);
				UpdateValue(devHandle, crNum, &AllParAllCh);	
			}
			else
				res = CAENHV_READERR;
		} /* End if TimeElapsed */
		else
			GetValue(devHandle, crNum, &AllParAllCh);

		if( res == CAENHV_OK )
		{
			for( i = 0 ; i < ChNum ; i++ )
			{
				if( ChList[i] >= 16 )
				{
					errors[devHandle][crNum] = -254;
					res = CAENHV_OUTOFRANGE;
					break;
				}
				else
				{
					switch( par )									
					{
						case PAR_FINE_GAIN:
		 		  			*((float *)ParValList + i) = 
							  (float)AllParAllCh.AllParCh[ChList[i]].fineGain;
							break;
				
						case PAR_COARSE_GAIN:
		 		  			*((float *)ParValList + i) = 
							  (float)(AllParAllCh.AllParCh[ChList[i]].status&0x7);
							break;
				
						case PAR_POLE_ZERO_ADJ:
		 		  			*((float *)ParValList + i) = 
							  (float)AllParAllCh.AllParCh[ChList[i]].poleZero;
							break;
				
						case PAR_SHAPE:
		 		  			*((float *)ParValList + i) = 
							  (float)((AllParAllCh.AllParCh[ChList[i]].status&(3<<3)) >> 3);
							break;
				
						case PAR_OUT_POL:
							*((unsigned long *)ParValList + i) = 
							 ((AllParAllCh.AllParCh[ChList[i]].status&(1<<6)) >> 6);
							break;

						case PAR_OUT_CONF:
							*((unsigned long *)ParValList + i) = 
							 ((AllParAllCh.AllParCh[ChList[i]].status&(1<<5)) >> 5);
							break;
					} /* End switch */
				} /* End if-else */
			} /* End for */
		} /* End if( res) */ 
	} /* End if-else (par) */
 
	return res;
}

/***------------------------------------------------------------------------

  N568SetChParam

  Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568SetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
						     ushort ChNum, const ushort *ChList, void *ParValue)
{
	ushort              cnet_buff;
	int                 code, i, par, dataw = sizeof(cnet_buff);
	CAENHVRESULT        res = CAENHV_OK;

	if( !strcmp(ParName, "FineGain") )
		par = PAR_FINE_GAIN;
	else if( !strcmp(ParName, "CoarGain") )
		par = PAR_COARSE_GAIN;
	else if( !strcmp(ParName, "PoleZAdj") )
		par = PAR_POLE_ZERO_ADJ;
	else if( !strcmp(ParName, "Shape") )
		par = PAR_SHAPE;
	else if( !strcmp(ParName, "OutPol") )
		par = PAR_OUT_POL;
	else if( !strcmp(ParName, "OutConf") )
		par = PAR_OUT_CONF;
	else
		par = PAR_NON;

	if( par == PAR_NON )
		res = CAENHV_PARAMNOTFOUND;
	else if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		for( i = 0 ; i < ChNum ; i++ )
		{
			switch( par )
			{							
				case PAR_FINE_GAIN:
					code = MAKE_CODE(ChList[i], SET_FINE_GAIN);
					cnet_buff = (ushort)(*(float *)ParValue);
					break;
				case PAR_COARSE_GAIN:
					code = MAKE_CODE(ChList[i], SET_COARSE_GAIN);
					cnet_buff = (ushort)(*(float *)ParValue);
					break;
				case PAR_POLE_ZERO_ADJ:
					code = MAKE_CODE(ChList[i], SET_POLE_ZERO_ADJ);
					cnet_buff = (ushort)*(float *)ParValue;
					break;
				case PAR_SHAPE:
					code = MAKE_CODE(ChList[i], SET_SHAPE);
					cnet_buff = (ushort)*(float *)ParValue;
					break;
				case PAR_OUT_POL:
					code = MAKE_CODE(ChList[i], SET_OUT_POL);
					cnet_buff = (ushort)*(unsigned *)ParValue;
					cnet_buff &= 1;
					break;
				case PAR_OUT_CONF:
					code = MAKE_CODE(ChList[i], SET_OUT_CONF);
					cnet_buff = (ushort)*(unsigned *)ParValue;
					cnet_buff &= 1;
					break;
			}

			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, &cnet_buff, dataw, NULL);
			if( errors[devHandle][crNum] != TUTTOK )
			{
				res = CAENHV_WRITEERR;
				break;
			}
			Sleep(200L);
		} /* End for */
	} /* End if-else (par) */

	return res;
}

/***------------------------------------------------------------------------

  N568TestBdPresence

  Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.


    --------------------------------------------------------------------***/
CAENHVRESULT N568TestBdPresence(int devHandle, int crNum, ushort slot, char *Model, 
								 char *Description, ushort *SerNum, 
								 uchar *FmwRelMin, uchar *FmwRelMax)
{
	char         tempbuff[80];
	CAENHVRESULT res = CAENHV_OK;
	int code;

	if( slot >= 1 )
	{													
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		if( WhichDevice[devHandle][crNum] == N568_DEV ) 
			strcpy(Model, "N568");
		else
			strcpy(Model, "N568B");

		strcpy(Description, "16 Channel Spectroscopy Amplifier");

		*SerNum = 0;

		code    = IDENT;
		memset(tempbuff, '\0', sizeof(tempbuff));
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
		if( errors[devHandle][crNum] == TUTTOK )
		{ 
			*FmwRelMax = tempbuff[26]-'0';
			*FmwRelMin = tempbuff[30]-'0';
		}
		else
			res = CAENHV_READERR;
	}

	return res;
}

/***------------------------------------------------------------------------

  N568GetBdParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetBdParamInfo(int devHandle, int crNum, ushort slot, 
								 char **ParNameList)
{		
	int				i;
	CAENHVRESULT	res;

	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		int  firstPar, numPar;
		char (*p)[MAX_PARAM_NAME];

		if( WhichDevice[devHandle][crNum] == N568_DEV )
			firstPar = 0, numPar = 1;
		else
			firstPar = 0, numPar = 3;

		p = malloc((numPar+1)*MAX_PARAM_NAME);

		for( i = firstPar ; i < firstPar + numPar ; i++ )
			strcpy(p[i-firstPar], BdPar[i]);

		strcpy(p[i-firstPar],"");
		*ParNameList = (char *)p;
		
		res = CAENHV_OK;
	}

	return res;
}

/***------------------------------------------------------------------------

  N568GetBdParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetBdParamProp(int devHandle, int crNum, ushort slot, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	CAENHVRESULT  res = CAENHV_OK;
	
	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else   
	{
		if( !strcmp( ParName, "Offset" ) )
		{
			if( !strcmp(PropName,"Type") )										
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 255.0; 
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_COUNT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if(    !strcmp( ParName, "MuxOut" ) 
			     && WhichDevice[devHandle][crNum] == N568B_DEV )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_ONOFF;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Onstate") )
				strcpy((char *)retval, "Enable");
			else if( !strcmp(PropName,"Offstate") )				
				strcpy((char *)retval, "Disable");
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
/* Rel. 2.7	else if(    !strcmp( ParName, "MuxChan" ) */
		else if(    !strcmp( ParName, "LastCh" ) 
			     && WhichDevice[devHandle][crNum] == N568B_DEV )
		{
			if( !strcmp(PropName,"Type") )										
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
/* Rel. 2.7	*(ulong *)retval = PARAM_MODE_RDWR; */
				*(ulong *)retval = PARAM_MODE_RDONLY;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 15.0; 
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_COUNT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else
			res = CAENHV_PARAMNOTFOUND;
	}

	return res;
}

/***------------------------------------------------------------------------

  N568GetBdParam

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValList)
{
	int                 par, code;
	char                cnetbuff[MAX_LENGTH_FIFO];
	CAENHVRESULT        res = CAENHV_OK;

	if( !strcmp(ParName, "Offset") )
		par = PAR_OFFSET;
	else if(    !strcmp(ParName, "MuxOut") 
		     && WhichDevice[devHandle][crNum] == N568B_DEV )
		par = PAR_MUX_OUT;
/* Rel. 2.7	else if(    !strcmp(ParName, "MuxChan") */
	else if(    !strcmp(ParName, "LastCh")
		     && WhichDevice[devHandle][crNum] == N568B_DEV )
		par = PAR_MUX_CHAN;
	else
		par = PAR_NON;

	if( par == PAR_NON )
		res = CAENHV_PARAMNOTFOUND;
	else if( slotNum != 1 && *slotList != 0 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		struct n568allch  AllParAllCh;

		switch( par )									
		{
			case PAR_OFFSET:
				if( IsTimeElapsed(devHandle, crNum) )
				{
					code = READ_ALL_PARAM_ALL_CH_AND_OFFSET;

					errors[devHandle][crNum] = 
						caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
				
					if( errors[devHandle][crNum] == TUTTOK )
					{
						memcpy(&AllParAllCh, cnetbuff, 6*16+2);
						UpdateValue(devHandle, crNum, &AllParAllCh);	
					}
					else
						res = CAENHV_READERR;
				} /* End if TimeElapsed */
				else
					GetValue(devHandle, crNum, &AllParAllCh);

	  			*(float *)ParValList = (float)AllParAllCh.offset;
				break;
				
			case PAR_MUX_OUT:							/* Not optimized */
				code = READ_MUX_EN_STAT_AND_LAST_CH;

				errors[devHandle][crNum] = 
					caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);				
				if( errors[devHandle][crNum] == TUTTOK )
/* Rel. 2.7 Brackets !	*(unsigned long *)ParValList = (cnetbuff[0]&(1<<7) >> 7); */
					*(unsigned long *)ParValList = ( cnetbuff[0]&(1<<7) ) >> 7;
				else
					res = CAENHV_READERR;
				break;

			case PAR_MUX_CHAN:
				code = READ_MUX_EN_STAT_AND_LAST_CH;

				errors[devHandle][crNum] = 
					caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);				
				if( errors[devHandle][crNum] == TUTTOK )
					*(float *)ParValList = (float)(cnetbuff[0]&0xf);
				else
					res = CAENHV_READERR;
				break;
		} /* End switch */
	} /* End if-else (par) */
 
	return res;
}

/***------------------------------------------------------------------------

  N568SetBdParam

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568SetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValue)
{
	ushort              cnet_buff;
	char                cnetbuff[MAX_LENGTH_FIFO];
	int                 par, dataw = sizeof(cnet_buff), goAhead = 1, code;
	CAENHVRESULT        res = CAENHV_OK;

	if( !strcmp(ParName, "Offset") )
		par = PAR_OFFSET;
	else if(    !strcmp(ParName, "MuxOut") 
		     && WhichDevice[devHandle][crNum] == N568B_DEV )
		par = PAR_MUX_OUT;
/*
	Rel. 2.7

	else if( !strcmp(ParName, "MuxChan") 
		&& WhichDevice[devHandle][crNum] == N568B_DEV )
		par = PAR_MUX_CHAN;
*/
	else
		par = PAR_NON;

	if( par == PAR_NON )
		res = CAENHV_PARAMNOTFOUND;
	else if( slotNum != 1 && *slotList != 0 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		switch( par )
		{							
			case PAR_OFFSET:
				code = SET_OFFSET;
				cnet_buff = (ushort)(*(float *)ParValue);
				break;
			case PAR_MUX_OUT:
/* Rel. 2.7 - Remember the & !! */
				{
					ulong temp = (ulong)*(ulong *)ParValue;

					temp &= 1;
					if( temp == 1 )
						code = EN_MUX_OUT;
					else
						code = DIS_MUX_OUT;

					dataw = 0;
				}
				break;
/*
	Rel. 2.7
			case PAR_MUX_CHAN:
				cnet_buff = (ushort)*(float *)ParValue;
				code = MAKE_CODE(cnet_buff, READ_ALL_PARAM_CH);
				break;
*/
		}

		errors[devHandle][crNum] = 
			caenet_comm(devHandle, crNum, code, &cnet_buff, dataw, cnetbuff);
		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	} /* End if-else (par) */

	return res;
}

/***------------------------------------------------------------------------

  N568GetSysComp

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetSysComp(int devHandle, int crNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList)
{
	CAENHVRESULT  res = CAENHV_OK;

	ushort *app1;
	ulong  *app2;

	*NrOfCr = 1;
	*CrNrOfSlList = malloc(sizeof(ushort));
	*SlotChList   = malloc(10*sizeof(ulong));

	app1 = *CrNrOfSlList;
	app2 = *SlotChList;

	*app1 = 1;

	app2[0] = ( 1 << 0x10 ) | 16;

	return res;
}

/***------------------------------------------------------------------------

  N568GetCrateMap

  Rel 2.10 - Modified for the elimination of the cratenum and 
		     code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetCrateMap(int devHandle, int crNum, ushort *NrOfSlot, 
							  char **ModelList, char **DescriptionList, 
							  ushort **SerNumList, uchar **FmwRelMinList, 
							  uchar **FmwRelMaxList)
{
	CAENHVRESULT    res = CAENHV_OK;
	char            tempbuff[80];
	char			Desc[80] = "16 Channel Spectroscopy Amplifier", 
		            *app1, *app2;
	ushort			*app3, desclen = 0;
	int             code;

	*ModelList       = malloc(10);			
	*DescriptionList = malloc((strlen(Desc)+1));
	*SerNumList      = malloc(sizeof(ushort));
	*FmwRelMinList   = malloc(sizeof(uchar));
	*FmwRelMaxList   = malloc(sizeof(uchar));
	*NrOfSlot = 1;

	app1 = *FmwRelMinList, app2 = *FmwRelMaxList, app3 = *SerNumList;

	if( WhichDevice[devHandle][crNum] == N568_DEV )
		strcpy(*ModelList, "N568"); 
	else
		strcpy(*ModelList, "N568B");
	
	memcpy(*DescriptionList+desclen, Desc, strlen(Desc)+1);

	app3[0]  = 0;

	code    = IDENT;
	memset(tempbuff, '\0', sizeof(tempbuff));
	errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
	if( errors[devHandle][crNum] == TUTTOK )
	{ 
		app2[0] = tempbuff[26]-'0';			/* FmwRelMax */
		app1[0] = tempbuff[30]-'0';			/* FmwRelMin */
	}
	else
		res = CAENHV_READERR;

	return res;
}

/***------------------------------------------------------------------------

  N568GetExecCommList

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetExecCommList(int devHandle, int crNum, ushort *NumComm, 
								  char **CommNameList)
{
	return CAENHV_EXECNOTFOUND;
}

/***------------------------------------------------------------------------

  N568ExecComm

    --------------------------------------------------------------------***/
CAENHVRESULT N568ExecComm(int devHandle, int crNum, const char *CommName)
{
	return CAENHV_EXECNOTFOUND;;
}

/***------------------------------------------------------------------------

  N568GetSysPropList

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetSysPropList(int devHandle, int crNum, ushort *NumProp, 
								 char **PropNameList)
{
	ushort prop		  = sysProp.num;
	int    i, proplen = 0;

	*PropNameList     = malloc(prop*80);
	*NumProp          = prop;

	for( i = 0 ; i < prop ; i++ )
	{
		memcpy(*PropNameList + proplen, sysProp.name[i], strlen(sysProp.name[i])+1);
		proplen += strlen(sysProp.name[i])+1;
	}

	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N568GetSysPropInfo

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetSysPropInfo(int devHandle, int crNum, const char *PropName, 
								 unsigned *PropMode, unsigned *PropType)
{
	unsigned     i;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	for( i = 0 ; i < sysProp.num ; i++ )
		if( !strcmp(PropName, sysProp.name[i]) )
		{
			*PropMode = sysProp.mode[i];
			*PropType = sysProp.type[i];
			res = CAENHV_OK;
			break;
		}

	return res;
}

/***------------------------------------------------------------------------

  N568GetSysProp

  Rel 2.10 - Modified for the elimination of the cratenum and 
		     code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568GetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Result)
{
	int          i, code;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		    /* ModelName */
	{
		if( WhichDevice[devHandle][crNum] == N568_DEV )
			strcpy((char *)Result, "N568");
		else
			strcpy((char *)Result, "N568B");
		res = CAENHV_OK;
	}
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
	{
		char    syident[80], tempbuff[80];

		code    = IDENT;
		memset(tempbuff, '\0', sizeof(tempbuff));
		errors[devHandle][crNum] = 
			caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
		if( errors[devHandle][crNum] == TUTTOK )
		{ 
			for( i = 26 ; tempbuff[i] != '\0' ; i+=2 )
				syident[i/2-13] = tempbuff[i];
			syident[i/2-13] = '\0';
			strcpy((char *)Result, syident);

			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[2]) )	  /* CnetCrNum */
	{
		*(unsigned short *)Result = crNum;
		res = CAENHV_OK;
	}

	return res;
}

/***------------------------------------------------------------------------

  N568SetSysProp

  Rel 2.10 - Modified for the elimination of the cratenum and 
		     code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N568SetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Set)
{
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		  /* ModelName */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[2]) )	  /* CnetCrNum */
		res = CAENHV_NOTSETPROP;

	return res;
}

/***------------------------------------------------------------------------

  N568Init
  This is one of the two possible entry points into the routines of this file: 
  it is explicitely called when the user calls CAENHVInitSystem()
  See also N568BInit

    --------------------------------------------------------------------***/
int N568Init(int devHandle, int crNum)
{
	n568AllParams[devHandle][crNum].parTime = 0;

	WhichDevice[devHandle][crNum] = N568_DEV;
	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N568BInit
  This is one of the two possible entry points into the routines of this file: 
  it is explicitely called when the user calls CAENHVInitSystem()
  See also N568Init

    --------------------------------------------------------------------***/
int N568BInit(int devHandle, int crNum)
{
	n568AllParams[devHandle][crNum].parTime = 0;

	WhichDevice[devHandle][crNum] = N568B_DEV;
	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N568End

    --------------------------------------------------------------------***/
void N568End(int devHandle, int crNum)
{
	WhichDevice[devHandle][crNum] = NOT_DEV;  /* Prob. not necessary ... */
}

/***------------------------------------------------------------------------

  N568GetError

    --------------------------------------------------------------------***/
char *N568GetError(int devHandle, int crNum)
{

#define LIB_NAME "a303lib.dll"

#define TUTTOK             0
#define E_OS               1
#define E_LESS_DATA        2
#define E_TIMEOUT          3
#define E_NO_A303          4
#define E_A303_BUSY        5

	int         resp = errors[devHandle][crNum];
	static char buf[80];

	if( resp == E_OS )
#ifdef UNIX                                       /* Rel. 2.0 - Linux */
		strcpy(buf, strerror(errno));
#else
		sprintf(buf, "Operating System Error: %d", GetLastError());
#endif
	else
		switch( resp )
		{
			case E_LESS_DATA:
				strcpy(buf, "CaeNet Error: Less data received" );
				break;
			case E_TIMEOUT:
				strcpy(buf, "CaeNet Error: Timeout expired" );
				break;
			case E_NO_A303:
				strcpy(buf, "CaeNet Error: A303 not found" );
				break;
			case E_A303_BUSY:
				sprintf(buf, "Another instance of %s already loaded", LIB_NAME );
				break;
			default:
				sprintf(buf, "CaeNet Error: %d", resp );
				break;
		}

	return buf;
}

