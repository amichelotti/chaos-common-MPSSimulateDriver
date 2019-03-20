/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY527INTERFACE.C                                        		       */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    November 2000    : Created                                           */
/*    April    2002    : Rel. 2.1 - Added the devHandle param              */
/*                                                                         */
/***************************************************************************/
#include  <string.h>
#include  <time.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <stdarg.h>
#include  <stdio.h>
#ifdef UNIX
#include  <errno.h>				/* Rel. 2.0 - Linux */
#endif
#include  "HSCAENETLib.h"			/* Rel. 2.1 */
#include  "CAENHVWrapper.h"
#include  "sy527interface.h"

#define   MAX_CH_TYPES                            20

#define   IDENT                                    0
#define   READ_STATUS                              1
#define   READ_SETTINGS                            2
#define   READ_BOARD_INFO                          3
#define   READ_CR_CONF                             4
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
#define   SET_CH_NAME	                          0x19
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

#define   PAR_V0S	  			 1
#define   PAR_I0S				   2
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

#define   MAX_NR_OF_CARDS     10			/* Rel. 2.1 - It seems more than enough */


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
 short   trip, dummy;
 ushort  flag;
};

/*
  The following structure contains all the useful information about
  the status of a channel
*/
struct hvrd
{
 long   vread;
 short  hvmax;
 short  iread;
 ushort status;
};

/*
  The following structure contains all the useful information about the
  characteristics of every board
*/
struct board
{
 char    name[5];
 char    curr_mis;
 ushort  sernum;
 char    vermaior;
 char    verminor;
 char    reserved[20];
 uchar   numch;
 ulong   omog;
 long    vmax;
 short   imax, rmin, rmax;
 short   resv, resi, decv, deci;
 uchar   present;
};

/*
  The following structure contains all the useful information about the
  types of a channel
*/
struct chtype
{
 char   iumis;
 char   dummy;
 char   reserved[4];
 long   vmax;
 short  imax, rmin, rmax;
 short  resv, resi, decv, deci;
 char   dummy1[4];
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

typedef struct board     (*bds)[10];
typedef char             (*ch_to_typ)[10][32];
typedef struct chtype    (*chtyps)[10][MAX_CH_TYPES];

/*
  Globals
  Rel. 2.10 - Added const to constant global variables.
            - Deleted the code and cratenum global variables
			  due to interfernce between thread.
*/
/*static int    			  code, cratenum; */
static const float			  pow_10[] = { 1.0, 10.0, 100.0, 1000.0 };
static const float			  pow_01[] = { 1.0, (float)0.10, (float)0.010, (float)0.0010 };

/*
   Rel. 2.1 - Now we handle more than one card for PC
   Rel. 2.10 - The interferences between threads are avoided 
			   because the same values are written in the same place.
*/
static bds				  boards[MAX_NR_OF_CARDS][100];

/*
  The following array contains the type of every channel for the not
  homogeneous boards
*/
/*
   Rel. 2.1 - Now we handle more than one card for PC
   Rel. 2.10 - The interferences between threads are avoided 
			   because the same values are written in the same place.
*/
static ch_to_typ          ch_to_type[MAX_NR_OF_CARDS][100];
static chtyps			  chtypes[MAX_NR_OF_CARDS][100];

static int                errors[MAX_NR_OF_CARDS][100];			/* The last error code */

/* The list of Channels' Parameters for SY527 */
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

/* The list of Board's Parameters for SY527 */
static const char         *BdPar[] = {
	"HVMax"
};

/* Correspondence between the A, mA, uA, nA and the Exp property of the I params */
static const short        IuMis[]  = { -1, -3, -6, -9 };
static const char  		  *IuName[] = { "A", "mA", "uA", "nA" };

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

  Size_of_Board
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct board)

    --------------------------------------------------------------------***/
