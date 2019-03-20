/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    N470INTERFACE.C                                        		       */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    May      2002    : Rel. 2.2 - Created                                */
/*    May      2002    : Rel. 2.3 - Optimized the N470/N570 access         */
/*                                  via CAENET                             */
/*    June     2002    : Rel. 2.5 - Corrected a bug in SetChParam when the */
/*                                  param is "Pw", the PS is N470 or N570  */
/*                                  and the card is an A303                */
/*                                                                         */
/***************************************************************************/
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <time.h>								  /* Rel. 2.3 */
#ifdef UNIX
#include  <errno.h>								  /* For Linux */
#endif
#include  "HSCAENETLib.h"							  
#include  "CAENHVWrapper.h"
#include  "n470interface.h"

#define   MAKE_CODE(ch,cod)      (((ch)<<8) | (cod))

#define   MAX_NR_OF_CARDS     10			      /* It seems more than enough */

/*
  The Caenet Codes
*/
#define   IDENT                                  0x0
#define   READ_CHS_MON                           0x1
#define   READ_ALL_PARAM_CH                      0x2

#define   SET_V0SET 	                         0x3
#define   SET_I0SET 	                         0x4
#define   SET_V1SET 	                         0x5
#define   SET_I1SET 	                         0x6
#define   SET_TRIP    	                         0x7
#define   SET_RUP    	                         0x8
#define   SET_RDWN    	                         0x9
#define   SET_CH_ON    	                         0xa   
#define   SET_CH_OFF   	                         0xb   
#define   KILL_CH   	                         0xc   
#define   CLEAR_ALARM  	                         0xd
#define   KEYB_EN   	                         0xe
#define   KEYB_DIS   	                         0xf
#define   TTL_LEVEL 	                         0x10
#define   NIM_LEVEL 	                         0x11

#define   PAR_V0S	  						1
#define   PAR_I0S							2
#define   PAR_V1S							3
#define   PAR_I1S							4
#define   PAR_RUP							5
#define   PAR_RDW							6
#define   PAR_TRP							7
#define   PAR_PW							9
#define   PAR_VMN						   10
#define   PAR_IMN						   11
#define   PAR_STS						   12
#define   PAR_VMX						   13
#define   PAR_NON						   14

#define   NOT_DEV							-1
#define   N470_DEV							 0
#define   N470A_DEV							 1
#define   N570_DEV                           2


/*
  The following structure contains all the useful information about
  the settings and monitorings of a channel
*/
struct n470ch
{
	ushort	status;
	ushort  vmon;
	ushort  imon;
	ushort  v0set;
	ushort  i0set;
	ushort  v1set;
	ushort  i1set;
	ushort  trip;
	ushort  rup;
	ushort  rdwn;
	ushort  hvmax;
};

/*
  The following structure contains all the usefule information about
  the monitorings of a channel
*/
struct n470chMon
{
	ushort  vmon;
	ushort  imon;
	ushort  hvmax;
	ushort	status;
};

/*
  Rel. 2.3
  The following structure is necessary in order to optimize access to 
  N470/N570 via CAENET
*/
struct n470AllParamsTag
{
	struct n470ch     AllParCh[4];
	struct n470chMon  MonParCh[4];
	time_t			  monTime;
	time_t			  setTime[4];
};


/*
  Globals
  Rel. 2.10 - Added const to constant global variables.
            - Deleted the code and cratenum global variables
			  due to interfernce between thread.
            - For WhichDevice, the interferences between threads are avoided 
			  because the same values are written in the same place.
			- n470AllParams protected with a critical section.
*/
/*static int                      cratenum, code; */

static int                      WhichDevice[MAX_NR_OF_CARDS][100];	 /* Are N470 or N570 ?  */

static struct n470AllParamsTag  n470AllParams[MAX_NR_OF_CARDS][100]; /* Rel. 2.3 */

static int                      errors[MAX_NR_OF_CARDS][100];	  	 /* The last error code */

/* The list of Channels' Parameters for N470/N570 */
static const char               *ChPar[] = {
	"V0Set", 
	"I0Set", 
	"V1Set", 
	"I1Set", 
	"RUp",	 
	"RDwn",	 
	"Trip",	 
	"Pw",
	"VMon",  
	"IMon",
	"HVMax",
	"Status"
};

