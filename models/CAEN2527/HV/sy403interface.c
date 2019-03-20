/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY403INTERFACE.C                                                     */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    April    2002    : Rel. 2.01 - Created                               */
/*                                                                         */
/***************************************************************************/
#include  <string.h>
#include  <time.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <stdarg.h>
#include  <stdio.h>
#ifdef UNIX
#include  <errno.h>							
#endif
#include  "HSCAENETLib.h"					
#include  "CAENHVWrapper.h"
#include  "sy403interface.h"

#define   IDENT                                    0
#define   READ_STATUS                              1
#define   READ_SETTINGS                            2
#define   READ_BOARDS_INFO                         3
#define   READ_GEN_STATUS                          5
#define   READ_HVMAX                               6

#define   LIST_CH_GRP                           0x40
#define   MON_CH_GRP                            0x41
#define   ADD_CH_GRP                            0x50
#define   REM_CH_GRP                            0x51

#define   SET_V0SET                             0x10  
#define   SET_V1SET                             0x11  
#define   SET_I0SET                             0x12  
#define   SET_I1SET                             0x13  
#define   SET_VMAX                              0x14  
#define   SET_RUP                               0x15  
#define   SET_RDWN                              0x16  
#define   SET_TRIP                              0x17  
#define   SET_FLAG                              0x18
#define   SET_CH_NAME	                        0x19
#define   SET_STATUS_ALARM                      0x1a
#define   FORMAT_EEPROM_1                       0x30
#define   FORMAT_EEPROM_2                       0x31
#define   CLEAR_ALARM                           0x32
#define   LOCK_KEYBOARD                         0x33
#define   UNLOCK_KEYBOARD                       0x34
#define   KILL_CHANNELS_1                       0x35
#define   KILL_CHANNELS_2                       0x36

#define   GRP                                      0
#define   NO_GRP                                   1
#define   ADD                                      0
#define   REM                                      1

#define   PAR_V0S	  					   1
#define   PAR_I0S						   2
#define   PAR_V1S						   3
#define   PAR_I1S						   4
#define   PAR_RUP						   5
#define   PAR_RDW						   6
#define   PAR_TRP						   7
#define   PAR_VMX						   8
#define   PAR_PON						   9
#define   PAR_PW						  10
#define   PAR_PEN						  11
#define   PAR_PDN						  12
#define   PAR_PSW						  13
#define   PAR_VMN						  14
#define   PAR_IMN						  15
#define   PAR_STS						  16
#define   PAR_NON						  17

#define   MAX_NR_OF_CARDS     10			


/*
  The following structure contains all the useful information about
  the settings of a channel
*/
struct hvch
{
 char    chname[12];
 long    v0set, v1set;
 ushort  i0set, i1set;
 short   vmax;
 short   rup, rdwn;
 short   trip;
 ushort  flag;
};

/*
  The following structure contains all the useful information about
  the status of a channel
*/
struct hvrd
{
 long   vread;
 short  iread;
 ushort status;
};

/*
  The following structure contains all the useful information about
  the voltage/current limits/resolutions of the 4 installed boards
*/
struct bd 
{
 short   vmax[4];
 short   imax[4];
 short   resv[4];
 short   resi[4];
 short   decv[4];
 short   deci[4];
 short   present[4];
};

/*
  The following structure contains all the useful information about
  the alarm status of SY527
*/
struct st_al
{
 unsigned  level   :1;
 unsigned  pulsed  :1;
 unsigned  ovc     :1;
 unsigned  ovv     :1;
 unsigned  unv     :1;
 unsigned  unused  :11;
};

/*
  Globals
  Rel. 2.10 - Added const to constant global variables.
            - Deleted the code and cratenum global variables
			  due to interfernce between thread.
*/
/* static int    			  code, cratenum; */

/* Rel. 2.10 - The boards variable contains static information about limits
			   and resolution. Also it is indexed by crate number, a1303 number
			   and board slot, so the interferences between threads are avoided 
			   because the same values are written in the same place.
*/
static struct bd	      boards[MAX_NR_OF_CARDS][100];

/* Rel. 2.10 - The errors variable probably is a source of, i think, rare interferences
			   because a thread can report an error of an another thread. 
*/
static int                errors[MAX_NR_OF_CARDS][100];			/* The last error code */

static const float		  pow_10[] = { 1.0, 10.0, 100.0, 1000.0 };
static const float		  pow_01[] = { 1.0, (float)0.10, (float)0.010, (float)0.0010 };

/* The list of Channels' Parameters for SY403 */
static const char         *ChPar[] = {
	"V0Set", 
	"I0Set", 
	"V1Set", 
	"I1Set", 
	"RUp",	 
	"RDwn",	 
	"Trip",	 
	"SVMax", 
	"POn",
	"Pw",
	"PEn",
	"PDwn",
	"Pswd",
	"VMon",  
	"IMon",  
	"Status"
};

/* The list of Board's Parameters for SY403 */
static const char         *BdPar[] = {
	"HVMax"
};