static int size_of_board(void)
{
 struct board b;

 return   sizeof(b.curr_mis) + sizeof(b.deci)     + sizeof(b.decv)     
		+ sizeof(b.imax)     + sizeof(b.name)     + sizeof(b.numch)    
		+ sizeof(b.omog)     + sizeof(b.reserved) + sizeof(b.resi)     
		+ sizeof(b.resv)     + sizeof(b.rmax)     + sizeof(b.rmin) 
		+ sizeof(b.sernum)   + sizeof(b.vermaior) + sizeof(b.verminor) 
		+ sizeof(b.vmax)     + sizeof(b.present);
}

/***------------------------------------------------------------------------

  Size_of_Chtype
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct chtype)

    --------------------------------------------------------------------***/
static int size_of_chtype(void)
{
 struct chtype c;

 return   sizeof(c.deci)     + sizeof(c.decv)  + sizeof(c.dummy)    
		+ sizeof(c.dummy1)   + sizeof(c.imax)  + sizeof(c.iumis) 
		+ sizeof(c.reserved) + sizeof(c.resi)  + sizeof(c.resv) 
		+ sizeof(c.rmax)	 + sizeof(c.rmin)  + sizeof(c.vmax);
}

/***------------------------------------------------------------------------

  Size_of_Chset
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct hvch)

    --------------------------------------------------------------------***/
static int size_of_chset(void)
{
 struct hvch ch;

 return   sizeof(ch.chname) + sizeof(ch.dummy) + sizeof(ch.flag) 
	    + sizeof(ch.i0set)  + sizeof(ch.i1set) + sizeof(ch.rdwn)  
	    + sizeof(ch.rup)    + sizeof(ch.trip)  + sizeof(ch.v0set)  
  		+ sizeof(ch.v1set)  + sizeof(ch.vmax);
}

/***------------------------------------------------------------------------

  Size_of_Chrd
  We need the true size in bytes as SY527 sends it via CAENET; it is different
  from sizeof(struct hvrd)

    --------------------------------------------------------------------***/
static int size_of_chrd(void)
{
 struct hvrd ch;

 return   sizeof(ch.hvmax) + sizeof(ch.iread) + sizeof(ch.status) 
	    + sizeof(ch.vread);
}