/* For ExecComm routines */
static const char               *execComm[] = { "Kill", "ClearAlarm" };

/* For SysProp routines */
static const struct sysPropTag 
{
	unsigned num;
	char     *name[7];
	unsigned mode[7];
	unsigned type[7];
} sysProp = { 
		6, 
	{	
		"ModelName", "SwRelease", "CnetCrNum", "LockKeyboard", 
		"FrontPanStat", "LevelSelect"
	}, 
	{	
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, 
		SYSPROP_MODE_WRONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR
	}, 
	{
		SYSPROP_TYPE_STR, SYSPROP_TYPE_STR, SYSPROP_TYPE_UINT2, 
		SYSPROP_TYPE_BOOLEAN, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_BOOLEAN
	} 
}; 

/***------------------------------------------------------------------------

  Caenet_Comm
  Rel. 2.01 - Added busy support
  Rel. 2.10 - Added the code and cratenum parameters instead of global
              variables to correct interference problem in multithraed

    --------------------------------------------------------------------***/
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

/***------------------------------------------------------------------------

  IsTimeElapsedForCh
  Rel. 2.3

    --------------------------------------------------------------------***/
static int IsTimeElapsedForCh(int devHandle, int crNum, int ch)
{
	time_t now;

	time(&now);

	if( ch == -1 )								/* MonParam */
		return !( n470AllParams[devHandle][crNum].monTime + 1 > now );
	else
		return !( n470AllParams[devHandle][crNum].setTime[ch] + 1 > now );
}

extern void locksem();		/* Rel. 2.11 */
extern void unlocksem();	/* Rel. 2.11 */

/***------------------------------------------------------------------------

  UpdateValueForCh
  Rel. 2.3

    --------------------------------------------------------------------***/
static void UpdateValueForCh(int devHandle, int crNum, int ch, void *p)
{
	time_t now;

/*	EnterCriticalSection(&cs);	 Rel. 2.10 */ 

	locksem();			/* Rel. 2.11 */

	time(&now);

	if( ch == -1 )								/* MonParam */
	{
		memcpy(n470AllParams[devHandle][crNum].MonParCh, p, sizeof(ushort)*4*4);
		n470AllParams[devHandle][crNum].monTime = now;
	}
	else
	{
		memcpy(&n470AllParams[devHandle][crNum].AllParCh[ch], p, sizeof(ushort)*11);
		n470AllParams[devHandle][crNum].setTime[ch] = now;
	}

/*	LeaveCriticalSection(&cs);	 Rel. 2.10 */
	unlocksem();			/* Rel. 2.11 */
}

/***------------------------------------------------------------------------

  GetValueForCh
  Rel. 2.3

    --------------------------------------------------------------------***/
static void GetValueForCh(int devHandle, int crNum, int ch, void *p)
{
	if( ch == -1 )								/* MonParam */
		memcpy(p, n470AllParams[devHandle][crNum].MonParCh, sizeof(ushort)*4*4);
	else
		memcpy(p, &n470AllParams[devHandle][crNum].AllParCh[ch], sizeof(ushort)*11);
}

/***------------------------------------------------------------------------

  N470GetChName

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
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
		    /* Rel. 2.4 - Added ( and )  for gcc */
			if(    ( ChList[i] >= 4 && WhichDevice[devHandle][crNum] == N470_DEV )
				|| ( ChList[i] >= 4 && WhichDevice[devHandle][crNum] == N470A_DEV )
				|| ( ChList[i] >= 2 && WhichDevice[devHandle][crNum] == N570_DEV ) )
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

/***------------------------------------------------------------------------

  N470SetChName

    --------------------------------------------------------------------***/
CAENHVRESULT N470SetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName)
{
	CAENHVRESULT res;

	errors[devHandle][crNum] = -255;
	res = CAENHV_WRITEERR;

	return res;
}