/* For ExecComm routines */
static const char         *execComm[] = { "Kill", "ClearAlarm", "Format" };

/* For SysProp routines */
static const struct sysPropTag 
{
	unsigned num;
	char     *name[6];
	unsigned mode[6];
	unsigned type[6];
	
} sysProp = { 
		6, 
	{	
		"ModelName", "SwRelease", "CnetCrNum", "StatusAlarm", "LockKeyboard", 
		"FrontPanStat" 
	}, 
	{	
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, 
		SYSPROP_MODE_RDWR, SYSPROP_MODE_WRONLY, SYSPROP_MODE_RDONLY
	}, 
	{
		SYSPROP_TYPE_STR, SYSPROP_TYPE_STR, SYSPROP_TYPE_UINT2, 
		SYSPROP_TYPE_UINT2, SYSPROP_TYPE_BOOLEAN, SYSPROP_TYPE_UINT2
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

  Swap

    --------------------------------------------------------------------***/
static void swap(char *a, char *b)
{
char temp;

temp = *a;
*a = *b;
*b = temp;
}

/***------------------------------------------------------------------------

  Swap_Byte

    --------------------------------------------------------------------***/
static void swap_byte(char *buff,int size)
{
int  i;

for( i=0 ; i<size ; i += 2 )
    swap(buff+i,buff+i+1);
}

/***------------------------------------------------------------------------

  Swap_Long

    --------------------------------------------------------------------***/
static void swap_long(void *buff)
{
swap((char *)buff,(char *)buff+3);
swap((char *)buff+1,(char *)buff+2);
}

/***------------------------------------------------------------------------

  Size_of_Chset
  We need the true size in bytes as SY403 sends it via CAENET; it is different
  from sizeof(struct hvch)

    --------------------------------------------------------------------***/
static int size_of_chset(void)
{
 struct hvch ch;

 return   sizeof(ch.chname) + sizeof(ch.flag) 
	    + sizeof(ch.i0set)  + sizeof(ch.i1set) + sizeof(ch.rdwn)  
	    + sizeof(ch.rup)    + sizeof(ch.trip)  + sizeof(ch.v0set)  
  		+ sizeof(ch.v1set)  + sizeof(ch.vmax);
}

/***------------------------------------------------------------------------

  Size_of_Chrd
  We need the true size in bytes as SY403 sends it via CAENET; it is different
  from sizeof(struct hvrd)

    --------------------------------------------------------------------***/
static int size_of_chrd(void)
{
 struct hvrd ch;

 return   sizeof(ch.iread) + sizeof(ch.status) + sizeof(ch.vread);
}

/***------------------------------------------------------------------------

  Size_of_Board
  We need the true size in bytes as SY403 sends it via CAENET; it is different
  from sizeof(struct bd)

    --------------------------------------------------------------------***/
static int size_of_board(void)
{
 struct bd b;

 return   sizeof(b.vmax) + sizeof(b.imax) + sizeof(b.resv) + sizeof(b.resi) 
	    + sizeof(b.decv) + sizeof(b.deci) ;
}

/***------------------------------------------------------------------------

  Build_Bd_Info

    --------------------------------------------------------------------***/
static int build_bd_info(int devHandle, int cr, char *cnetbuff)
{
struct bd *pntbd  = &boards[devHandle][cr];
int            i  = sizeof(pntbd->vmax);

swap_byte(cnetbuff, size_of_board());

memcpy(pntbd, cnetbuff, i);
swap_byte((char *)pntbd,sizeof(pntbd->vmax));

memcpy(&pntbd->imax, cnetbuff+i, sizeof(pntbd->imax));
swap_byte((char *)&pntbd->imax,sizeof(pntbd->imax));
i += sizeof(pntbd->imax);

memcpy(&pntbd->resv, cnetbuff+i, sizeof(pntbd->resv));
swap_byte((char *)&pntbd->resv,sizeof(pntbd->resv));
i += sizeof(pntbd->resv);

memcpy(&pntbd->resi, cnetbuff+i, sizeof(pntbd->resi));
swap_byte((char *)&pntbd->resi,sizeof(pntbd->resi));
i += sizeof(pntbd->resi);

memcpy(&pntbd->decv, cnetbuff+i, sizeof(pntbd->decv));
swap_byte((char *)&pntbd->decv,sizeof(pntbd->decv));
i += sizeof(pntbd->decv);

memcpy(&pntbd->deci, cnetbuff+i, sizeof(pntbd->deci));
swap_byte((char *)&pntbd->deci,sizeof(pntbd->deci));
i += sizeof(pntbd->deci);

return TUTTOK;
}

/***------------------------------------------------------------------------

  Build_Chset_Info

    --------------------------------------------------------------------***/
static void build_chset_info(struct hvch *ch, char *cnetbuff)
{
int i = sizeof(ch->chname);

swap_byte(cnetbuff, size_of_chset());

memcpy(&ch->chname, cnetbuff, i);

memcpy(&ch->v0set,cnetbuff+i, sizeof(ch->v0set));
swap_long(&ch->v0set);
i += sizeof(ch->v0set);

memcpy(&ch->v1set,cnetbuff+i, sizeof(ch->v1set));
swap_long(&ch->v1set);
i += sizeof(ch->v1set);

memcpy(&ch->i0set,cnetbuff+i, sizeof(ch->i0set));
swap_byte((char *)&ch->i0set, sizeof(ch->i0set));
i += sizeof(ch->i0set);

memcpy(&ch->i1set,cnetbuff+i, sizeof(ch->i1set));
swap_byte((char *)&ch->i1set, sizeof(ch->i1set));
i += sizeof(ch->i1set);

memcpy(&ch->vmax,cnetbuff+i, sizeof(ch->vmax));
swap_byte((char *)&ch->vmax, sizeof(ch->vmax));
i += sizeof(ch->vmax);

memcpy(&ch->rup,cnetbuff+i, sizeof(ch->rup));
swap_byte((char *)&ch->rup, sizeof(ch->rup));
i += sizeof(ch->rup);

memcpy(&ch->rdwn,cnetbuff+i, sizeof(ch->rdwn));
swap_byte((char *)&ch->rdwn, sizeof(ch->rdwn));
i += sizeof(ch->rdwn);

memcpy(&ch->trip,cnetbuff+i, sizeof(ch->trip));
swap_byte((char *)&ch->trip, sizeof(ch->trip));
i += sizeof(ch->trip);

memcpy(&ch->flag,cnetbuff+i, sizeof(ch->flag));
swap_byte((char *)&ch->flag, sizeof(ch->flag));
i += sizeof(ch->flag);
}

/***------------------------------------------------------------------------

  Build_Chrd_Info

    --------------------------------------------------------------------***/
static void build_chrd_info(struct hvrd *ch, char *cnetbuff)
{
int i = sizeof(ch->vread);

swap_byte(cnetbuff, size_of_chrd());

memcpy(&ch->vread, cnetbuff, sizeof(ch->vread));
swap_long(&ch->vread);

memcpy(&ch->iread, cnetbuff+i, sizeof(ch->iread));
swap_byte((char *)&ch->iread, sizeof(ch->iread));
i += sizeof(ch->iread);

memcpy(&ch->status, cnetbuff+i, sizeof(ch->status));
swap_byte((char *)&ch->status, sizeof(ch->status));
i += sizeof(ch->status);
}

/***------------------------------------------------------------------------

  Get_Cr_Info
  
	Rel 2.10 - Modified for the elimination of the cratenum and 
			   code global variables.
    --------------------------------------------------------------------***/
static int get_cr_info(int devHandle, int crateNum)
{
	int   bd, response;
	char  cnetbuff[MAX_LENGTH_FIFO];
	int   code;

	code = READ_BOARDS_INFO;
	if( ( response = caenet_comm(devHandle,crateNum,code,NULL,0,cnetbuff) ) != TUTTOK )
	{
		for( bd = 0 ; bd < 4 ; bd++ )
			boards[devHandle][crateNum].present[bd] = 0;
		return response;
	}
	else
	{
		struct hvrd ch_mon;

		build_bd_info(devHandle, crateNum, cnetbuff);
		for( bd = 0 ; bd < 4 ; bd++ )
		{
			code = ( (bd*16) << 8 ) | READ_STATUS;
			response = caenet_comm(devHandle,crateNum,code,NULL,0,cnetbuff);
			build_chrd_info(&ch_mon, cnetbuff);
			if( ch_mon.status & ( 1 << 2 ) )
				boards[devHandle][crateNum].present[bd] = 1;
			else
				boards[devHandle][crateNum].present[bd] = 0;
		}
	}

	return TUTTOK;
}

/***------------------------------------------------------------------------

  Sy403SysInfo
  This procedure refreshes values of the internal data structures

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
static int Sy403SysInfo(int devHandle, int crNum)
{

	errors[devHandle][crNum] = get_cr_info(devHandle, crNum);  
	return errors[devHandle][crNum];
}

/***------------------------------------------------------------------------

  Sy403GetChName

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME])
{
	int                 i, code;
	char                cnetbuff[MAX_LENGTH_FIFO];
/*	
	Rel. 2.10 - static creates problems with threads

	static struct hvch  ch_set;                       // Settings of the channel 
*/
	struct hvch  ch_set;                       /* Settings of the channel */
    CAENHVRESULT        res = CAENHV_OK;

	if( slot >= 4 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !boards[devHandle][crNum].present[slot] )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( ChNum > 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		for( i = 0 ; i < ChNum ; i++ )
		{
			if( ChList[i] >= 16 )
			{
				res = CAENHV_OUTOFRANGE;
				break;
			}
			else
			{
				code = ( (slot*16+ChList[i]) << 8 ) | READ_SETTINGS;

				errors[devHandle][crNum] = 
					caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
				if( errors[devHandle][crNum] == TUTTOK )
				{
					build_chset_info(&ch_set, cnetbuff);
					strncpy(ChNameList[i], ch_set.chname, MAX_CH_NAME);
				}
				else
				{
					res = CAENHV_READERR;
					break;
				}
			}
		}
	}
		
	return res;
}

/***------------------------------------------------------------------------

  Sy403SetChName

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403SetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName)
{
	int                 i, len, code;
	char                cnet_buff[20];
	char				*p = cnet_buff;
    CAENHVRESULT        res = CAENHV_OK;

	if( slot >= 4 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !boards[devHandle][crNum].present[slot] )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( ChNum > 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		for( len = 0 ; ChName[len] ; len++ )
			p[len] = ChName[len];
		p[len] = '\0';

		swap_byte(p, strlen(p)+1);

		len = len + 1 + (len + 1)%2;

		for( i = 0 ; i < ChNum ; i++ )
		{
			if( ChList[i] >= 16 )
			{
				res = CAENHV_OUTOFRANGE;
				break;
			}
			else
			{
				code = ( (slot*16+ChList[i]) << 8 ) | SET_CH_NAME;

				errors[devHandle][crNum] = 
					caenet_comm(devHandle, crNum,code, cnet_buff, len , NULL);
				if( errors[devHandle][crNum] != TUTTOK )
				{
					res = CAENHV_WRITEERR;
					break;
				}
				Sleep(200L);
			}
		}
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetChParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetChParamInfo(int devHandle, int crNum, ushort slot, ushort Ch, 
								 char **ParNameList)
{
	int          i;
	CAENHVRESULT res;

	if( slot >= 4 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !boards[devHandle][crNum].present[slot] )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( Ch >= 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		char (*p)[MAX_PARAM_NAME] = malloc((16+1)*MAX_PARAM_NAME);

		for( i = 0 ; i < 16 ; i++ )
			strcpy(p[i], ChPar[i]);

		strcpy(p[i],"");
		*ParNameList = (char *)p;
		
		res = CAENHV_OK;
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetChParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetChParamProp(int devHandle, int crNum, ushort slot, ushort Ch, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	CAENHVRESULT  res = CAENHV_OK;
	
	if( slot >= 4 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !boards[devHandle][crNum].present[slot] )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( Ch >= 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		float imax, vmax, deci, rmin, rmax;
		short iumis;

		vmax  = (float)boards[devHandle][crNum].vmax[slot];
		deci  = pow_10[boards[devHandle][crNum].deci[slot]];
		imax  = (float)(boards[devHandle][crNum].imax[slot]);
		iumis = ( deci == 1.0 ) ? -3 : -6;
		rmin  = 1.0;
		rmax  = 999.0;

		if( !strcmp( ParName, "V0Set" ) || !strcmp( ParName, "V1Set" ) )
		{
			if( !strcmp(PropName,"Type") )										
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = vmax; 
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
				*(float *)retval = pow_01[boards[devHandle][crNum].deci[slot]];
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = imax;
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_AMPERE;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = iumis;
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
				*(float *)retval = rmin;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = rmax;
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
				*(float *)retval = 1000.0;
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_SECOND;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "SVMax" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = vmax;
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_VOLT;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = 1;
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "POn" ) || !strcmp( ParName, "Pw" ) )
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
		else if( !strcmp( ParName, "PEn" ) || !strcmp( ParName, "Pswd" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_ONOFF;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Onstate") )
				strcpy((char *)retval, "En");
			else if( !strcmp(PropName,"Offstate") )				
				strcpy((char *)retval, "Dis");
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "PDwn" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_ONOFF;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDWR;
			else if( !strcmp(PropName,"Onstate") )
				strcpy((char *)retval, "Ramp");
			else if( !strcmp(PropName,"Offstate") )				
				strcpy((char *)retval, "Kill");
			else
				res = CAENHV_PARAMPROPNOTFOUND;
		}
		else if( !strcmp( ParName, "VMon" ) )
		{
			if( !strcmp(PropName,"Type") )
				*(ulong *)retval = PARAM_TYPE_NUMERIC;
			else if( !strcmp(PropName,"Mode") )
				*(ulong *)retval = PARAM_MODE_RDONLY;
			else if( !strcmp(PropName,"Minval") )
				*(float *)retval = 0.0;
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = vmax;
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
				*(float *)retval = pow_01[boards[devHandle][crNum].deci[slot]];
			else if( !strcmp(PropName,"Maxval") )
				*(float *)retval = imax;
			else if( !strcmp(PropName,"Unit") )
				*(ushort *)retval = PARAM_UN_AMPERE;
			else if( !strcmp(PropName,"Exp") )
				*(short *)retval = iumis;
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

  Sy403GetChParam

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
							 ushort ChNum, const ushort *ChList, void *ParValList)
{
	int                 i, par, set, code;
	char                cnetbuff[MAX_LENGTH_FIFO];
/*	
	Rel. 2.10 - static creates problems with threads

	static struct hvch  ch_set;                       // Settings of the channel
	static struct hvrd  ch_mon;			  // Monitorings of the channel 
*/
	struct hvch  ch_set;                              /* Settings of the channel */
	struct hvrd  ch_mon;				  /* Monitorings of the channel */
    CAENHVRESULT        res = CAENHV_OK;

	if( slot >= 4 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !boards[devHandle][crNum].present[slot] )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( ChNum > 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		if( !strcmp(ParName, "V0Set") )
		{
			par = PAR_V0S;
			set = 1;
		}
		else if( !strcmp(ParName, "V1Set") )
		{
			par = PAR_V1S;
			set = 1;
		}
		else if( !strcmp(ParName, "I0Set") )
		{
			par = PAR_I0S;
			set = 1;
		}
		else if( !strcmp(ParName, "I1Set") )
		{
			par = PAR_I1S;
			set = 1;
		}
		else if( !strcmp(ParName, "RUp") )
		{
			par = PAR_RUP;
			set = 1;
		}
		else if( !strcmp(ParName, "RDwn") )
		{
			par = PAR_RDW;
			set = 1;
		}
		else if( !strcmp(ParName, "Trip") )
		{
			par = PAR_TRP;
			set = 1;
		}
		else if( !strcmp(ParName, "SVMax") )
		{
			par = PAR_VMX;
			set = 1;
		}
		else if( !strcmp(ParName, "VMon") )
		{
			par = PAR_VMN;
			set = 0;
		}
		else if( !strcmp(ParName, "POn") )
		{
			par = PAR_PON;
			set = 1;
		}
		else if( !strcmp(ParName, "Pw") )
		{
			par = PAR_PW;
			set = 1;
		}
		else if( !strcmp(ParName, "PEn") )
		{
			par = PAR_PEN;
			set = 1;
		}
		else if( !strcmp(ParName, "PDwn") )
		{
			par = PAR_PDN;
			set = 1;
		}
		else if( !strcmp(ParName, "Pswd") )
		{
			par = PAR_PSW;
			set = 1;
		}
		else if( !strcmp(ParName, "IMon") )
		{
			par = PAR_IMN;
			set = 0;
		}
		else if( !strcmp(ParName, "Status") )
		{
			par = PAR_STS;
			set = 0;
		}
		else
			par = PAR_NON;
	
		if( par == PAR_NON )
			res = CAENHV_PARAMNOTFOUND;
		else
		{
			short  decv = boards[devHandle][crNum].decv[slot];
			short  deci = boards[devHandle][crNum].deci[slot];

			for( i = 0 ; i < ChNum ; i++ )
			{
				if( ChList[i] >= 16 )
				{
					res = CAENHV_OUTOFRANGE;
					break;
				}
				else
				{
					if( set )
						code = ( (slot*16+ChList[i]) << 8 ) | READ_SETTINGS;
					else
						code = ( (slot*16+ChList[i]) << 8 ) | READ_STATUS;

					errors[devHandle][crNum] = 
						caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
					if( errors[devHandle][crNum] == TUTTOK )
					{	
						if( set )
							build_chset_info(&ch_set, cnetbuff);
						else
							build_chrd_info(&ch_mon, cnetbuff);
					}
					else
					{
						res = CAENHV_READERR;
						break;
					}

					switch( par )									
					{
						case PAR_V0S:
							*((float *)ParValList + i) = 
							(float)ch_set.v0set/(float)pow_10[decv];
							break;

						case PAR_I0S:
							*((float *)ParValList + i) = 
							  (float)ch_set.i0set/(float)pow_10[deci];
							break;
							
						case PAR_V1S:
							*((float *)ParValList + i) = 
							  (float)ch_set.v1set/(float)pow_10[decv];
							break;
			
						case PAR_I1S:
							*((float *)ParValList + i) = 
							  (float)ch_set.i1set/(float)pow_10[deci];
							break;
			
						case PAR_RUP:
							*((float *)ParValList + i) = (float)ch_set.rup;
							break;
			
						case PAR_RDW:
							*((float *)ParValList + i) = (float)ch_set.rdwn;
							break;
			
						case PAR_TRP:
							*((float *)ParValList + i) = (float)ch_set.trip/(float)10.0;
							break;
			
						case PAR_VMX:
							*((float *)ParValList + i) = (float)ch_set.vmax;
							break;
			
						case PAR_VMN:
							*((float *)ParValList + i) = 
							 (float)ch_mon.vread/(float)pow_10[decv];
							break;
			
						case PAR_IMN:
							*((float *)ParValList + i) = 
							 (float)ch_mon.iread/(float)pow_10[deci];
							break;
			
						case PAR_STS:
							*((unsigned long *)ParValList + i) = ch_mon.status;
							break;

						case PAR_PON:
							*((unsigned long *)ParValList + i) = 
							 ( ch_set.flag & (1<<0xf) ) >> 15;
							break;

						case PAR_PW:
							*((unsigned long *)ParValList + i) = 
							 ( ch_set.flag & (1<<0xb) ) >> 11;
							break;

						case PAR_PEN:
							*((unsigned long *)ParValList + i) = 
							 ( ch_set.flag & (1<<0xe) ) >> 14;
							break;

						case PAR_PDN:
							*((unsigned long *)ParValList + i) = 
							 ( ch_set.flag & (1<<0xd) ) >> 13;
							break;

						case PAR_PSW:
							*((unsigned long *)ParValList + i) = 
							( ch_set.flag & (1<<0xc) ) >> 12;
							break;
					}
				}
			}
		}
	}
 
	return res;
}