/***------------------------------------------------------------------------

  Build_Bd_Info
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
static int build_bd_info(int devHandle, int cr, int b, char *cnetbuff)
{
struct board  *bd = &(*boards[devHandle][cr])[b];
int           i   = sizeof(bd->name);

swap_byte(cnetbuff, size_of_board());

memcpy(bd, cnetbuff, i);

memcpy(&bd->curr_mis, cnetbuff+i, sizeof(bd->curr_mis));
i += sizeof(bd->curr_mis);

memcpy(&bd->sernum, cnetbuff+i, sizeof(bd->sernum));
swap_byte((char *)&bd->sernum,sizeof(bd->sernum));
i += sizeof(bd->sernum);

memcpy(&bd->vermaior, cnetbuff+i, sizeof(bd->vermaior));
i += sizeof(bd->vermaior);

memcpy(&bd->verminor, cnetbuff+i, sizeof(bd->verminor));
i += sizeof(bd->verminor) + sizeof(bd->reserved);

memcpy(&bd->numch, cnetbuff+i, sizeof(bd->numch));
i += sizeof(bd->numch);

memcpy(&bd->omog, cnetbuff+i, sizeof(bd->omog));
swap_long(&bd->omog);
i += sizeof(bd->omog);

memcpy(&bd->vmax, cnetbuff+i, sizeof(bd->vmax));
swap_long(&bd->vmax);
i += sizeof(bd->vmax);

memcpy(&bd->imax, cnetbuff+i, sizeof(bd->imax));
swap_byte((char *)&bd->imax,sizeof(bd->imax));
i += sizeof(bd->imax);

memcpy(&bd->rmin, cnetbuff+i, sizeof(bd->rmin));
swap_byte((char *)&bd->rmin,sizeof(bd->rmin));
i += sizeof(bd->rmin);

memcpy(&bd->rmax, cnetbuff+i, sizeof(bd->rmax));
swap_byte((char *)&bd->rmax,sizeof(bd->rmax));
i += sizeof(bd->rmax);

memcpy(&bd->resv, cnetbuff+i, sizeof(bd->resv));
swap_byte((char *)&bd->resv,sizeof(bd->resv));
i += sizeof(bd->resv);

memcpy(&bd->resi, cnetbuff+i, sizeof(bd->resi));
swap_byte((char *)&bd->resi,sizeof(bd->resi));
i += sizeof(bd->resi);

memcpy(&bd->decv, cnetbuff+i, sizeof(bd->decv));
swap_byte((char *)&bd->decv,sizeof(bd->decv));
i += sizeof(bd->decv);

memcpy(&bd->deci, cnetbuff+i, sizeof(bd->deci));
swap_byte((char *)&bd->deci,sizeof(bd->deci));
i += sizeof(bd->deci) + sizeof(bd->present);

if( bd->omog & (1L<<0x11) )      /* The board is not homogeneous */
  {
   int    j, n = bd->numch;
   short  typ;

   memcpy(&typ,cnetbuff+i,sizeof(typ));
   i += sizeof(typ);

   memcpy(*ch_to_type[devHandle][cr][b], cnetbuff+i, (n&1) ? n+1 : n);
   i += ( (n&1) ? n+1 : n );
   swap_byte(*ch_to_type[devHandle][cr][b],32);

   for( j = 0 ; j < typ ; j++ )
      {
       swap_byte(cnetbuff+i, size_of_chtype());
     
       memcpy(&(*chtypes[devHandle][cr])[b][j].iumis, cnetbuff+i, 
		         sizeof((*chtypes[devHandle][cr])[b][j].iumis));
       i += sizeof((*chtypes[devHandle][cr])[b][j].iumis) 
		 + sizeof((*chtypes[devHandle][cr])[b][j].dummy) 
		 + sizeof((*chtypes[devHandle][cr])[b][j].reserved);
 
       memcpy(&(*chtypes[devHandle][cr])[b][j].vmax, cnetbuff+i, 
		         sizeof((*chtypes[devHandle][cr])[b][j].vmax));
       swap_long(&(*chtypes[devHandle][cr])[b][j].vmax);
       i += sizeof((*chtypes[devHandle][cr])[b][j].vmax);

       memcpy(&(*chtypes[devHandle][cr])[b][j].imax, cnetbuff+i, 
		         sizeof((*chtypes[devHandle][cr])[b][j].imax));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].imax, 
		         sizeof((*chtypes[devHandle][cr])[b][j].imax));
       i += sizeof((*chtypes[devHandle][cr])[b][j].imax);

       memcpy(&(*chtypes[devHandle][cr])[b][j].rmin, cnetbuff+i, 
		         sizeof((*chtypes[devHandle][cr])[b][j].rmin));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].rmin, 
		         sizeof((*chtypes[devHandle][cr])[b][j].rmin));
       i += sizeof((*chtypes[devHandle][cr])[b][j].rmin);

       memcpy(&(*chtypes[devHandle][cr])[b][j].rmax, cnetbuff+i, 
		        sizeof((*chtypes[devHandle][cr])[b][j].rmax));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].rmax, 
		        sizeof((*chtypes[devHandle][cr])[b][j].rmax));
       i += sizeof((*chtypes[devHandle][cr])[b][j].rmax);

       memcpy(&(*chtypes[devHandle][cr])[b][j].resv, cnetbuff+i, 
		        sizeof((*chtypes[devHandle][cr])[b][j].resv));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].resv, 
		        sizeof((*chtypes[devHandle][cr])[b][j].resv));
       i += sizeof((*chtypes[devHandle][cr])[b][j].resv);

       memcpy(&(*chtypes[devHandle][cr])[b][j].resi, cnetbuff+i, 
		        sizeof((*chtypes[devHandle][cr])[b][j].resi));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].resi, 
		        sizeof((*chtypes[devHandle][cr])[b][j].resi));
       i += sizeof(&(*chtypes[devHandle][cr])[b][j].resi);

       memcpy(&(*chtypes[devHandle][cr])[b][j].decv, cnetbuff+i, 
		        sizeof((*chtypes[devHandle][cr])[b][j].decv));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].decv, 
		        sizeof((*chtypes[devHandle][cr])[b][j].decv));
       i += sizeof((*chtypes[devHandle][cr])[b][j].decv);

       memcpy(&(*chtypes[devHandle][cr])[b][j].deci, cnetbuff+i, 
		        sizeof((*chtypes[devHandle][cr])[b][j].deci));
       swap_byte((char *)&(*chtypes[devHandle][cr])[b][j].deci, 
		        sizeof((*chtypes[devHandle][cr])[b][j].deci));
       i += sizeof((*chtypes[devHandle][cr])[b][j].deci) 
		 + sizeof((*chtypes[devHandle][cr])[b][j].dummy1);
      }
  }

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