/***------------------------------------------------------------------------

  N470GetChParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetChParamInfo(int devHandle, int crNum, ushort slot, ushort Ch, 
								 char **ParNameList)
{
	int				i;
	CAENHVRESULT	res;

	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	/* Rel. 2.4 - Added ( and )  for gcc */
	else if(   ( Ch >= 4 && WhichDevice[devHandle][crNum] == N470_DEV )
			|| ( Ch >= 4 && WhichDevice[devHandle][crNum] == N470A_DEV )
			|| ( Ch >= 2 && WhichDevice[devHandle][crNum] == N570_DEV ) )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		char (*p)[MAX_PARAM_NAME] = malloc((12+1)*MAX_PARAM_NAME);

		for( i = 0 ; i < 12 ; i++ )
			strcpy(p[i], ChPar[i]);

		strcpy(p[i],"");
		*ParNameList = (char *)p;
		
		res = CAENHV_OK;
	}

	return res;
}

/***------------------------------------------------------------------------

  N470GetChParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetChParamProp(int devHandle, int crNum, ushort slot, ushort Ch, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	CAENHVRESULT  res = CAENHV_OK;
	
	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	/* Rel. 2.4 - Added ( and )  for gcc */
	else if(   ( Ch >= 4 && WhichDevice[devHandle][crNum] == N470_DEV )
			|| ( Ch >= 4 && WhichDevice[devHandle][crNum] == N470A_DEV )
			|| ( Ch >= 2 && WhichDevice[devHandle][crNum] == N570_DEV ) )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		if( !strcmp( ParName, "V0Set" ) || !strcmp( ParName, "V1Set" ) )
		{
			if( !strcmp(PropName,"Type") )										
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
			{
				if(    WhichDevice[devHandle][crNum] == N470_DEV 
					|| WhichDevice[devHandle][crNum] == N470A_DEV )
					*(float *)retval = 8000.0; 
				else if( WhichDevice[devHandle][crNum] == N570_DEV )
					*(float *)retval = 15000.0; 
			}
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_VOLT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "I0Set" ) || !strcmp( ParName, "I1Set" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )     
				*(float *)retval = (float)0.0;
			else if( !strcmp(PropName,"Maxval") )
			{
				if( WhichDevice[devHandle][crNum] == N470_DEV )
					*(float *)retval = 3000.0; 
				else if( WhichDevice[devHandle][crNum] == N470A_DEV )
					*(float *)retval = 800.0; 
				else if( WhichDevice[devHandle][crNum] == N570_DEV )
					*(float *)retval = 1000.0; 
			}
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_AMPERE;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = -6;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "RUp" ) || !strcmp( ParName, "RDwn" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 1.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = 500.0;
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_VPS;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "Trip" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = (float)0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = (float)99.99;
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_SECOND;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "Pw" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_ONOFF;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Onstate") )
				strcpy((char *)retval, "On");
			else if( !strcmp(PropName,"Offstate") )				
				strcpy((char *)retval, "Off");
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "VMon" ) || !strcmp( ParName, "HVMax" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDONLY;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
			{
				if(    WhichDevice[devHandle][crNum] == N470_DEV 
					|| WhichDevice[devHandle][crNum] == N470A_DEV )
					*(float *)retval = 8000.0; 
				else if( WhichDevice[devHandle][crNum] == N570_DEV )
					*(float *)retval = 15000.0; 
			}
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_VOLT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "IMon" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDONLY;
			else if( !strcmp(PropName,"Minval") )     
				*(float *)retval = (float)0.0;
			else if( !strcmp(PropName,"Maxval") )
			{
				if( WhichDevice[devHandle][crNum] == N470_DEV )
					*(float *)retval = 3000.0; 
				else if( WhichDevice[devHandle][crNum] == N470A_DEV )
					*(float *)retval = 800.0; 
				else if( WhichDevice[devHandle][crNum] == N570_DEV )
					*(float *)retval = 1000.0; 
			}
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_AMPERE;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = -6;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "Status" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_CHSTATUS;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDONLY;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else
			res = CAENHV_PARAMNOTFOUND;
	}

	return res;
}

