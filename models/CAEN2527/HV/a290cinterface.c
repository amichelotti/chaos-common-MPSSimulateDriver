/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    A290CINTERFACE.C                                                     */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    May      2004    : Created  Rel. 2.12                                */
/*                                                                         */
/***************************************************************************/
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#ifdef UNIX
#include  <errno.h>			                /* Rel. 2.0 - Linux */
#endif
#include  "HSCAENETLib.h"	                        /* Rel. 2.1 */
#include  "CAENHVWrapper.h"
#include  "a290cinterface.h"

#define   MAX_NR_OF_CARDS     10			/* Rel. 2.1 - It seems more than enough */

/*
  Some of the Caenet Codes
*/
#define   IDENT                                  0x0
#define   READ_p5PARAMS                          0x1
#define   READ_p12PARAMS                         0x2
#define   READ_p24PARAMS                         0x3
#define   READ_p5STDBYPARAMS                     0x4
#define   READ_m5PARAMS                          0x5
#define   READ_m12PARAMS                         0x6
#define   READ_m24PARAMS                         0x7
#define   READ_m2PARAMS                          0x8
#define   READ_TEMPS	                         0x9
#define   READ_FAN_SPEEDS                       0x10
#define   READ_PS_INFO							0x18
#define   READ_PS_STATUS                        0x19

#define   CLEAR_ALARM                           0x80
#define   PS_INIT                               0x83
#define   PS_ON                                 0x84
#define   PS_OFF                                0x85
#define   VME_SYSRES                            0x86
#define   TMODE_ON                              0x87
#define   TMODE_OFF                             0x88

#define   SET_FAN_SPEED							0x8D
#define   SET_p5ISET							0xA0
#define   SET_p12ISET							0xA1
#define   SET_p24ISET							0xA2
#define   SET_p5SDBYISET						0xA3
#define   SET_m5ISET							0xA4
#define   SET_m12ISET							0xA5
#define   SET_m24ISET							0xA6
#define   SET_m2ISET							0xA7
 

/*
  The following structure contains all the useful information about
  the settings and monitorings of a power supply
*/
static struct scaleTag
{
 ushort  p5VMon;
 ushort  p5IMon;
 ushort  p12VMon;
 ushort  p12IMon;
 ushort  p24VMon;
 ushort  p24IMon;
 ushort  p5StdbyVMon;
 ushort  p5StdbyIMon;
 ushort  m5VMon;
 ushort  m5IMon;
 ushort  m12VMon;
 ushort  m12IMon;
 ushort  m24VMon;
 ushort  m24IMon;
 ushort  m2VMon;
 ushort  m2IMon;
} scale;

static float decv[] = { 100.0, 1000.0 };
static float deci[] = { 1.0, 10.0, 100.0, 1000.0 };

static int            errors[MAX_NR_OF_CARDS][100];		/* The last error code */

/* For ExecComm routines */
static const char *execComm[] = {	"ClearAlarm", "InitPS", "OnPS", "OffPS", "VMESysRes",
									"TestModeOn", "TestModeOff" };

