/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY127INTERFACE.C                                        		       */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    December 2001    : Created                                           */
/*    April    2002    : Rel. 2.1 - Added the devHandle param              */
/*                                                                         */
/***************************************************************************/
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#ifdef UNIX
#include  <errno.h>			                /* Rel. 2.0 - Linux */
#endif
#include  "HSCAENETLib.h"	                /* Rel. 2.1 */
#include  "CAENHVWrapper.h"
#include  "sy127interface.h"

#define   MAKE_CODE(ch,cod)      (((ch)<<8) | (cod))

#define   MAX_NR_OF_CARDS     10			/* Rel. 2.1 - It seems more than enough */

/*
  Some of the Caenet Codes
*/
#define   IDENT                                  0x0
#define   READ_CH                                0x1
#define   CRATE_MAP                              0x3
#define   READ_PROT                              0x4

#define   SET_V0SET 	                          0x10
#define   SET_V1SET 	                          0x11
#define   SET_I0SET 	                          0x12
#define   SET_I1SET 	                          0x13
#define   SET_RUP    	                          0x15
#define   SET_RDWN    	                        0x16
#define   SET_TRIP    	                        0x17
#define   SET_FLAG    	                        0x18   /* Analog to sy527, but here to On/Off only */
#define   SET_CH_NAME	                          0x19

#define   FORMAT_EEPROM_1                       0x30
#define   FORMAT_EEPROM_2                       0x31
#define   CLEAR_ALARM                           0x32
#define   SET_PROT                              0x39

#define   PAR_V0S	  					 1
#define   PAR_I0S						   2
#define   PAR_V1S						   3
#define   PAR_I1S						   4
#define   PAR_RUP						   5
#define   PAR_RDW						   6
#define   PAR_TRP						   7
#define   PAR_PW						   9
#define   PAR_VMN						  10
#define   PAR_IMN						  11
#define   PAR_STS						  12
#define   PAR_NON						  13


/*
  The following structure contains all the useful information about
  the settings and monitorings of a channel
*/
struct hvch
{
 ushort  v0set;
 ushort  v1set;
 ushort  i0set;
 ushort  i1set;
 ushort  rup;
 ushort  rdwn;
 ushort  trip;
 ushort  status;
 ushort  gr_ass;
 ushort  vread;
 ushort  iread;
 ushort  st_phase;
 ushort  st_time;
 ushort  mod_type;
 char    chname[10];
};

/*
  The following structure contains all the useful information about
  description and voltage and current resolution for any kind
  of board of SY127
*/
struct bdid_tag
{
  char     id;
  unsigned pos;
  unsigned present;
  unsigned vmax, imax, vres, ires;
};

typedef struct bdid_tag    (*BdID)[10];

/*
  Globals
  Rel. 2.10 - Added const to constant global variables.
            - Deleted the code and cratenum global variables
			  due to interfernce between thread.
*/
/*static int            cratenum, code; */

/* 
   Rel. 2.01 - Now we handle more than one card for PC
   Rel. 2.10 - The interferences between threads are avoided 
			   because the same values are written in the same place.
*/
static BdID           BoardID[MAX_NR_OF_CARDS][100];

static int            errors[MAX_NR_OF_CARDS][100];		/* The last error code */

struct mt_tag 
{
  char     name[8];
  char     desc[20];
  int      IuMis;
  unsigned numch;      
  unsigned vmax, imax, vres, ires, rmax;
};