/***------------------------------------------------------------------------

  N470GetChParam
	
	Rel. 2.3 - Added an optimization
	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
							 ushort ChNum, const ushort *ChList, void *ParValList)
{
	int                 i, par, parType, code;
	char                cnetbuff[MAX_LENGTH_FIFO];
	CAENHVRESULT        res = CAENHV_OK;

	if( !strcmp(ParName, "V0Set") )
	{
		par = PAR_V0S;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "V1Set") )
	{
		par = PAR_V1S;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "I0Set") )
	{
		par = PAR_I0S;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "I1Set") )
	{
		par = PAR_I1S;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "RUp") )
	{
		par = PAR_RUP;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "RDwn") )
	{
		par = PAR_RDW;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "Trip") )
	{
		par = PAR_TRP;
		parType = 1;						/* Set */
	}
	else if( !strcmp(ParName, "VMon") )
	{
		par = PAR_VMN;
		parType = 0;						/* Mon */
	}
	else if( !strcmp(ParName, "Pw") )
	{
		par = PAR_PW;
		parType = 0;						/* Mon */
	}
	else if( !strcmp(ParName, "IMon") )
	{
		par = PAR_IMN;
		parType = 0;						/* Mon */
	}
	else if( !strcmp(ParName, "HVMax") )
	{
		par = PAR_VMX;
		parType = 0;						/* Mon */
	}
	else if( !strcmp(ParName, "Status") )
	{
		par = PAR_STS;
		parType = 0;						/* Mon */
	}
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
		if( parType == 0 )					/* Mon: only one CAENET cycle */
		{
			struct n470chMon chMonPar[4];

/* Rel. 2.3 - Added the optimization */
			if( IsTimeElapsedForCh(devHandle, crNum, -1) )
			{
				code = READ_CHS_MON;

				errors[devHandle][crNum] = 
					caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
				
				if( errors[devHandle][crNum] == TUTTOK )
				{
					memcpy(chMonPar, cnetbuff, sizeof(ushort)*4*4);

					UpdateValueForCh(devHandle, crNum, -1, chMonPar);	/* Rel. 2.3 */
				}
				else
					res = CAENHV_READERR;
			} /* End if TimeElapsed */
			else
				GetValueForCh(devHandle, crNum, -1, chMonPar);

			if( res == CAENHV_OK )
			{
				for( i = 0 ; i < ChNum ; i++ )
				{
					/* Rel. 2.4 - Added ( and )  for gcc */
					if(    ( ChList[i] >= 4 && WhichDevice[devHandle][crNum] == N470_DEV )
						|| ( ChList[i] >= 4 && WhichDevice[devHandle][crNum] == N470A_DEV )
						|| ( ChList[i] >= 2 && WhichDevice[devHandle][crNum] == N570_DEV ) )
					{
						errors[devHandle][crNum] = -254;
						res = CAENHV_OUTOFRANGE;
						break;
					}
					else
					{
						switch( par )									
						{
							case PAR_VMN:
		 			  			*((float *)ParValList + i) = 
								  (float)chMonPar[ChList[i]].vmon;
								break;
				
						  	case PAR_IMN:
								if( WhichDevice[devHandle][crNum] == N470A_DEV )
								  	*((float *)ParValList + i) = 
									(float)chMonPar[ChList[i]].imon/(float)10.0;
								else
							  		*((float *)ParValList + i) = 
									(float)chMonPar[ChList[i]].imon;
								break;
			
							case PAR_STS:
								*((unsigned long *)ParValList + i) = 
								  chMonPar[ChList[i]].status & 0x1ff;
								break;

							case PAR_PW:
								*((unsigned long *)ParValList + i) = 
								 chMonPar[ChList[i]].status & 1;
								break;

							case PAR_VMX:
								*((float *)ParValList + i) = 
								 (float)chMonPar[ChList[i]].hvmax;
								break;
						} /* End switch */
					} /* End if-else */
				} /* End for */
			} /* End if( res) */
		} /* End if( parType )	 */
		else								/* Set: more CAENET cycles */
		{
			struct n470ch chPar;

			for( i = 0 ; i < ChNum ; i++ )
			{
/* Rel. 2.3 - Added the optimization */
				if( IsTimeElapsedForCh(devHandle, crNum, ChList[i]) )
				{
					code = MAKE_CODE(ChList[i], READ_ALL_PARAM_CH);

					errors[devHandle][crNum] = 
						caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);

					if( errors[devHandle][crNum] == TUTTOK )
					{
						memcpy(&chPar, cnetbuff, sizeof(ushort)*11);
						UpdateValueForCh(devHandle, crNum, ChList[i], &chPar); /* Rel. 2.3 */
					}
					else
					{
						res = CAENHV_READERR;
						break;
					}
				}
				else
					GetValueForCh(devHandle, crNum, ChList[i], &chPar);		/* Rel. 2.3 */

				switch( par )									
				{
			  		case PAR_V0S:
						*((float *)ParValList + i) = (float)chPar.v0set;
						break;
	
		  			case PAR_I0S:
						if( WhichDevice[devHandle][crNum] == N470A_DEV )
							*((float *)ParValList + i) = (float)chPar.i0set/(float)10.0;
						else
							*((float *)ParValList + i) = (float)chPar.i0set;
				  		break;
							
					case PAR_V1S:
						*((float *)ParValList + i) = (float)chPar.v1set;
						break;
			
		  			case PAR_I1S:
						if( WhichDevice[devHandle][crNum] == N470A_DEV )
							*((float *)ParValList + i) = (float)chPar.i1set/(float)10.0;
						else
							*((float *)ParValList + i) = (float)chPar.i1set;
				  		break;
			
		  			case PAR_RUP:
						*((float *)ParValList + i) = (float)chPar.rup;
			  			break;
			
		  			case PAR_RDW:
						*((float *)ParValList + i) = (float)chPar.rdwn;
			  			break;
			
		  			case PAR_TRP:
						*((float *)ParValList + i) = (float)chPar.trip/(float)100.0;
			  			break;
				} /* End switch */
			} /* End for() */
		}  /* End if-else( parType )	 */
	} /* End if-else (par) */
 
	return res;
}