/* For SysProp routines */
static struct sysPropTag 
{
	unsigned num;
	unsigned present[53];
	char     *name[53];
	unsigned mode[53];
	unsigned type[53];
} sysProp = { 
		53,
		{
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1
		},
	{	
		"ModelName", "SwRelease", "CnetCrNum", 
		"p5VMon", "p5Imon", "p5Iset", "p5Status", 
		"p12VMon", "p12Imon", "p12Iset", "p12Status", 
		"p24VMon", "p24Imon", "p24Iset", "p24Status", 
		"p5StdbyVMon", "p5StdbyImon", "p5StdbyIset", "p5StdbyStatus", 
		"m5VMon", "m5Imon", "m5Iset", "m5Status", 
		"m12VMon", "m12Imon", "m12Iset", "m12Status", 
		"m24VMon", "m24Imon", "m24Iset", "m24Status", 
		"m2VMon", "m2Imon", "m2Iset", "m2Status", 
		"CrateTemp", "PSTemp", "SwitchFanSpeed",
		"AverageFanSpeed", "Fan1Speed", "Fan2Speed",
		"Fan3Speed", "Fan4Speed", "Fan5Speed", "Fan6Speed",
		"Alarm", "Unv", "Ovv", "Ovc", "Oct", "Config0",
		"Config1", "PSStatus"
	}, 
	{	
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, 
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDWR,   SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY,
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY

	}, 
	{
		SYSPROP_TYPE_STR, SYSPROP_TYPE_STR, SYSPROP_TYPE_UINT2, 
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_REAL, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2,
		SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_UINT2		
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

  Get_cr_info

    --------------------------------------------------------------------***/
static int get_cr_info(int devHandle, int crNum)
{
	int     response, code;
	ushort  value[8];

	code = READ_PS_INFO;
	if( ( response = caenet_comm(devHandle, crNum, code, NULL,0,value) ) == TUTTOK )
	{
		int vrange, vres, ires;

		vrange = value[0] & 0x1f;
		vres   = ( ( value[0] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[0] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[3] = 0;
			sysProp.present[4] = 0;
			sysProp.present[5] = 0;
			sysProp.present[6] = 0;
		}
		else
		{
			sysProp.present[3] = 1;
			sysProp.present[4] = 1;
			sysProp.present[5] = 1;
			sysProp.present[6] = 1;
			scale.p5VMon = vres;
			scale.p5IMon = ires;
		}

		vrange = value[1] & 0x1f;
		vres   = ( ( value[1] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[1] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[7] = 0;
			sysProp.present[8] = 0;
			sysProp.present[9] = 0;
			sysProp.present[10] = 0;
		}
		else
		{
			sysProp.present[7] = 1;
			sysProp.present[8] = 1;
			sysProp.present[9] = 1;
			sysProp.present[10] = 1;
			scale.p12VMon = vres;
			scale.p12IMon = ires;
		}

		vrange = value[2] & 0x1f;
		vres   = ( ( value[2] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[2] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[11] = 0;
			sysProp.present[12] = 0;
			sysProp.present[13] = 0;
			sysProp.present[14] = 0;
		}
		else
		{
			sysProp.present[11] = 1;
			sysProp.present[12] = 1;
			sysProp.present[13] = 1;
			sysProp.present[14] = 1;
			scale.p24VMon = vres;
			scale.p24IMon = ires;
		}

		vrange = value[3] & 0x1f;
		vres   = ( ( value[3] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[3] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[15] = 0;
			sysProp.present[16] = 0;
			sysProp.present[17] = 0;
			sysProp.present[18] = 0;
		}
		else
		{
			sysProp.present[15] = 1;
			sysProp.present[16] = 1;
			sysProp.present[17] = 1;
			sysProp.present[18] = 1;
			scale.p5StdbyVMon = vres;
			scale.p5StdbyIMon = ires;
		}

		vrange = value[4] & 0x1f;
		vres   = ( ( value[4] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[4] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[19] = 0;
			sysProp.present[20] = 0;
			sysProp.present[21] = 0;
			sysProp.present[22] = 0;
		}
		else
		{
			sysProp.present[19] = 1;
			sysProp.present[20] = 1;
			sysProp.present[21] = 1;
			sysProp.present[22] = 1;
			scale.m5VMon = vres;
			scale.m5IMon = ires;
		}

		vrange = value[5] & 0x1f;
		vres   = ( ( value[5] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[5] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[23] = 0;
			sysProp.present[24] = 0;
			sysProp.present[25] = 0;
			sysProp.present[26] = 0;
		}
		else
		{
			sysProp.present[23] = 1;
			sysProp.present[24] = 1;
			sysProp.present[25] = 1;
			sysProp.present[26] = 1;
			scale.m12VMon = vres;
			scale.m12IMon = ires;
		}

		vrange = value[6] & 0x1f;
		vres   = ( ( value[6] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[6] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 ) 
		{
			sysProp.present[27] = 0;
			sysProp.present[28] = 0;
			sysProp.present[29] = 0;
			sysProp.present[30] = 0;
		}
		else
		{
			sysProp.present[27] = 1;
			sysProp.present[28] = 1;
			sysProp.present[29] = 1;
			sysProp.present[30] = 1;
			scale.m24VMon = vres;
			scale.m24IMon = ires;
		}

		vrange = value[7] & 0x1f;
		vres   = ( ( value[7] & ( 1 << 5 ) ) >> 5);
		ires   = ( ( value[7] & ( 3 << 6 ) ) >> 6);
		if( vrange == 0 )
		{
			sysProp.present[31] = 0;
			sysProp.present[32] = 0;
			sysProp.present[33] = 0;
			sysProp.present[34] = 0;
		}
		else
		{
			sysProp.present[31] = 1;
			sysProp.present[32] = 1;
			sysProp.present[33] = 1;
			sysProp.present[34] = 1;
			scale.m2VMon = vres;
			scale.m2IMon = ires;
		}
	}

	code = READ_FAN_SPEEDS;
	value[0] = 0xffff;
	value[1] = 0xffff;
	value[2] = 0xffff;
	value[3] = 0xffff;
	value[4] = 0xffff;
	value[5] = 0xffff;
	value[6] = 0xffff;
	value[7] = 0xffff;
	if( ( response = caenet_comm(devHandle, crNum, code, NULL,0,value) ) == TUTTOK )
	{
		if( value[5] == 0xffff && 
			value[6] == 0xffff &&
			value[7] == 0xffff )
		{
			sysProp.present[42] = 0;
			sysProp.present[43] = 0;
			sysProp.present[44] = 0; 
		}
		else
		{
			sysProp.present[42] = 1;
			sysProp.present[43] = 1;
			sysProp.present[44] = 1; 
		}
	}

	return response;
}

/***------------------------------------------------------------------------

  A290CSysInfo
  This procedure refreshes values of the internal data structures

    --------------------------------------------------------------------***/
static int A290CSysInfo(int devHandle, int crNum)
{

	errors[devHandle][crNum] = get_cr_info(devHandle, crNum);  /* Get info about the Crate Conf. */

	return errors[devHandle][crNum];
}

/***------------------------------------------------------------------------

  A290CGetChName
  
    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME])
{
	return CAENHV_OUTOFRANGE;
}

/***------------------------------------------------------------------------

  A290CSetChName

    --------------------------------------------------------------------***/
CAENHVRESULT A290CSetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName)
{
	return CAENHV_OUTOFRANGE;
}

/***------------------------------------------------------------------------

  A290CGetChParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetChParamInfo(int devHandle, int crNum, ushort slot, ushort Ch, 
								 char **ParNameList)
{
	return CAENHV_OUTOFRANGE;
}

/***------------------------------------------------------------------------

  A290CGetChParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetChParamProp(int devHandle, int crNum, ushort slot, ushort Ch, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	return CAENHV_OUTOFRANGE;
}

/***------------------------------------------------------------------------

  A290CGetChParam

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
							 ushort ChNum, const ushort *ChList, void *ParValList)
{
	return CAENHV_OUTOFRANGE;
}

/***------------------------------------------------------------------------

  A290CSetChParam

    --------------------------------------------------------------------***/
CAENHVRESULT A290CSetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
						     ushort ChNum, const ushort *ChList, void *ParValue)
{
	return CAENHV_OUTOFRANGE;
}

/***------------------------------------------------------------------------

  A290CTestBdPresence

    --------------------------------------------------------------------***/
CAENHVRESULT A290CTestBdPresence(int devHandle, int crNum, ushort slot, char *Model, 
								 char *Description, ushort *SerNum, 
								 uchar *FmwRelMin, uchar *FmwRelMax)
{
	return CAENHV_SLOTNOTPRES;
}

/***------------------------------------------------------------------------

  A290CGetBdParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetBdParamInfo(int devHandle, int crNum, ushort slot, 
								 char **ParNameList)
{		
	return CAENHV_SLOTNOTPRES;
}

/***------------------------------------------------------------------------

  A290CGetBdParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetBdParamProp(int devHandle, int crNum, ushort slot, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	return CAENHV_SLOTNOTPRES;
}

/***------------------------------------------------------------------------

  A290CGetBdParam

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValList)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  A290CSetBdParam

    --------------------------------------------------------------------***/
CAENHVRESULT A290CSetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValue)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  A290CGetSysComp

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetSysComp(int devHandle, int crNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList)
{
	CAENHVRESULT  res;

	if( ( res = A290CSysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		ushort *app1;
		ulong  *app2;

		*NrOfCr = 1;
		*CrNrOfSlList = malloc(sizeof(ushort));
		*SlotChList   = malloc(10*sizeof(ulong));

		app1 = *CrNrOfSlList;
		app2 = *SlotChList;

		*app1 = 0;
		*app2 = 0;
	}

	return res;
}

/***------------------------------------------------------------------------

  A290CGetCrateMap

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetCrateMap(int devHandle, int crNum, ushort *NrOfSlot, 
							  char **ModelList, char **DescriptionList, 
							  ushort **SerNumList, uchar **FmwRelMinList, 
							  uchar **FmwRelMaxList)
{
	CAENHVRESULT    res;

	char			Desc[80] = "                                 ";

/* The following ust be done, because the caller expects it ! */
	*ModelList       = malloc(10);			
	*DescriptionList = malloc((strlen(Desc)+1));
	*SerNumList      = malloc(sizeof(ushort));
	*FmwRelMinList   = malloc(sizeof(uchar));
	*FmwRelMaxList   = malloc(sizeof(uchar));
	*NrOfSlot = 0;

	res = A290CSysInfo(devHandle, crNum);

	return res;

}

/***------------------------------------------------------------------------

  A290CGetExecCommList

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetExecCommList(int devHandle, int crNum, ushort *NumComm, 
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

  A290CExecComm

    --------------------------------------------------------------------***/
CAENHVRESULT A290CExecComm(int devHandle, int crNum, const char *CommName)
{
	CAENHVRESULT res = CAENHV_OK;
	int          code;

	if( !strcmp(CommName, execComm[0]) )	/* Clear Alarm  */
	{
		code = CLEAR_ALARM;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[1]) )	/* Init PS */
	{
		code = PS_INIT;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[2]) )	/* On PS */
	{
		code = PS_ON;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[3]) )	/* Off PS */
	{
		code = PS_OFF;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[4]) )	/* VME Sys Res */
	{
		code = VME_SYSRES;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[5]) )	/* Test Mode On */
	{
		code = TMODE_ON;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[6]) )	/* Test Mode Off */
	{
		code = TMODE_OFF;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else
		res = CAENHV_EXECNOTFOUND;

	return res;
}

/***------------------------------------------------------------------------

  A290CGetSysPropList

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetSysPropList(int devHandle, int crNum, ushort *NumProp, 
								 char **PropNameList)
{
	ushort	count, 
			prop = sysProp.num;
	int		i, proplen  = 0;

	*PropNameList     = malloc(prop*80);

	for( i = 0, count = 0 ; i < prop ; i++ )
	{
		if( sysProp.present[i] )
		{
			count++;
			memcpy(*PropNameList + proplen, sysProp.name[i], strlen(sysProp.name[i])+1);
			proplen += strlen(sysProp.name[i])+1;
		}
	}

	*NumProp = count;

	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  A290CGetSysPropInfo

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetSysPropInfo(int devHandle, int crNum, const char *PropName, 
								 unsigned *PropMode, unsigned *PropType)
{
	unsigned     i;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	for( i = 0 ; i < sysProp.num ; i++ )
		if( sysProp.present[i] && !strcmp(PropName, sysProp.name[i]) )
		{
			*PropMode = sysProp.mode[i];
			*PropType = sysProp.type[i];
			res = CAENHV_OK;
			break;
		}

	return res;
}

/***------------------------------------------------------------------------

  A290CGetSysProp

    --------------------------------------------------------------------***/
CAENHVRESULT A290CGetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Result)
{
	int          i, code;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		    /* ModelName */
	{
		strcpy((char *)Result,"A290C");
		res = CAENHV_OK;
	}
	else if( !strcmp(PropName, sysProp.name[1]) )	    /* SwRelease */
	{
		char    syident[80], tempbuff[80];

		code    = IDENT;
		memset(tempbuff, '\0', sizeof(tempbuff));
		errors[devHandle][crNum] = 
			caenet_comm(devHandle, crNum, code, NULL, 0, tempbuff);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			for( i = 34 ; tempbuff[i] != '\0' ; i+=2 )
				syident[i/2-17] = tempbuff[i];
			syident[i/2-17] = '\0';
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
	else if( !strcmp(PropName, sysProp.name[3]) && sysProp.present[3] )	  /* +5VMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.p5VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[4]) && sysProp.present[4] )	  /* +5IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.p5IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[5]) && sysProp.present[5] )	  /* +5ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_p5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[6]) && sysProp.present[6] )	  /* +5Status */
	{
		ushort   v[4];

		code = READ_p5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[7]) && sysProp.present[7] )	  /* +12VMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.p12VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[8]) && sysProp.present[8] )	  /* +12IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.p12IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[9]) && sysProp.present[9] )	  /* +12ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_p12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[10]) && sysProp.present[10] )	  /* +12Status */
	{
		ushort   v[4];

		code = READ_p12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[11]) && sysProp.present[11] )	  /* +24VMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.p24VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[12]) && sysProp.present[12] )	  /* +24IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.p24IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[13]) && sysProp.present[13] )	  /* +24ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_p24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[14]) && sysProp.present[14] )	  /* +24Status */
	{
		ushort   v[4];

		code = READ_p24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[15]) && sysProp.present[15] )	  /* +5StdbyVMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p5STDBYPARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.p5StdbyVMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[16]) && sysProp.present[16] )	  /* +5StdbyIMon */
	{
		ushort   v[4];
		float    value;

		code = READ_p5STDBYPARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.p5StdbyIMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[17]) && sysProp.present[17] )	  /* +5StdbyISet */
	{
		ushort   v[4];
		float    value;

		code = READ_p5STDBYPARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[18]) && sysProp.present[18] )	  /* +5StdbyStatus */
	{
		ushort   v[4];

		code = READ_p5STDBYPARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[19]) && sysProp.present[19] )	  /* -5VMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.m5VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[20]) && sysProp.present[20] )	  /* -5IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.m5IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[21]) && sysProp.present[21] )	  /* -5ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_m5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[22]) && sysProp.present[22] )	  /* -5Status */
	{
		ushort   v[4];

		code = READ_m5PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[23]) && sysProp.present[23] )	  /* -12VMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.m12VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[24]) && sysProp.present[24] )	  /* -12IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.m12IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[25]) && sysProp.present[25] )	  /* -12ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_m12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[26]) && sysProp.present[26] )	  /* -12Status */
	{
		ushort   v[4];

		code = READ_m12PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[27]) && sysProp.present[27] )	  /* -24VMon  */
	{
		ushort   v[4];
		float    value;

		code = READ_m24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.m24VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[28]) && sysProp.present[28] )	  /* -24IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.m24IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[29]) && sysProp.present[29] )	  /* -24ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_m24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[30]) && sysProp.present[30] )	  /* -24Status */
	{
		ushort   v[4];

		code = READ_m24PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[31]) && sysProp.present[31] )	  /* -2VMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m2PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[0];
			value = value / decv[scale.m2VMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[32]) && sysProp.present[32] )	  /* -2IMon */
	{
		ushort   v[4];
		float    value;

		code = READ_m2PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[1];
			value = value / deci[scale.m2IMon];
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[33]) && sysProp.present[33] )	  /* -2ISet */
	{
		ushort   v[4];
		float    value;

		code = READ_m2PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			value = v[2];
			value = value / (float)2.0;
			*(float *)Result = value;
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[34]) && sysProp.present[34] )	  /* -2Status */
	{
		ushort   v[4];

		code = READ_m2PARAMS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[35]) && sysProp.present[35] )	  /* CrateTemp */
	{
		ushort   v[2];

		code = READ_TEMPS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[0];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[36]) && sysProp.present[36] )	  /* PSTemp */
	{
		ushort   v[2];

		code = READ_TEMPS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[1];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[37]) && sysProp.present[37] )	  /* SwFanSpeed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[0];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[38]) && sysProp.present[38] )	  /* AvFanSpeed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[1];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[39]) && sysProp.present[39] )	  /* Fan1Speed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[2];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[40]) && sysProp.present[40] )	  /* Fan2Speed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[41]) && sysProp.present[41] )	  /* Fan3Speed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[4];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[42]) && sysProp.present[42] )	  /* Fan4Speed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[5];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[43]) && sysProp.present[43] )	  /* Fan5Speed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[6];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[44]) && sysProp.present[44] )	  /* Fan6Speed */
	{
		ushort   v[8];

		code = READ_FAN_SPEEDS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[7];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[45]) && sysProp.present[45] )	  /* Alarm */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[0];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[46]) && sysProp.present[46] )	  /* Unv */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[1];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[47]) && sysProp.present[47] )	  /* Ovv */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[2];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[48]) && sysProp.present[48] )	  /* Ovc */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[3];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[49]) && sysProp.present[49] )	  /* Oct */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[4];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[50]) && sysProp.present[50] )	  /* Config0 */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[5];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[51]) && sysProp.present[51] )	  /* Config1 */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[6];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[52]) && sysProp.present[52] )	  /* PSStatus */
	{
		ushort   v[8];

		code = READ_PS_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v[7];
			
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}

	return res;
}