memcpy(&ch->dummy,cnetbuff+i, sizeof(ch->dummy));
swap_byte((char *)&ch->dummy, sizeof(ch->dummy));
i += sizeof(ch->dummy);

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

memcpy(&ch->hvmax, cnetbuff+i, sizeof(ch->hvmax));
swap_byte((char *)&ch->hvmax, sizeof(ch->hvmax));
i += sizeof(ch->hvmax);

memcpy(&ch->iread, cnetbuff+i, sizeof(ch->iread));
swap_byte((char *)&ch->iread, sizeof(ch->iread));
i += sizeof(ch->iread);

memcpy(&ch->status, cnetbuff+i, sizeof(ch->status));
swap_byte((char *)&ch->status, sizeof(ch->status));
i += sizeof(ch->status);
}

/***------------------------------------------------------------------------

  Get_Cr_Info

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
static int get_cr_info(int devHandle, int crateNum, ushort *cr_cnf)
{
	int   i, response, code;
	short bd;
	char  cnetbuff[MAX_LENGTH_FIFO];

	code = READ_CR_CONF;
	if( ( response = caenet_comm(devHandle,crateNum,code,NULL,0,cr_cnf)) != TUTTOK )
		return response;

	code = READ_BOARD_INFO;
	for( bd = 0, i = 1 ; bd < 10 ; bd++, i = i << 1 )
		if(*cr_cnf & i)
		{
			if( ( response = caenet_comm(devHandle,crateNum,code,&bd,sizeof(bd),cnetbuff) ) != TUTTOK )
			{
			  (*boards[devHandle][crateNum])[bd].present = 0;
				return response;
			}
			else
			{
				build_bd_info(devHandle, crateNum, bd, cnetbuff);
				(*boards[devHandle][crateNum])[bd].present = 1;
			}
		}
		else
			(*boards[devHandle][crateNum])[bd].present = 0;

	return TUTTOK;
}

/***------------------------------------------------------------------------

  Sy527SysInfo
  This procedure refreshes values of the internal data structures

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
static int Sy527SysInfo(int devHandle, int crNum)
{
	ushort  cr_conf;

	errors[devHandle][crNum] = get_cr_info(devHandle, crNum, &cr_conf);  
	return errors[devHandle][crNum];
}

/***------------------------------------------------------------------------

  Sy527GetChName

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME])
{
	int                 i, code;
	ushort              ch_addr;
	char                cnetbuff[MAX_LENGTH_FIFO];
/*	
	Rel. 2.10 - static creates problems with threads

	static struct hvch  ch_set;                        Settings of the channel 
*/
	struct hvch  ch_set;                       /* Settings of the channel */
    CAENHVRESULT        res = CAENHV_OK;

	code     = READ_SETTINGS;

	for( i = 0 ; i < ChNum ; i++ )
	{
		ch_addr = ( slot << 8 ) | ChList[i];

		errors[devHandle][crNum] = 
			caenet_comm(devHandle, crNum, code, &ch_addr, sizeof(ch_addr), cnetbuff);
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

	return res;
}