/***------------------------------------------------------------------------

  N470SetChParam
    
	Rel.2.05 - If Param == "Pw" we must get back the channel status
	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.


    --------------------------------------------------------------------***/
CAENHVRESULT N470SetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
						     ushort ChNum, const ushort *ChList, void *ParValue)
{
	ushort              cnet_buff,
		                ch_sts;                                /* Rel. 2.5 */
	void                *dest = NULL;                          /* Rel. 2.5 */
	int                 code, i, par, dataw = sizeof(cnet_buff);
	CAENHVRESULT        res = CAENHV_OK;

	if( !strcmp(ParName, "V0Set") )
		par = PAR_V0S;
	else if( !strcmp(ParName, "V1Set") )
		par = PAR_V1S;
	else if( !strcmp(ParName, "I0Set") )
		par = PAR_I0S;
	else if( !strcmp(ParName, "I1Set") )
		par = PAR_I1S;
	else if( !strcmp(ParName, "RUp") )
		par = PAR_RUP;
	else if( !strcmp(ParName, "RDwn") )
		par = PAR_RDW;
	else if( !strcmp(ParName, "Trip") )
		par = PAR_TRP;
	else if( !strcmp(ParName, "Pw") )
		par = PAR_PW;
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
				case PAR_V0S:
					code = MAKE_CODE(ChList[i], SET_V0SET);
					cnet_buff = (ushort)(*(float *)ParValue);
					break;
				case PAR_V1S:
					code = MAKE_CODE(ChList[i], SET_V1SET);
					cnet_buff = (ushort)(*(float *)ParValue);
					break;
				case PAR_I0S:
					code = MAKE_CODE(ChList[i], SET_I0SET);
					if( WhichDevice[devHandle][crNum] == N470A_DEV )
						cnet_buff = (ushort)(*(float *)ParValue*10.0 + 0.5);
					else
						cnet_buff = (ushort)*(float *)ParValue;
					break;
				case PAR_I1S:
					code = MAKE_CODE(ChList[i], SET_I1SET);
					if( WhichDevice[devHandle][crNum] == N470A_DEV )
						cnet_buff = (ushort)(*(float *)ParValue*10.0 + 0.5);
					else
						cnet_buff = (ushort)*(float *)ParValue;
					break;
				case PAR_RUP:
					code = MAKE_CODE(ChList[i], SET_RUP);
					cnet_buff = (ushort)*(float *)ParValue;
					break;
				case PAR_RDW:
					code = MAKE_CODE(ChList[i], SET_RDWN);
					cnet_buff = (ushort)*(float *)ParValue;
					break;
				case PAR_TRP:
					code = MAKE_CODE(ChList[i], SET_TRIP);
					cnet_buff = (ushort)(*(float *)ParValue*100.0 + 0.05);
					break;
				case PAR_PW:
					cnet_buff = (ushort)( *(unsigned *)ParValue );
					cnet_buff &= 1;
					if( cnet_buff )
						code = MAKE_CODE(ChList[i], SET_CH_ON);
					else
						code = MAKE_CODE(ChList[i], SET_CH_OFF);
					dataw = 0;
					dest = &ch_sts;			/* Rel. 2.5 */
				  break;
			}