/***------------------------------------------------------------------------

  Sy403SetChParam

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403SetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
						     ushort ChNum, const ushort *ChList, void *ParValue)
{
	int                 i, par, code;
	ushort              cnet_buff;
    CAENHVRESULT        res = CAENHV_OK;

	if( slot >= 4 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !boards[devHandle][crNum].present[slot] )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( ChNum > 16 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
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
		else if( !strcmp(ParName, "SVMax") )
			par = PAR_VMX;
		else if( !strcmp(ParName, "POn") )
			par = PAR_PON;
		else if( !strcmp(ParName, "Pw") )
			par = PAR_PW;
		else if( !strcmp(ParName, "PEn") )
			par = PAR_PEN;
		else if( !strcmp(ParName, "PDwn") )
			par = PAR_PDN;
		else if( !strcmp(ParName, "Pswd") )
			par = PAR_PSW;
		else
			par = PAR_NON;

		if( par == PAR_NON )
			res = CAENHV_PARAMNOTFOUND;
		else
		{
			float  scalev = pow_10[boards[devHandle][crNum].decv[slot]];
			float  scalei = pow_10[boards[devHandle][crNum].deci[slot]];

			for( i = 0 ; i < ChNum ; i++ )
			{
				if( ChList[i] >= 16 )
				{
					res = CAENHV_OUTOFRANGE;
					break;
				}
				else
				{
					switch( par )
					{							
						case PAR_V0S:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_V0SET;
							cnet_buff = (ushort)(*(float *)ParValue*scalev + 0.5);
							break;
						case PAR_V1S:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_V1SET;
							cnet_buff = (ushort)(*(float *)ParValue*scalev + 0.5);
							break;
						case PAR_I0S:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_I0SET;
							cnet_buff = (ushort)(*(float *)ParValue*scalei + 0.5);
							break;
						case PAR_I1S:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_I1SET;
							cnet_buff = (ushort)(*(float *)ParValue*scalei + 0.5);
							break;
						case PAR_VMX:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_VMAX;
							cnet_buff = (ushort)(*(float *)ParValue);
							break;
						case PAR_RUP:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_RUP;
							cnet_buff = (ushort)(*(float *)ParValue);
							break;
						case PAR_RDW:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_RDWN;
							cnet_buff = (ushort)(*(float *)ParValue);
							break;
						case PAR_TRP:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_TRIP;
							cnet_buff = (ushort)(*(float *)ParValue*10.0 + 0.5);
							break;
						case PAR_PON:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_FLAG;
							cnet_buff = (ushort)(( *(unsigned *)ParValue << 7 ) | (1<<0xf) );
							break;
						case PAR_PW:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_FLAG;
							cnet_buff = (ushort)(( *(unsigned *)ParValue << 3 ) | (1<<0xb) );
							break;
						case PAR_PEN:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_FLAG;
							cnet_buff = (ushort)(( *(unsigned *)ParValue << 6 ) | (1<<0xe) );
							break;
						case PAR_PDN:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_FLAG;
							cnet_buff = (ushort)(( *(unsigned *)ParValue << 5 ) | (1<<0xd) );
							break;
						case PAR_PSW:
							code      = ( (slot*16+ChList[i]) << 8 ) | SET_FLAG;
							cnet_buff = (ushort)(( *(unsigned *)ParValue << 4 ) | (1<<0xc) );
							break;
					}
			
					errors[devHandle][crNum] = 
							caenet_comm(devHandle, crNum, code, &cnet_buff, sizeof(cnet_buff), NULL);
					if( errors[devHandle][crNum] != TUTTOK )
					{
						res = CAENHV_WRITEERR;
						break;
					}
					Sleep(200L);
				}
			}
		}
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy403TestBdPresence

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403TestBdPresence(int devHandle, int crNum, ushort slot, char *Model, 
								 char *Description, ushort *SerNum, 
								 uchar *FmwRelMin, uchar *FmwRelMax)
{
	CAENHVRESULT res;

	if( ( res = Sy403SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		if( boards[devHandle][crNum].present[slot] )
		{
			short vmax;
			float imax, deci;

			vmax  = boards[devHandle][crNum].vmax[slot];
			deci  = pow_10[boards[devHandle][crNum].deci[slot]];
			imax  = (float)(boards[devHandle][crNum].imax[slot])/deci;

			if( vmax == 3000 && imax < 1000.0 )
			{
				strcpy(Description, "  16 CH   3000V   200.00uA    ");
				strcpy(Model, "A505");
			}
			else if( vmax == 600 )
			{
				strcpy(Description, "  16 CH    600V   200.00uA    ");
				strcpy(Model, "A504");
			}
			else
			{
				strcpy(Description, "  16 CH   3000V  3000.00uA    ");
				strcpy(Model, "A503");
			}
			*SerNum = 0;
			*FmwRelMin = 0;
			*FmwRelMax = 0;
		}
		else
			res = CAENHV_SLOTNOTPRES;
   }

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetBdParamInfo

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetBdParamInfo(int devHandle, int crNum, ushort slot, char **ParNameList)
{
	int i;

	char (*p)[MAX_PARAM_NAME] = malloc((1+1)*MAX_PARAM_NAME);

	for( i = 0 ; i < 1 ; i++ )
		strcpy(p[i], BdPar[i]);

	strcpy(p[i],"");
	*ParNameList = (char *)p;
		
	return CAENHV_OK;
}

/***------------------------------------------------------------------------

  Sy403GetBdParamProp

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetBdParamProp(int devHandle, int crNum, ushort slot, 
								 const char *ParName, const char *PropName, void *retval)
{
	CAENHVRESULT  res = CAENHV_OK;
	
	if( !strcmp( ParName, "HVMax" ) )
	{
		if( !strcmp(PropName,"Type") )
			*(ulong *)retval = PARAM_TYPE_NUMERIC;
		else if( !strcmp(PropName,"Mode") )
			*(ulong *)retval = PARAM_MODE_RDONLY;
		else if( !strcmp(PropName,"Minval") )
			*(float *)retval = 0.0;
		else if( !strcmp(PropName,"Maxval") )
			*(float *)retval = (float)boards[devHandle][crNum].vmax[slot];
		else if( !strcmp(PropName,"Unit") )
			*(ushort *)retval = PARAM_UN_VOLT;
		else if( !strcmp(PropName,"Exp") )
			*(short *)retval = 1;
		else
			res = CAENHV_PARAMPROPNOTFOUND;
	}
	else
		res = CAENHV_PARAMNOTFOUND;

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetBdParam

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList,
							 const char *ParName, void *ParValList)
{
	int                 i, code;
	char                cnetbuff[MAX_LENGTH_FIFO];
	ushort              hvmax[4];
    CAENHVRESULT        res = CAENHV_OK;

	if( strcmp(ParName, "HVMax") )
		res = CAENHV_PARAMNOTFOUND;
	else if( slotNum > 4 )
		res = CAENHV_SLOTNOTPRES;
	else
	{
		code = READ_HVMAX;

		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			memcpy(hvmax, cnetbuff, sizeof(hvmax));

			for( i = 0 ; i < slotNum ; i++ )
			{
				if( slotList[i] >= 4 )
				{
					res = CAENHV_SLOTNOTPRES;
					break;
				}
				else if( boards[devHandle][crNum].present[slotList[i]] == 0 )
				{
					res = CAENHV_SLOTNOTPRES;
					break;
				}
				else
					*((float *)ParValList + i) = (float)hvmax[slotList[i]];
			}
		}
		else
			res = CAENHV_READERR;
	}
	
	return res;
}

/***------------------------------------------------------------------------

  Sy403SetBdParam

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403SetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, 
							 const char *ParName, void *ParValue)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  Sy403GetSysComp

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetSysComp(int devHandle, int crNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList)
{
	CAENHVRESULT  res;

	if( ( res = Sy403SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		int    i;
		ushort *app1;
		ulong  *app2;

		*NrOfCr = 1;
		*CrNrOfSlList = malloc(sizeof(ushort));
		*SlotChList   = malloc(4*sizeof(ulong));

		app1 = *CrNrOfSlList;
		app2 = *SlotChList;

		*app1 = 4;

		for( i = 0 ; i < 4 ; i++ )
			if( boards[devHandle][crNum].present[i] )
				app2[i] = ( i << 0x10 ) | 0x10;
			else
				app2[i] = ( i << 0x10 );
	}
	else
		res = CAENHV_READERR;

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetCrateMap

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetCrateMap(int devHandle, int crNum, ushort *NrOfSlot, 
							  char **ModelList, 
							  char **DescriptionList, ushort **SerNumList, 
							  uchar **FmwRelMinList, uchar **FmwRelMaxList)
{
	CAENHVRESULT  res;

	if( ( res = Sy403SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		char			BdName[10], Desc[80]  = "  16 CH   3000V  3000.00uA    ", 
			            *app1, *app2;
		ushort			i, sl     = 4, *app3, modlen = 0, desclen = 0;

		*ModelList       = malloc(sl*10);					/* Max among 'boards[bd].name's */
		*DescriptionList = malloc(sl*(strlen(Desc)+1));
		*SerNumList      = malloc(sl*sizeof(ushort));
		*FmwRelMinList   = malloc(sl*sizeof(uchar));
		*FmwRelMaxList   = malloc(sl*sizeof(uchar));
		*NrOfSlot = sl;

		app1 = *FmwRelMinList, app2 = *FmwRelMaxList, app3 = *SerNumList;

		for( i = 0 ; i < sl ; i++ )
			if( boards[devHandle][crNum].present[i] )
			{
				short vmax;
				float imax, deci;

				vmax  = boards[devHandle][crNum].vmax[i];
				deci  = pow_10[boards[devHandle][crNum].deci[i]];
				imax  = (float)(boards[devHandle][crNum].imax[i])/deci;

				if( vmax == 3000 && imax < 1000.0 )
				{
					strcpy(Desc, "  16 CH   3000V   200.00uA    ");
					strcpy(BdName, "A505");
				}
				else if( vmax == 600 )
				{
					strcpy(Desc, "  16 CH    600V   200.00uA    ");
					strcpy(BdName, "A504");
				}
				else
				{
					strcpy(Desc, "  16 CH   3000V  3000.00uA    ");
					strcpy(BdName, "A503");
				}

				memcpy(*ModelList+modlen, BdName, 6);
				*(*ModelList+modlen + 4) = '\0';
				memcpy(*DescriptionList+desclen, Desc, strlen(Desc)+1);
				app3[i]  = 0;
			    app1[i]  = 0;
			    app2[i]  = 0;
				modlen  += strlen(*ModelList+modlen)+1;
				desclen += strlen(Desc)+1;
			}
			else
			{
				memcpy(*ModelList+modlen, "", strlen("")+1);
				modlen  += strlen("")+1;
				memcpy(*DescriptionList+desclen, "", strlen("")+1);
				desclen  += strlen("")+1;
			}
	}
	else
		res = CAENHV_READERR;

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetExecCommList

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetExecCommList(int devHandle, int crNum, ushort *NumComm, 
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

  Sy403ExecComm

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403ExecComm(int devHandle, int crNum, const char *CommName)
{
	CAENHVRESULT res = CAENHV_OK;
	int code;

	if( !strcmp(CommName, execComm[0]) )		/* Kill */
	{
		code = KILL_CHANNELS_1;
		errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			code = KILL_CHANNELS_2;
			errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
			if( errors[devHandle][crNum] != TUTTOK )
				res = CAENHV_WRITEERR;
		}
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[1]) )	/* Clear Alarm  */
	{
		code = CLEAR_ALARM;
		errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
		if( errors[devHandle][crNum] != TUTTOK )
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(CommName, execComm[2]) )	/* Format */
	{
		code = FORMAT_EEPROM_1;
		errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			code = FORMAT_EEPROM_2;
			errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,NULL);
			if( errors[devHandle][crNum] != TUTTOK )
				res = CAENHV_WRITEERR;
		}
		else
			res = CAENHV_WRITEERR;
	}
	else
		res = CAENHV_EXECNOTFOUND;

	return res;
}