/***------------------------------------------------------------------------

  Sy527SetChName

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527SetChName(int devHandle, int crNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName)
{
	int                 i, len, code;
	ushort              cnet_buff[20];
	char				*p = (char *)&cnet_buff[1];
    CAENHVRESULT        res = CAENHV_OK;

	code     = SET_CH_NAME;

	for( len = 0 ; ChName[len] ; len++ )
		p[len] = ChName[len];
	p[len] = '\0';

	swap_byte(p, strlen(p)+1);

	len = 2 + len + 1 + (len + 1)%2;

	for( i = 0 ; i < ChNum ; i++ )
	{
		cnet_buff[0] = ( slot << 8 ) | ChList[i];

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

  Sy527GetChParamInfo

  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetChParamInfo(int devHandle, int crNum, ushort slot, ushort Ch, 
								 char **ParNameList)
{
	int          i;
	CAENHVRESULT res;

	if( slot >= 10 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !(*boards[devHandle][crNum])[slot].present )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( Ch >= (*boards[devHandle][crNum])[slot].numch )
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

  Sy527GetChParamProp
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetChParamProp(int devHandle, int crNum, ushort slot, ushort Ch, 
								 const char *ParName, const char *PropName, 
								 void *retval)
{
	CAENHVRESULT  res = CAENHV_OK;
	
	if( slot >= 10 )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else if( !(*boards[devHandle][crNum])[slot].present )
	{
		errors[devHandle][crNum] = -253;
		res = CAENHV_SLOTNOTPRES;
	}
	else if( Ch >= (*boards[devHandle][crNum])[slot].numch )
	{
		errors[devHandle][crNum] = -254;
		res = CAENHV_OUTOFRANGE;
	}
	else
	{
		float imax, vmax, deci, rmin, rmax;
		short iumis;

		/* Rel. 2.4 - Added index and casts for UNIX warnings on types */
		if( (*boards[devHandle][crNum])[slot].omog & (1L<<0x11) )
		{
			int index = (*ch_to_type[devHandle][crNum])[slot][Ch];
		    
			vmax  = (float)(*chtypes[devHandle][crNum])[slot][index].vmax;
			deci  = pow_10[(*chtypes[devHandle][crNum])[slot][index].deci];
			imax  = (float)(*chtypes[devHandle][crNum])[slot][index].imax/deci;
			iumis = IuMis[(int)(*chtypes[devHandle][crNum])[slot][index].iumis];
			rmin  =	(*chtypes[devHandle][crNum])[slot][index].rmin;
			rmax  = (*chtypes[devHandle][crNum])[slot][index].rmax;
		}
		else
		{
			vmax  = (float)(*boards[devHandle][crNum])[slot].vmax;
			deci  = pow_10[(*boards[devHandle][crNum])[slot].deci];
			imax  = (float)((*boards[devHandle][crNum])[slot].imax)/deci;
			iumis = IuMis[(int)(*boards[devHandle][crNum])[slot].curr_mis];
			rmin  = (*boards[devHandle][crNum])[slot].rmin;
			rmax  = (*boards[devHandle][crNum])[slot].rmax;
		}

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
				*(float *)retval = pow_01[(*boards[devHandle][crNum])[slot].deci];
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
				*(float *)retval = pow_01[(*boards[devHandle][crNum])[slot].deci];
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

  Sy527GetChParam

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
							 ushort ChNum, const ushort *ChList, void *ParValList)
{
	int                 i, par, set, code;
	ushort              ch_addr;
	char                cnetbuff[MAX_LENGTH_FIFO];
/*	
	Rel. 2.10 - static creates problems with threads

	static struct hvch  ch_set;                        Settings of the channel 
	static struct hvrd  ch_mon;						   Monitorings of the channel 
*/
	struct hvch  ch_set;                          /* Settings of the channel */
	struct hvrd  ch_mon;						  /* Monitorings of the channel */
    CAENHVRESULT res = CAENHV_OK;

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
		short  decv = (*boards[devHandle][crNum])[slot].decv;
		short  deci = (*boards[devHandle][crNum])[slot].deci;

		if( set )
			code = READ_SETTINGS;
		else
			code = READ_STATUS;

		for( i = 0 ; i < ChNum ; i++ )
		{
			ch_addr = ( slot << 8 ) | ChList[i];

			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, &ch_addr, sizeof(ch_addr), cnetbuff);
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

	      if( (*boards[devHandle][crNum])[slot].omog & (1L<<0x11) )
		{
		      /* Rel. 2.4 - Added index for UNIX warnings on types */
		    int index = (*ch_to_type[devHandle][crNum])[slot][ChList[i]];
		    
			decv = (*chtypes[devHandle][crNum])[slot][index].decv;
		        deci = (*chtypes[devHandle][crNum])[slot][index].deci;
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
 
	return res;
}

/***------------------------------------------------------------------------

  Sy527SetChParam

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527SetChParam(int devHandle, int crNum, ushort slot, const char *ParName, 
						     ushort ChNum, const ushort *ChList, void *ParValue)
{
	int                 i, par, code;
	ushort              cnet_buff[2];
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
		float  scalev = pow_10[(*boards[devHandle][crNum])[slot].decv];
		float  scalei = pow_10[(*boards[devHandle][crNum])[slot].deci];

		for( i = 0 ; i < ChNum ; i++ )
		{
			cnet_buff[0] = ( slot << 8 ) | ChList[i];

			if( (*boards[devHandle][crNum])[slot].omog & (1L<<0x11) )
			{
			  /* Rel. 2.4 - Added index for UNIX warnings on types */
			    int index = (*ch_to_type[devHandle][crNum])[slot][ChList[i]];
			    
			    scalev = pow_10[(*chtypes[devHandle][crNum])[slot][index].decv];
			    scalei = pow_10[(*chtypes[devHandle][crNum])[slot][index].deci];
			}
			switch( par )
			{							
			case PAR_V0S:
				code = SET_V0SET;
				cnet_buff[1]=(ushort)(*(float *)ParValue*scalev + 0.5);
				break;
			case PAR_V1S:
				code = SET_V1SET;
				cnet_buff[1]=(ushort)(*(float *)ParValue*scalev + 0.5);
				break;
			case PAR_I0S:
				code = SET_I0SET;
				cnet_buff[1]=(ushort)(*(float *)ParValue*scalei + 0.5);
				break;
			case PAR_I1S:
				code = SET_I1SET;
				cnet_buff[1]=(ushort)(*(float *)ParValue*scalei + 0.5);
				break;
			case PAR_VMX:
				code = SET_VMAX;
				cnet_buff[1]=(ushort)(*(float *)ParValue);
				break;
			case PAR_RUP:
				code = SET_RUP;
				cnet_buff[1]=(ushort)(*(float *)ParValue);
				break;
			case PAR_RDW:
				code = SET_RDWN;
				cnet_buff[1]=(ushort)(*(float *)ParValue);
				break;
			case PAR_TRP:
				code = SET_TRIP;
				cnet_buff[1]=(ushort)(*(float *)ParValue*10.0 + 0.5);
				break;
			case PAR_PON:
				code = SET_FLAG;
				cnet_buff[1]=(ushort)(( *(unsigned *)ParValue << 7 ) | (1<<0xf) );
				break;
			case PAR_PW:
				code = SET_FLAG;
				cnet_buff[1]=(ushort)(( *(unsigned *)ParValue << 3 ) | (1<<0xb) );
				break;
			case PAR_PEN:
				code = SET_FLAG;
				cnet_buff[1]=(ushort)(( *(unsigned *)ParValue << 6 ) | (1<<0xe) );
				break;
			case PAR_PDN:
				code = SET_FLAG;
				cnet_buff[1]=(ushort)(( *(unsigned *)ParValue << 5 ) | (1<<0xd) );
				break;
			case PAR_PSW:
				code = SET_FLAG;
				cnet_buff[1]=(ushort)(( *(unsigned *)ParValue << 4 ) | (1<<0xc) );
				break;
			}
			
			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, cnet_buff,sizeof(cnet_buff),NULL);
			if( errors[devHandle][crNum] != TUTTOK )
			{
				res = CAENHV_WRITEERR;
				break;
			}
			Sleep(200L);
		}
	}

	return res;
}