/*			errors[devHandle][cratenum] = caenet_comm(devHandle, &cnet_buff, dataw, NULL); */
/* Rel. 2.5 */
			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code,  &cnet_buff, dataw, dest);
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

  N470TestBdPresence

  Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N470TestBdPresence(int devHandle, int crNum, ushort slot, char *Model, 
								 char *Description, ushort *SerNum, 
								 uchar *FmwRelMin, uchar *FmwRelMax)
{
	char         tempbuff[80];
	CAENHVRESULT res = CAENHV_OK;
	int          code;

	if( slot >= 1 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		if( WhichDevice[devHandle][crNum] == N470A_DEV ) 
		{
			strcpy(Model, "N470A");
			strcpy(Description, "Four Channel Programmable Power Supply");
		}
		else if( WhichDevice[devHandle][crNum] == N470_DEV ) 
		{
			strcpy(Model, "N470");
			strcpy(Description, "Four Channel Programmable Power Supply");
		}
		else
		{
			strcpy(Model, "N570");
			strcpy(Description, "Two Channel Programmable Power Supply");
		}

		*SerNum = 0;

		code    = IDENT;
		memset(tempbuff, '\0', sizeof(tempbuff));
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
		if( errors[devHandle][crNum] == TUTTOK )
		{ 
			if( WhichDevice[devHandle][crNum] == N470A_DEV )
			{
				*FmwRelMax = tempbuff[28]-'0';
				*FmwRelMin = tempbuff[32]-'0';
			}
			else
			{
				*FmwRelMax = tempbuff[26]-'0';
				*FmwRelMin = tempbuff[30]-'0';
			}
		}
		else
			res = CAENHV_READERR;
	}

	return res;
}

/***------------------------------------------------------------------------

  N470GetBdParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetBdParamInfo(int devHandle, int crNum, ushort slot, 
								 char **ParNameList)
{		
	return CAENHV_NOTPRES;
}

/***------------------------------------------------------------------------

  N470GetBdParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetBdParamProp(int devHandle, int crNum, ushort slot, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  N470GetBdParam

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValList)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  N470SetBdParam

    --------------------------------------------------------------------***/
CAENHVRESULT N470SetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValue)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  N470GetSysComp

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetSysComp(int devHandle, int crNum, uchar *NrOfCr,
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

	if( WhichDevice[devHandle][crNum] == N570_DEV )
		app2[0] = ( 1 << 0x10 ) | 2;
	else
		app2[0] = ( 1 << 0x10 ) | 4;

	return res;
}