/* Rel. 2.10 - Added static and const */
static const struct mt_tag modul_type[] =         /* Modules types */
{
  {
    "", " Not Present     ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "A334", " 2KV    3mA      ", -6, 4, 2000, 3000, 2, 1, 250
  },
  {
    "A333", " 3KV    3mA      ", -6, 4, 3000, 3000, 1, 1, 500
  },
  {
    "A333", " 4KV    2mA      ", -6, 4, 4000, 2000, 1, 1, 500
  },
  {
    "A331", " 8KV    500uA    ", -6, 4, 8000, 500, 1, 1, 500
  },
  {
    "A332", " 6KV    1mA      ", -6, 4, 6000, 1000, 1, 1, 500
  },
  {
    "A335", " 800.0V 500.0uA  ", -6, 4, 800, 500, 10, 10, 50
  },
  {
    "A431", " 8KV    200.0uA  ", -6, 4, 8000, 200, 1, 10, 500
  },
  {
    "A432", " 6KV    200.0uA  ", -6, 4, 6000, 200, 1, 10, 500
  },
  {
    "A435", " 200.0V 200.0uA  ", -6, 4, 200, 200, 10, 10, 25
  },
  {
    "A434", " 2KV    200.0uA  ", -6, 4, 2000, 200, 2, 10, 250
  },
  {
    "A433", " 4KV    200.0uA  ", -6, 4, 4000, 200, 1, 10, 500
  },
  {
    "A332", " 6KV    1mA      ", -6, 4, 6000, 1000, 1, 1, 500
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "A333", " 3KV    3mA      ", -6, 4, 3000, 3000, 1, 1, 500
  },
  {
    "A333", " 4KV    2mA      ", -6, 4, 4000, 2000, 1, 1, 500
  },
  {
    "A436", " 800.0V 200.0uA  ", -6, 4, 800, 200, 10, 10, 50
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "A431", " 8KV    200.0uA  ", -6, 4, 8000, 200, 1, 10, 500
  },
  {
    "A330", " 10KV   1mA      ", -6, 4, 10000, 1000, 1, 1, 500
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "A430", " 10KV   200.0uA  ", -6, 4, 10000, 200, 1, 10, 500
  },
  {
    "A429", " 15KV   200.0uA  ", -6, 4, 15000, 200, 1, 10, 500
  },
  {
    "A329", " 15KV   1mA      ", -6, 4, 15000, 1000, 1, 1, 500
  },
  {
    "A426", " 20KV   200.0uA  ", -6, 4, 20000, 200, 1, 10, 500
  },
  {
    "A326", " 2.5KV  5mA      ", -6, 4, 2500, 5000, 1, 1, 500
  },
  {
    "A327", " 1KV    10mA     ", -3, 4, 1000, 10000, 4, 1, 125
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "A325", " 20KV   500uA    ", -6, 4, 20000, 500, 1, 1, 500
  },
  {
    "A328", " 10KV   2mA      ", -3, 4, 10000, 2000, 1, 1, 500
  },
  {
    "A230", " I/O Module      ", 0, 4, 4095, 4095, 1, 1, 500
  },
  {
    "A635", " 200.0V 40uA     ", -6, 4, 200, 40, 10, 100, 25
  },
  {
    "A636", " 800.0V 40uA     ", -6, 4, 800, 40, 10, 100, 50
  },
  {
    "A634", " 2KV    40uA     ", -6, 4, 2000, 40, 2, 100, 250
  },
  {
    "A633", " 4KV    40uA     ", -6, 4, 4000, 40, 1, 100, 500
  },
  {
    "A632", " 6KV    40uA     ", -6, 4, 6000, 40, 1, 100, 500
  },
  {
    "A631", " 8KV    40uA     ", -6, 4, 8000, 40, 1, 100, 500
  },
  {
    "A630", " 10KV   40uA     ", -6, 4, 10000, 40, 1, 100, 500
  },
  {
    "A629", " 15KV   40uA     ", -6, 4, 15000, 40, 1, 100, 500
  },
  {
    "A626", " 20KV   40uA     ", -6, 4, 20000, 40, 1, 100, 500
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Special Module  ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
  {
    "", " Not Implemented ", 0, 0, 0, 0, 0, 0, 0
  },
};

/* The list of Channels' Parameters for SY527 */
static const char *ChPar[] = {
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
	"Status"
};

/* For ExecComm routines */
static const char *execComm[] = { "ClearAlarm", "Format" };

/* For SysProp routines */
static const struct sysPropTag 
{
	unsigned num;
	char     *name[7];
	unsigned mode[7];
	unsigned type[7];
} sysProp = { 
		7, 
	{	
		"ModelName", "SwRelease", "CnetCrNum", "LockKeyboard", 
		"FrontPanStat", "POn", "Pswd" 
	}, 
	{	
		SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDONLY, 
		SYSPROP_MODE_RDWR, SYSPROP_MODE_RDONLY, SYSPROP_MODE_RDWR,
    SYSPROP_MODE_RDWR
	}, 
	{
		SYSPROP_TYPE_STR, SYSPROP_TYPE_STR, SYSPROP_TYPE_UINT2, 
	  SYSPROP_TYPE_BOOLEAN, SYSPROP_TYPE_UINT2, SYSPROP_TYPE_BOOLEAN,
    SYSPROP_TYPE_BOOLEAN
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

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
static int get_cr_info(int devHandle, int crNum)
{
  int  response, code;
  char cmap[10];

  code = CRATE_MAP;
  if( ( response = caenet_comm(devHandle, crNum, code, NULL,0,cmap) ) == TUTTOK )
  {
    int slot;

    for( slot = 0 ; slot < 10 ; slot++ )
    {
      int id = cmap[slot];

      (*BoardID[devHandle][crNum])[slot].pos = ( id > 0 ? 1 : 0 );
      id &= 0x7f;
      if( id > 0x2f || !id )
        (*BoardID[devHandle][crNum])[slot].present = 0;
      else
      {
        (*BoardID[devHandle][crNum])[slot].present = 1;
        (*BoardID[devHandle][crNum])[slot].id = id;
        (*BoardID[devHandle][crNum])[slot].vmax = modul_type[id].vmax;
        (*BoardID[devHandle][crNum])[slot].imax = modul_type[id].imax;
        (*BoardID[devHandle][crNum])[slot].vres = modul_type[id].vres;
        (*BoardID[devHandle][crNum])[slot].ires = modul_type[id].ires;
      }
    }
  }

  return response;
}

/***------------------------------------------------------------------------

  Build_Chrd_Info

    --------------------------------------------------------------------***/
static void build_chrd_info(struct hvch *ch, char *cnetbuff)
{
  int i = sizeof(ch->v0set);

  memcpy(&ch->v0set, cnetbuff,   sizeof(ch->v0set));
  memcpy(&ch->v1set, cnetbuff+i, sizeof(ch->v1set));
  i += sizeof(ch->v1set);
  memcpy(&ch->i0set, cnetbuff+i, sizeof(ch->i0set));
  i += sizeof(ch->i0set);
  memcpy(&ch->i1set, cnetbuff+i, sizeof(ch->i1set));
  i += sizeof(ch->i1set);
  memcpy(&ch->rup, cnetbuff+i, sizeof(ch->rup));
  i += sizeof(ch->rup);
  memcpy(&ch->rdwn, cnetbuff+i, sizeof(ch->rdwn));
  i += sizeof(ch->rdwn);
  memcpy(&ch->trip, cnetbuff+i, sizeof(ch->trip));
  i += sizeof(ch->trip);
  memcpy(&ch->status, cnetbuff+i, sizeof(ch->status));
  i += sizeof(ch->status);
  memcpy(&ch->gr_ass, cnetbuff+i, sizeof(ch->gr_ass));
  i += sizeof(ch->gr_ass);
  memcpy(&ch->vread, cnetbuff+i, sizeof(ch->vread));
  i += sizeof(ch->vread);
  memcpy(&ch->iread, cnetbuff+i, sizeof(ch->iread));
  i += sizeof(ch->iread);
  memcpy(&ch->st_phase, cnetbuff+i, sizeof(ch->st_phase));
  i += sizeof(ch->st_phase);
  memcpy(&ch->st_time, cnetbuff+i, sizeof(ch->st_time));
  i += sizeof(ch->st_time);
  memcpy(&ch->mod_type, cnetbuff+i, sizeof(ch->mod_type));
  i += sizeof(ch->mod_type);
  memcpy(&ch->chname, cnetbuff+i, sizeof(ch->chname));
}

/***------------------------------------------------------------------------

  Sy127SysInfo
  This procedure refreshes values of the internal data structures

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
static int Sy127SysInfo(int devHandle, int crNum)
{

	errors[devHandle][crNum] = get_cr_info(devHandle, crNum);  /* Get info about the Crate Conf. */

	return errors[devHandle][crNum];
}

/***------------------------------------------------------------------------

  Sy127GetChName
  
  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME])
{
	int                 i, code;
	uchar               ch_addr;
	char                cnetbuff[MAX_LENGTH_FIFO];
	struct hvch         chPar;
	CAENHVRESULT        res = CAENHV_OK;

	for( i = 0 ; i < ChNum ; i++ )
	{
		ch_addr = ( slot << 2 ) | ChList[i];

		code = MAKE_CODE(ch_addr, READ_CH);

		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);

		if( errors[devHandle][crNum] == TUTTOK )
		{
			build_chrd_info(&chPar, cnetbuff);
			strncpy(ChNameList[i], chPar.chname, sizeof(chPar.chname));
		}
		else
		{
			res = CAENHV_READERR;
			break;
		}
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy127SetChName

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127SetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName)
{
	int                 i, len, code;
	uchar               ch_addr;
	char                cnet_buff[10];
	CAENHVRESULT        res = CAENHV_OK;

	for( len = 0 ; ChName[len] ; len++ )
		cnet_buff[len] = ChName[len];
	cnet_buff[len] = '\0';

/*	swap_byte(p, strlen(p)+1); */

	len = len + 1 + (len + 1)%2;

	for( i = 0 ; i < ChNum ; i++ )
	{
		ch_addr = ( slot << 2 ) | ChList[i];

		code = MAKE_CODE(ch_addr, SET_CH_NAME);

		errors[devHandle][crNum] = 
			caenet_comm(devHandle, crNum, code, cnet_buff, len , NULL);

		if( errors[devHandle][crNum] != TUTTOK )
		{
			res = CAENHV_WRITEERR;
			break;
		}
		Sleep(200L);
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy127GetChParamInfo
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetChParamInfo(int devHandle, int crNum, ushort slot, ushort Ch, 
								 char **ParNameList)
{
	int				i, 
					id = (*BoardID[devHandle][crNum])[slot].id;
	CAENHVRESULT	res;

	if( slot >= 10 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !(*BoardID[devHandle][crNum])[slot].present )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( Ch >= modul_type[id].numch )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		char (*p)[MAX_PARAM_NAME] = malloc((11+1)*MAX_PARAM_NAME);

		for( i = 0 ; i < 11 ; i++ )
			strcpy(p[i], ChPar[i]);

		strcpy(p[i],"");
		*ParNameList = (char *)p;
		
		res = CAENHV_OK;
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy127GetChParamProp

  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetChParamProp(int devHandle, int crNum, ushort slot, ushort Ch, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	int           id = (*BoardID[devHandle][crNum])[slot].id;
	CAENHVRESULT  res = CAENHV_OK;
	
	if( slot >= 10 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !(*BoardID[devHandle][crNum])[slot].present )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( Ch >= modul_type[id].numch )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		float imax, vmax, rmin, rmax, ires;
		short iumis;
		int   id = (*BoardID[devHandle][crNum])[slot].id;

		vmax  = (float)(*BoardID[devHandle][crNum])[slot].vmax;
		imax  = (float)(*BoardID[devHandle][crNum])[slot].imax;
		ires  = (float)(*BoardID[devHandle][crNum])[slot].ires;

		if(    (*BoardID[devHandle][crNum])[slot].id >= 0x20 
        && (*BoardID[devHandle][crNum])[slot].id <= 0x28 )            /* 40 uA boards */
			imax = 40.0;

		rmin  = 1;
		rmax  = (float)modul_type[id].rmax;
		iumis = modul_type[id].IuMis;

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
				*(float *)retval = (float)0.0;
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
				*(float *)retval = (float)0.0;
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

  Sy127GetChParam

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
							 ushort ChNum, const ushort *ChList, void *ParValList)
{
	int                 i, par, code;
	ushort              ch_addr;
	char                cnetbuff[MAX_LENGTH_FIFO];
	struct hvch         chPar;
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
	else if( !strcmp(ParName, "VMon") )
		par = PAR_VMN;
	else if( !strcmp(ParName, "Pw") )
		par = PAR_PW;
	else if( !strcmp(ParName, "IMon") )
		par = PAR_IMN;
	else if( !strcmp(ParName, "Status") )
		par = PAR_STS;
	else
		par = PAR_NON;

	if( par == PAR_NON )
		res = CAENHV_PARAMNOTFOUND;
	else
	{
		for( i = 0 ; i < ChNum ; i++ )
		{
			ch_addr = ( slot << 2 ) | ChList[i];

			code = MAKE_CODE(ch_addr, READ_CH);

			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, NULL, 0, cnetbuff);

			if( errors[devHandle][crNum] == TUTTOK )
				build_chrd_info(&chPar, cnetbuff);
			else
			{
				res = CAENHV_READERR;
				break;
			}

			switch( par )									
			{
		  	case PAR_V0S:
					*((float *)ParValList + i) = 
  			    (float)chPar.v0set/(float)(*BoardID[devHandle][crNum])[slot].vres;
				  break;

  			case PAR_I0S:
	 				*((float *)ParValList + i) = 
	  			  (float)chPar.i0set/(float)(*BoardID[devHandle][crNum])[slot].ires;
		  		break;
							
			  case PAR_V1S:
 			  	*((float *)ParValList + i) = 
  			    (float)chPar.v1set/(float)(*BoardID[devHandle][crNum])[slot].vres;
				  break;
			
  			case PAR_I1S:
	 				*((float *)ParValList + i) = 
	  			  (float)chPar.i1set/(float)(*BoardID[devHandle][crNum])[slot].ires;
		  		break;
			
  			case PAR_RUP:
	 				*((float *)ParValList + i) = 
            (float)chPar.rup/(float)(*BoardID[devHandle][crNum])[slot].vres;
	  			break;
			
  			case PAR_RDW:
	 				*((float *)ParValList + i) = 
            (float)chPar.rdwn/(float)(*BoardID[devHandle][crNum])[slot].vres;
	  			break;
			
  			case PAR_TRP:
	 				*((float *)ParValList + i) = (float)chPar.trip/(float)10.0;
	  			break;
			
  			case PAR_VMN:
 			  	*((float *)ParValList + i) = 
  			    (float)chPar.vread/(float)(*BoardID[devHandle][crNum])[slot].vres;
				  break;
			
		  	case PAR_IMN:
			  	*((float *)ParValList + i) = 
				   (float)chPar.iread/(float)(*BoardID[devHandle][crNum])[slot].ires;
				  break;
			
			  case PAR_STS:
				  *((unsigned long *)ParValList + i) = chPar.status;
				  break;

 				case PAR_PW:
  				*((unsigned long *)ParValList + i) = ( chPar.status & (1<<2) ) >> 2;
		  		break;
      } /* End switch */
		} /* End for */
	} /* End if-else (par) */
 
	return res;
}

/***------------------------------------------------------------------------

  Sy127SetChParam

  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127SetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
						     ushort ChNum, const ushort *ChList, void *ParValue)
{
	int                 i, par, code;
	ushort              cnet_buff, ch_addr;
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
	else
	{
		for( i = 0 ; i < ChNum ; i++ )
		{
			ch_addr = ( slot << 2 ) | ChList[i];

			switch( par )
			{							
  			case PAR_V0S:
				code = MAKE_CODE(ch_addr, SET_V0SET);
				if( (*BoardID[devHandle][crNum])[slot].vmax == 20000 )
					cnet_buff = (ushort)(*(float *)ParValue/(float)2.0);
				else
					cnet_buff = (ushort)(*(float *)ParValue*(*BoardID[devHandle][crNum])[slot].vres + 0.5);
				break;
  			case PAR_V1S:
          code = MAKE_CODE(ch_addr, SET_V1SET);
          if( (*BoardID[devHandle][crNum])[slot].vmax == 20000 )
    				cnet_buff = (ushort)(*(float *)ParValue/(float)2.0);
          else
            cnet_buff = (ushort)(*(float *)ParValue*(*BoardID[devHandle][crNum])[slot].vres + 0.5);
  				break;
			  case PAR_I0S:
          code = MAKE_CODE(ch_addr, SET_I0SET);
			  	cnet_buff = (ushort)(*(float *)ParValue*(*BoardID[devHandle][crNum])[slot].ires + 0.5);
				  break;
			  case PAR_I1S:
          code = MAKE_CODE(ch_addr, SET_I1SET);
				  cnet_buff = (ushort)(*(float *)ParValue*(*BoardID[devHandle][crNum])[slot].ires + 0.5);
				  break;
			  case PAR_RUP:
          code = MAKE_CODE(ch_addr, SET_RUP);
				  cnet_buff = (ushort)(*(float *)ParValue*(*BoardID[devHandle][crNum])[slot].vres + 0.5);
				  break;
			  case PAR_RDW:
          code = MAKE_CODE(ch_addr, SET_RDWN);
				  cnet_buff = (ushort)(*(float *)ParValue*(*BoardID[devHandle][crNum])[slot].vres + 0.5);
				  break;
			  case PAR_TRP:
          code = MAKE_CODE(ch_addr, SET_TRIP);
				  cnet_buff = (ushort)(*(float *)ParValue*10.0 + 0.5);
				  break;
			  case PAR_PW:
          code = MAKE_CODE(ch_addr, SET_FLAG);
			  	cnet_buff = (ushort)( *(unsigned *)ParValue );
          cnet_buff &= 1;
				  break;
			}

			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, &cnet_buff,sizeof(cnet_buff),NULL);

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

  Sy127TestBdPresence
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127TestBdPresence(int devHandle, int crNum, ushort slot, char *Model, 
								 char *Description, ushort *SerNum, 
								 uchar *FmwRelMin, uchar *FmwRelMax)
{
	CAENHVRESULT res;

	if( ( res = Sy127SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		if( (*BoardID[devHandle][crNum])[slot].present )
		{
      int id;

      id = (*BoardID[devHandle][crNum])[slot].id;
			strcpy(Model, modul_type[id].name);
      sprintf(Description, "%s %s",modul_type[id].desc,(*BoardID[devHandle][crNum])[slot].pos ? "Pos.":"Neg.");
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

  Sy127GetBdParamInfo
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetBdParamInfo(int devHandle, int crNum, ushort slot, 
								 char **ParNameList)
{		
	return CAENHV_NOTPRES;
}

/***------------------------------------------------------------------------

  Sy127GetBdParamProp
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetBdParamProp(int devHandle, int crNum, ushort slot, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  Sy127GetBdParam
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValList)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  Sy127SetBdParam
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127SetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, const char *ParName, 
							 void *ParValue)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  Sy127GetSysComp
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetSysComp(int devHandle, int crNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList)
{
	CAENHVRESULT  res;

	if( ( res = Sy127SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		int    i;
		ushort *app1;
		ulong  *app2;

		*NrOfCr = 1;
		*CrNrOfSlList = malloc(sizeof(ushort));
		*SlotChList   = malloc(10*sizeof(ulong));

		app1 = *CrNrOfSlList;
		app2 = *SlotChList;

		*app1 = 10;

		for( i = 0 ; i < 10 ; i++ )
			if( (*BoardID[devHandle][crNum])[i].present )
      {
        int id = (*BoardID[devHandle][crNum])[i].id;

				app2[i] = ( i << 0x10 ) | modul_type[id].numch;
      }
			else
				app2[i] = ( i << 0x10 );
	}
	else
		res = CAENHV_READERR;

	return res;
}

/***------------------------------------------------------------------------

  Sy127GetCrateMap

  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetCrateMap(int devHandle, int crNum, ushort *NrOfSlot, 
							  char **ModelList, char **DescriptionList, 
							  ushort **SerNumList, uchar **FmwRelMinList, 
							  uchar **FmwRelMaxList)
{
	CAENHVRESULT  res;

	if( ( res = Sy127SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		char			Desc[80]  = "  12 CH   3000V  3000.00uA    ", *app1, *app2;
		ushort			i, sl     = 10, *app3, modlen = 0, desclen = 0;

		*ModelList       = malloc(sl*10);					/* Max among 'boards[bd].name's */
		*DescriptionList = malloc(sl*(strlen(Desc)+1));
		*SerNumList      = malloc(sl*sizeof(ushort));
		*FmwRelMinList   = malloc(sl*sizeof(uchar));
		*FmwRelMaxList   = malloc(sl*sizeof(uchar));
		*NrOfSlot = sl;

		app1 = *FmwRelMinList, app2 = *FmwRelMaxList, app3 = *SerNumList;

		for( i = 0 ; i < sl ; i++ )
			if( (*BoardID[devHandle][crNum])[i].present )
			{
        int id = (*BoardID[devHandle][crNum])[i].id;

				memcpy(*ModelList+modlen, modul_type[id].name, 
					   sizeof(modul_type[id].name));
				*(*ModelList+modlen + sizeof(modul_type[id].name)) = '\0';
        sprintf(Desc, "%s %s",modul_type[id].desc,
                      (*BoardID[devHandle][crNum])[i].pos ? "Pos." : "Neg.");
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

  Sy127GetExecCommList
 
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetExecCommList(int devHandle, int crNum, ushort *NumComm, 
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

  Sy127ExecComm

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127ExecComm(int devHandle, int crNum, const char *CommName)
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
	else if( !strcmp(CommName, execComm[1]) )	/* Format */
	{
		code = FORMAT_EEPROM_1;

		errors[devHandle][crNum] = 
			caenet_comm(devHandle,crNum,code,NULL,0,NULL);

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

  Sy127GetSysPropList

  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetSysPropList(int devHandle, int crNum, ushort *NumProp, 
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

  Sy127GetSysPropInfo

  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetSysPropInfo(int devHandle, int crNum, const char *PropName, 
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

  Sy127GetSysProp

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127GetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Result)
{
	int          i, code;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		    /* ModelName */
	{
		strcpy((char *)Result, "SY127");
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
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* LockKeyboard */
  {
    ushort   v;
    unsigned value;

    code = READ_PROT;
    errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, &v);
    if( errors[devHandle][crNum] == TUTTOK )
    {
      value = v;
      value = ( value & (1<<2) ) >> 2;
      if( value == 1 )
        *(unsigned *)Result = 0;
      else
        *(unsigned *)Result = 1;
			res = CAENHV_OK;
    }
		else
  	  res = CAENHV_READERR;
  }
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* FrontPanStat */
	{
		ushort   v;

		code    = READ_PROT;
		errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, &v);
		if( errors[devHandle][crNum] == TUTTOK )
		{
			*(ushort *)Result = v&0xf8;
			res = CAENHV_OK;
		}
		else
			res = CAENHV_READERR;
	}
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* POn */
  {
    ushort   v;
    unsigned value;

    code = READ_PROT;
    errors[devHandle][crNum] = 
		caenet_comm(devHandle, crNum, code, NULL, 0, &v);
    if( errors[devHandle][crNum] == TUTTOK )
    {
      value = v;
      *(unsigned *)Result = value & 1;
			res = CAENHV_OK;
    }
		else
  	  res = CAENHV_READERR;
  }
	else if( !strcmp(PropName, sysProp.name[6]) )	  /* Pswd */
  {
    ushort v;
    unsigned value;

    code = READ_PROT;
    errors[devHandle][crNum] = caenet_comm(devHandle, crNum, code, NULL, 0, &v);
    if( errors[devHandle][crNum] == TUTTOK )
    {
      value = v;
      *(unsigned *)Result = ( value & (1<<1) ) >> 1;
			res = CAENHV_OK;
    }
		else
  	  res = CAENHV_READERR;
  }

	return res;
}

/***------------------------------------------------------------------------

  Sy127SetSysProp

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
             code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy127SetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Set)
{
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;
	int          code;

	if( !strcmp(PropName, sysProp.name[0]) )		    /* ModelName */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[1]) )	  /* SwRelease */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[2]) )	  /* CnetCrNum */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[3]) )	  /* LockKeyboard */
	{
   ushort pr;

   code = READ_PROT;
   errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,&pr);
   if( errors[devHandle][crNum] != TUTTOK)
      res = CAENHV_WRITEERR;        /* Yes, WRITEERR ! */
   else
   {
     unsigned lock = *(unsigned *)Set;

     pr &= 7;

     if( lock )
       pr &= 3;
     else
       pr |= 4;

     code = SET_PROT;
     errors[devHandle][crNum] = 
		 caenet_comm(devHandle,crNum, code,&pr,sizeof(short),NULL);
		 if( errors[devHandle][crNum] == TUTTOK )
		    res = CAENHV_OK;
		 else
			  res = CAENHV_WRITEERR;
   }
	}
	else if( !strcmp(PropName, sysProp.name[4]) )	  /* FrontPanStat */
		res = CAENHV_NOTSETPROP;
	else if( !strcmp(PropName, sysProp.name[5]) )	  /* POn */
	{
   ushort pr;

   code = READ_PROT;
   errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,&pr);
   if( errors[devHandle][crNum] != TUTTOK)
      res = CAENHV_WRITEERR;        /* Yes, WRITEERR ! */
   else
   {
     unsigned lock = *(unsigned *)Set;

     pr &= 7;
     lock &= 1;

     if( lock )
       pr |= 1;
     else
       pr &= 6;

     code = SET_PROT;
     errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,&pr,sizeof(short),NULL);
		 if( errors[devHandle][crNum] == TUTTOK )
		    res = CAENHV_OK;
		 else
			  res = CAENHV_WRITEERR;
   }
	}
	else if( !strcmp(PropName, sysProp.name[6]) )	  /* Pswd */
	{
   ushort pr;

   code = READ_PROT;
   errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,NULL,0,&pr);
   if( errors[devHandle][crNum] != TUTTOK)
      res = CAENHV_WRITEERR;        /* Yes, WRITEERR ! */
   else
   {
     unsigned ps = *(unsigned *)Set;

     pr &= 7;
     ps &= 1;

     if( ps )
       pr |= 2;
     else
       pr &= 5;

     code = SET_PROT;
     errors[devHandle][crNum] = caenet_comm(devHandle,crNum,code,&pr,sizeof(short),NULL);
		 if( errors[devHandle][crNum] == TUTTOK )
		    res = CAENHV_OK;
		 else
			  res = CAENHV_WRITEERR;
   }
	}

	return res;
}


/***------------------------------------------------------------------------

  Sy127Init
  This is the entry point into the routines of this file: it is explicitely
  called when the user calls CAENHVInitSystem()
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
int Sy127Init(int devHandle, int crNum)
{
	if( BoardID[devHandle][crNum] == 0 )
		BoardID[devHandle][crNum] = malloc(sizeof(struct bdid_tag)*10);

	return Sy127SysInfo(devHandle, crNum);
}

/***------------------------------------------------------------------------

  Sy127End
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
void Sy127End(int devHandle, int crNum)
{
  if( BoardID[devHandle][crNum] )
  {
    free(BoardID[devHandle][crNum]);

    BoardID[devHandle][crNum]     = (BdID)0;
  }
}

/***------------------------------------------------------------------------

  Sy127GetError
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
char *Sy127GetError(int devHandle, int crNum)
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