/***------------------------------------------------------------------------

  Sy527TestBdPresence
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527TestBdPresence(int devHandle, int crNum, ushort slot, char *Model, 
								 char *Description, ushort *SerNum, 
								 uchar *FmwRelMin, uchar *FmwRelMax)
{
	CAENHVRESULT res;

	if( ( res = Sy527SysInfo(devHandle, crNum) ) == CAENHV_OK )
	{
		if( (*boards[devHandle][crNum])[slot].present )
		{
			strcpy(Model, (*boards[devHandle][crNum])[slot].name);
			sprintf(Description, " %3d CH   %4ldV  %8.2f%s", 
				    (*boards[devHandle][crNum])[slot].numch, (*boards[devHandle][crNum])[slot].vmax, 
					(float)(*boards[devHandle][crNum])[slot].imax/pow_10[(*boards[devHandle][crNum])[slot].deci],
					IuName[(int)(*boards[devHandle][crNum])[slot].curr_mis]); /* Rel. 2.4 - Cast */
			*SerNum = (*boards[devHandle][crNum])[slot].sernum;
			*FmwRelMin = (*boards[devHandle][crNum])[slot].verminor;
			*FmwRelMax = (*boards[devHandle][crNum])[slot].vermaior;
		}
		else
			res = CAENHV_SLOTNOTPRES;
   }

	return res;
}