/***------------------------------------------------------------------------

  N470GetCrateMap

	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetCrateMap(int devHandle, int crNum, ushort *NrOfSlot, 
							  char **ModelList, char **DescriptionList, 
							  ushort **SerNumList, uchar **FmwRelMinList, 
							  uchar **FmwRelMaxList)
{
	CAENHVRESULT    res = CAENHV_OK;
	char            tempbuff[80];
	char			Desc[80]  = "Four Channel Programmable Power Supply", 
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

	if( WhichDevice[devHandle][crNum] == N470A_DEV )
		strcpy(*ModelList, "N470A"); 
	else if( WhichDevice[devHandle][crNum] == N470_DEV ) 
		strcpy(*ModelList, "N470"); 
	else
		strcpy(*ModelList, "N570");
	
	if( WhichDevice[devHandle][crNum] == N570_DEV )
	    strcpy(Desc, "Two Channel Programmable Power Supply");

	memcpy(*DescriptionList+desclen, Desc, strlen(Desc)+1);

	app3[0]  = 0;

	code    = IDENT;
	memset(tempbuff, '\0', sizeof(tempbuff));
	errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
	if( errors[devHandle][crNum] == TUTTOK )
	{ 
		if( WhichDevice[devHandle][crNum] == N470A_DEV )
		{
			app2[0] = tempbuff[28]-'0';			/* FmwRelMax */
			app1[0] = tempbuff[32]-'0';			/* FmwRelMin */
		}
		else
		{
			app2[0] = tempbuff[26]-'0';			/* FmwRelMax */
			app1[0] = tempbuff[30]-'0';			/* FmwRelMin */
		}
	}
	else
		res = CAENHV_READERR;

	return res;
}

/***------------------------------------------------------------------------

  N470GetExecCommList

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetExecCommList(int devHandle, int crNum, ushort *NumComm, 
								  char **CommNameList)
{
	ushort comm		  = sizeof(execComm)/sizeof(char *);
	int    i, commlen = 0;

	*CommNameList     = malloc(comm*80);
	*NumComm          = comm;

	for( i = 0 ; i < comm ; i++ )
	{
		memcpy(*CommNameList + commlen, execComm[i], strlen(execComm[i])+1);
		commlen += strlen(execComm[i])+1;
	}

	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N470ExecComm

	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N470ExecComm(int devHandle, int crNum, const char *CommName)
{
	CAENHVRESULT res = CAENHV_OK;
	int code;

	if( !strcmp(CommName, execComm[0]) )	/* Kill */
	{
		code = KILL_CH;

		errors[devHandle][crNum]  = caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[1]) )	/* Clear Alarm */
	{
		code = CLEAR_ALARM;

		errors[devHandle][crNum]  = caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else
		res = CAENHV_EXECNOTFOUND;

	return res;
}

/***------------------------------------------------------------------------

  N470GetSysPropList

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetSysPropList(int devHandle, int crNum, ushort *NumProp, 
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

  N470GetSysPropInfo

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetSysPropInfo(int devHandle, int crNum, const char *PropName, 
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

  N470GetSysProp
  
	Rel. 2.3 - Added an optimization
	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N470GetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Result)
{
	int          i, code;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		    /* ModelName */
	{
		if( WhichDevice[devHandle][crNum] == N470A_DEV )
			strcpy((char *)Result, "N470A");
		else if(  WhichDevice[devHandle][crNum] == N470_DEV )
			strcpy((char *)Result, "N470");
		else
			strcpy((char *)Result, "N570");
		res = CAENHV_OK;
	}
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
	{
		char    syident[80], tempbuff[80];

		code    = IDENT;
		memset(tempbuff, '\0', sizeof(tempbuff));
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
		if( errors[devHandle][crNum] == TUTTOK )
		{ 
			if( WhichDevice[devHandle][crNum] == N470A_DEV )
			{
				for( i = 28 ; tempbuff[i] != '\0' ; i+=2 )
					syident[i/2-14] = tempbuff[i];
				syident[i/2-14] = '\0';
				strcpy((char *)Result, syident);
			}
			else
			{
				for( i = 26 ; tempbuff[i] != '\0' ; i+=2 )
					syident[i/2-13] = tempbuff[i];
				syident[i/2-13] = '\0';
				strcpy((char *)Result, syident);
			}
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
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* LockKeyboard */
	{
		*(unsigned *)Result = 0;
		res = CAENHV_OK;                              /* Like ExecComms */
	}
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* FrontPanStat */
	{
		struct n470ch  chPar;
		char           cnetbuff[40];

		if( IsTimeElapsedForCh(devHandle, crNum, 0) )
		{
			code = MAKE_CODE(0, READ_ALL_PARAM_CH);
			errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
			if( errors[devHandle][crNum] == TUTTOK )
			{
				memcpy(&chPar, cnetbuff, sizeof(ushort)*11);
				UpdateValueForCh(devHandle, crNum, 0, &chPar); /* Rel. 2.3 */
				res = CAENHV_OK;
			}
			else
				res = CAENHV_READERR;
		}
		else
			GetValueForCh(devHandle, crNum, 0, &chPar);		 /* Rel. 2.3 */

		if( res != CAENHV_READERR )
		{
			*(ushort *)Result = ( chPar.status & 0xfe00 ) >> 9;
			res = CAENHV_OK;
		}
	}
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* LevelSelect */
	{
		struct n470ch  chPar;
		unsigned       levsel;
		char           cnetbuff[40];

		if( IsTimeElapsedForCh(devHandle, crNum, 0) )
		{
			code = MAKE_CODE(0, READ_ALL_PARAM_CH);
			errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
			if( errors[devHandle][crNum] == TUTTOK )
			{
				memcpy(&chPar, cnetbuff, sizeof(ushort)*11);
				UpdateValueForCh(devHandle, crNum, 0, &chPar); /* Rel. 2.3 */
				res = CAENHV_OK;
			}
			else
				res = CAENHV_READERR;
		}
		else
			GetValueForCh(devHandle, crNum, 0, &chPar);  		/* Rel. 2.3 */

		if( res != CAENHV_READERR )
		{
			levsel = ( chPar.status & ( 1 << 13 ) ) >> 13;
			*(unsigned *)Result = levsel;
			res = CAENHV_OK;
		}
	}

	return res;
}

/***------------------------------------------------------------------------

  N470SetSysProp

	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT N470SetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Set)
{
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;
	int code;

	if( !strcmp(PropName, sysProp.name[0]) )		  /* ModelName */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[2]) )	  /* CnetCrNum */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* LockKeyboard */
	{
		unsigned lock = *(unsigned *)Set;

		if( lock )
			code = KEYB_DIS;
		else
			code = KEYB_EN;

		errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* FrontPanStat */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* LevelSelect */
	{
		unsigned level = *(unsigned *)Set;

		if( level )
			code = TTL_LEVEL;
		else
			code = NIM_LEVEL;

		errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}

	return res;
}