/***------------------------------------------------------------------------

  Sy403GetSysPropList

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetSysPropList(int devHandle, int crNum, ushort *NumProp, 
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

  Sy403GetSysPropInfo

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetSysPropInfo(int devHandle, int crNum, const char *PropName, 
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

  Sy403GetSysProp

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403GetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Result)
{
	int          i, code;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		  /* ModelName */
	{
		strcpy((char *)Result, "SY403");
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
			for( i = 14 ; tempbuff[i] != '\0' ; i+=2 )
				syident[i/2-7] = tempbuff[i];
			syident[i/2-7] = '\0';
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
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* StatusAlarm */
	{
		ushort   value[2];

		code    = READ_GEN_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, value);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = value[0];
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* LockKeyboard */
  {
    *(unsigned *)Result = 0;
		res = CAENHV_OK;                              /* Like ExecComms */
  }
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* FrontPanStat */
	{
		ushort   value[2];

		code    = READ_GEN_STATUS;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, value);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = value[1]&0x1f;
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy403SetSysProp

  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy403SetSysProp(int devHandle, int crNum, const char *PropName, void *Set)
{
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;
	int          code;

	if( !strcmp(PropName, sysProp.name[0]) )		  /* ModelName */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[2]) )	  /* CnetCrNum */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* StatusAlarm */
	{
		ushort   value;

		code    = SET_STATUS_ALARM;
		value = *(ushort *)Set;
		errors[devHandle][crNum] = 
			caenet_comm(devHandle, crNum, code, &value, sizeof(value), NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* LockKeyboard */
	{
		unsigned lock;

		lock    = *(unsigned *)Set;
		code    = ( lock ? LOCK_KEYBOARD : UNLOCK_KEYBOARD );
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, NULL);
		if( errors[devHandle][crNum] == TUTTOK )
			res = CAENHV_OK;
		else
			res = CAENHV_WRITEERR;
	}
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* FrontPanStat */
		res = CAENHV_NOTSETPROP;

	return res;
}

/***------------------------------------------------------------------------

  Sy403Init
  This is the entry point into the routines of this file: it is explicitely
  called when the user calls CAENHVInitSystem()

    --------------------------------------------------------------------***/
int Sy403Init(int devHandle, int crNum)
{
	return Sy403SysInfo(devHandle, crNum);
}

/***------------------------------------------------------------------------

  Sy403End

    --------------------------------------------------------------------***/
void Sy403End(int devHandle, int crNum)
{
}

/***------------------------------------------------------------------------

  Sy403GetError

    --------------------------------------------------------------------***/
char *Sy403GetError(int devHandle, int crNum)
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