/***------------------------------------------------------------------------

  Sy527GetBdParamInfo
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetBdParamInfo(int devHandle, int crNum, ushort slot, char **ParNameList)
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

  Sy527GetBdParamProp
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetBdParamProp(int devHandle, int crNum, ushort slot, 
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
		{
			if( (*boards[devHandle][crNum])[slot].omog & (1L<<0x11) )     
				*(float *)retval = /* Rel. 2.4 - Cast */
				(float)(*chtypes[devHandle][crNum])[slot][(int)(*ch_to_type[devHandle][crNum])[slot][0]].vmax;
			else
				*(float *)retval = (float)(*boards[devHandle][crNum])[slot].vmax;
		}
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

  Sy527GetBdParam

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList,
							 const char *ParName, void *ParValList)
{
	int                 i, code;
	ushort              ch_addr;
	char                cnetbuff[MAX_LENGTH_FIFO];
/*	
	Rel. 2.10 - static creates problems with threads

	static struct hvrd  ch_mon;						   Monitorings of the channel 
*/
	struct hvrd  ch_mon;						  /* Monitorings of the channel */
    CAENHVRESULT res = CAENHV_OK;

	if( strcmp(ParName, "HVMax") )
		res = CAENHV_PARAMNOTFOUND;
	else
	{
		code = READ_STATUS;

		for( i = 0 ; i < slotNum ; i++ )
		{
			ch_addr = ( slotList[i] << 8 ) | 0;

			errors[devHandle][crNum] = 
				caenet_comm(devHandle, crNum, code, &ch_addr, sizeof(ch_addr), cnetbuff);
			if( errors[devHandle][crNum] == TUTTOK )
				build_chrd_info(&ch_mon, cnetbuff);
			else
			{
				res = CAENHV_READERR;
				break;
			}

			*((float *)ParValList + i) = (float)ch_mon.hvmax;
		}
	}
	
	return res;
}