/***------------------------------------------------------------------------

  A290CSetSysProp

    --------------------------------------------------------------------***/
CAENHVRESULT A290CSetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Set)
{
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;
	int          code;

	if( !strcmp(PropName, sysProp.name[0]) )	  /* ModelName */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[2]) )	  /* CnetCrNum */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* p5VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* p5IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[6]) )	  /* p5Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[7]) )	  /* p12VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[8]) )	  /* p12IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[10]) )	  /* p12Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[11]) )	  /* p24VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[12]) )	  /* p24IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[14]) )	  /* p24Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[15]) )	  /* p5StdbyVMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[16]) )	  /* p5StdbyIMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[18]) )	  /* p5StdbyStatus */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[19]) )	  /* m5VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[20]) )	  /* m5IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[22]) )	  /* m5Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[23]) )	  /* m12VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[24]) )	  /* m12IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[26]) )	  /* m12Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[27]) )	  /* m24VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[28]) )	  /* m24IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[30]) )	  /* m24Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[31]) )	  /* m2VMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[32]) )	  /* m2IMon */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[34]) )	  /* m2Status */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[35]) )	  /* CrateTemp */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[36]) )	  /* PSTemp */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[37]) )	  /* SwitchFanSpeed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[39]) )	  /* Fan1Speed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[40]) )	  /* Fan2Speed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[41]) )	  /* Fan3Speed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[42]) )	  /* Fan4Speed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[43]) )	  /* Fan5Speed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[44]) )	  /* Fan6Speed */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[45]) )	  /* Alarm */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[46]) )	  /* Unv */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[47]) )	  /* Ovv */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[48]) )	  /* Ovc */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[49]) )	  /* Oct */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[50]) )	  /* Config0 */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[51]) )	  /* Config1 */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[52]) )	  /* PSStatus */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* p5Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.p5IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_p5ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[9]) )	  /* p12Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.p12IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_p12ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[13]) )	  /* p24Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.p24IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_p24ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[17]) )	  /* p5StdbyIset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.p5StdbyIMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_p5SDBYISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[21]) )	  /* m5Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.m5IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_m5ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[25]) )	  /* m12Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.m12IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_m12ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[29]) )	  /* m24Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.m24IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_m24ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[33]) )	  /* m2Iset */
	{
		ushort vs;
		float  vf = *(float *)Set;

		switch( scale.m2IMon )
		{
		case 0:				/* 1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 1:				/* 0.1 A */
			if( vf > 999.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 2:				/* 0.01 A */
			if( vf > 99.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;
		case 3:				/* 0.001 A */
			if( vf > 9.5 )
			{
				res = CAENHV_OUTOFRANGE;
				goto fine;
			}
			break;

		}
		code = SET_m2ISET;
		vs = (ushort)(vf * (float)2.0);
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[38]) )	  /* AverageFanSpeed */
	{
		ushort vs = *(ushort *)Set;

		if( vs > 3120 )
		{
			res = CAENHV_OUTOFRANGE;
			goto fine;
		}
		code = SET_FAN_SPEED;
		errors[devHandle][crNum] = 
		caenet_comm(devHandle,crNum, code,&vs,sizeof(ushort),NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}

fine:
	return res;
}


/***------------------------------------------------------------------------

  A290CInit
  This is the entry point into the routines of this file: it is explicitely
  called when the user calls CAENHVInitSystem()

    --------------------------------------------------------------------***/
int A290CInit(int devHandle, int crNum)
{
	return A290CSysInfo(devHandle, crNum);
}

/***------------------------------------------------------------------------

  A290CEnd

    --------------------------------------------------------------------***/
void A290CEnd(int devHandle, int crNum)
{
}

/***------------------------------------------------------------------------

  A290CGetError

    --------------------------------------------------------------------***/
char *A290CGetError(int devHandle, int crNum)
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