/***------------------------------------------------------------------------

  N470Init
  This is one of the three possible entry points into the routines of this file: 
  it is explicitely called when the user calls CAENHVInitSystem()
  See also N570Init and N470AInit
  Rel. 2.3 - Added n470AllParams

    --------------------------------------------------------------------***/
int N470Init(int devHandle, int crNum)
{
	int i;

	for( i = 0 ; i < 4 ; i++ )
	{
		n470AllParams[devHandle][crNum].monTime = 0;
		n470AllParams[devHandle][crNum].setTime[i] = 0;
	}

	WhichDevice[devHandle][crNum] = N470_DEV;
	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N470AInit
  This is one of the three possible entry points into the routines of this file: 
  it is explicitely called when the user calls CAENHVInitSystem()
  See also N470Init and N570Init
  Rel. 2.3 - Added n470AllParams

    --------------------------------------------------------------------***/
int N470AInit(int devHandle, int crNum)
{
	int i;

	for( i = 0 ; i < 4 ; i++ )
	{
		n470AllParams[devHandle][crNum].monTime = 0;
		n470AllParams[devHandle][crNum].setTime[i] = 0;
	}

	WhichDevice[devHandle][crNum] = N470A_DEV;
	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N570Init
  This is one of the three possible entry points into the routines of this file: 
  it is explicitely called when the user calls CAENHVInitSystem()
  See also N470Init and N470AInit
  Rel. 2.3 - Added n470AllParams

    --------------------------------------------------------------------***/
int N570Init(int devHandle, int crNum)
{
	int i;

	for( i = 0 ; i < 4 ; i++ )
	{
		n470AllParams[devHandle][crNum].monTime = 0;
		n470AllParams[devHandle][crNum].setTime[i] = 0;
	}

	WhichDevice[devHandle][crNum] = N570_DEV;
	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  N470End

    --------------------------------------------------------------------***/
void N470End(int devHandle, int crNum)
{
	WhichDevice[devHandle][crNum] = NOT_DEV;  /* Prob. not necessary ... */
}

/***------------------------------------------------------------------------

  N470GetError

    --------------------------------------------------------------------***/
char *N470GetError(int devHandle, int crNum)
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