/***------------------------------------------------------------------------

  Sy527SetBdParam
  Rel. 2.0 - Substituted CAENHV_OK with CAENHV_PARAMNOTFOUND
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527SetBdParam(int devHandle, int crNum, ushort slotNum, 
							 const ushort *slotList, 
							 const char *ParName, void *ParValue)
{
	return CAENHV_PARAMNOTFOUND;
}

/***------------------------------------------------------------------------

  Sy527GetSysComp
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetSysComp(int devHandle, int crNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList)
{
	CAENHVRESULT  res;

	if( ( res = Sy527SysInfo(devHandle, crNum) ) == CAENHV_OK )
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
			if( (*boards[devHandle][crNum])[i].present )
				app2[i] = ( i << 0x10 ) | (*boards[devHandle][crNum])[i].numch;
			else
				app2[i] = ( i << 0x10 );
	}
	else
		res = CAENHV_READERR;

	return res;
}

/***------------------------------------------------------------------------

  Sy527GetCrateMap
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetCrateMap(int devHandle, int crNum, ushort *NrOfSlot, 
							  char **ModelList, 
							  char **DescriptionList, ushort **SerNumList, 
							  uchar **FmwRelMinList, uchar **FmwRelMaxList)
{
	CAENHVRESULT  res;

	if( ( res = Sy527SysInfo(devHandle, crNum) ) == CAENHV_OK )
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
			if( (*boards[devHandle][crNum])[i].present )
			{
				memcpy(*ModelList+modlen, (*boards[devHandle][crNum])[i].name, 
					   sizeof((*boards[devHandle][crNum])[i].name));
				*(*ModelList+modlen + sizeof((*boards[devHandle][crNum])[i].name)) = '\0';
				sprintf(Desc, " %3d CH   %4ldV  %8.2f%s", 
					    (*boards[devHandle][crNum])[i].numch, (*boards[devHandle][crNum])[i].vmax, 
						(float)(*boards[devHandle][crNum])[i].imax/pow_10[(*boards[devHandle][crNum])[i].deci],
						IuName[(int)(*boards[devHandle][crNum])[i].curr_mis]); /* Rel. 2.4 - Cast */
				memcpy(*DescriptionList+desclen, Desc, strlen(Desc)+1);
				app3[i]  = (*boards[devHandle][crNum])[i].sernum;
			    app1[i]  = (*boards[devHandle][crNum])[i].verminor;
			    app2[i]  = (*boards[devHandle][crNum])[i].vermaior;
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

  Sy527GetExecCommList
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetExecCommList(int devHandle, int crNum, ushort *NumComm, 
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

  Sy527ExecComm

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527ExecComm(int devHandle, int crNum, const char *CommName)
{
	CAENHVRESULT res = CAENHV_OK;
	int          code;

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

  Sy527GetSysPropList
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetSysPropList(int devHandle, int crNum, ushort *NumProp, 
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

  Sy527GetSysPropInfo
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetSysPropInfo(int devHandle, int crNum, const char *PropName, 
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

  Sy527GetSysProp

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527GetSysProp(int devHandle, int crNum, const char *PropName, 
							 void *Result)
{
	int          i,code;
	CAENHVRESULT res = CAENHV_PROPNOTFOUND;

	if( !strcmp(PropName, sysProp.name[0]) )		  /* ModelName */
	{
		strcpy((char *)Result, "SY527");
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

  Sy527SetSysProp

  Rel. 2.1 - Added the devHandle param
  Rel 2.10 - Modified for the elimination of the cratenum and 
			 code global variables.

    --------------------------------------------------------------------***/
CAENHVRESULT Sy527SetSysProp(int devHandle, int crNum, const char *PropName, void *Set)
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

  Sy527Init
  This is the entry point into the routines of this file: it is explicitely
  called when the user calls CAENHVInitSystem()
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
int Sy527Init(int devHandle, int crNum)
{
	if( boards[devHandle][crNum] == 0 )
	{
		boards[devHandle][crNum] = malloc(sizeof(struct board)*10);
		ch_to_type[devHandle][crNum] = malloc(sizeof(char)*10*32);
		chtypes[devHandle][crNum] = malloc(sizeof(struct chtype)*10*MAX_CH_TYPES);
	}

	return Sy527SysInfo(devHandle, crNum);
}

/***------------------------------------------------------------------------

  Sy527End
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
void Sy527End(int devHandle, int crNum)
{
  if( boards[devHandle][crNum] )
  {
    free(boards[devHandle][crNum]);
	free(ch_to_type[devHandle][crNum]);
	free(chtypes[devHandle][crNum]);

    boards[devHandle][crNum]     = (bds)0;
    ch_to_type[devHandle][crNum] = (ch_to_typ)0;
    chtypes[devHandle][crNum]    = (chtyps)0;
  }
}

/***------------------------------------------------------------------------

  Sy527GetError
  Rel. 2.1 - Added the devHandle param

    --------------------------------------------------------------------***/
char *Sy527GetError(int devHandle, int crNum)
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
