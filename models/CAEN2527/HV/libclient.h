/******************************************************************************/
/*                                                                            */
/*       --- CAEN Engineering Srl - Computing Division ---                    */
/*                                                                            */
/*   SY1527 Software Project                                                  */
/*                                                                            */
/*   LIBCLIENT.H                                                              */
/*                                                                            */
/*   Created: January 2000                                                    */
/*                                                                            */
/*   Auth: E. Zanetti, A. Morachioli                                          */
/*                                                                            */
/*   Release: 1.0L                                                            */
/*																			  */
/*  Modifiche																  */
/*   Release 1.01L: introdotto SETHVCLKCFG e modificata define MAX_BOARD_DESC */
/*   Release 2.00L: introdotta CNETCOMM										  */
/*                                                                            */
/******************************************************************************/
#ifndef __LIBCLIENT
#define __LIBCLIENT

#include "sy1527oslib.h"

/* define generiche */
#define LIBSEP ':'
#define LIB_SEP ":"
#define LIB_SOCKPORT        2527
#define LIB_TCPCOD          0
#define LIB_RS232COD        1
#define LIB_CONNECT_TIMEOUT 5   /* timeout sulla login */
#define LIB_TIMEOUT         20  /* timeout di attesa della risposta da parte del server, in secondi */
#define LIB_MAXSTRLEN       256
#define LIB_MAXLENCOM       20
#define LIB_MAXLENITEM      30
#define LIB_MAXLENPROP      10

/* tipi di canali di comunicazioni ammessi */
#define LIB_TCP     "TCP/IP"
#define LIB_SERIAL  "RS232"

/* stringhe associate ai codici di errore positivi */
#define LIB_NOSERIAL_STR "Communication on the RS232 device is not yet avalaible"
#define LIB_TIMEERR_STR  "Timeout from server"
#define LIB_DOWN_STR     "CFE server down"
#define LIB_OK_STR       "ok"
#define LIB_SYS_NOT_PRES "System NOT Present"
#define LIB_SLOT_NOT_PRES "Slot NOT Present"
#define LIB_NO_MORE_LOG  "NO MORE Login are permitted"
#define LIB_NO_MORE_MEM  "NO MORE MEMORY FOR LOGIN"
#define LIB_NO_MEMORY    "User Memory not sufficient:Please MAKE LOGOUT"
#define LIB_NO_MEM_FOR_MSG  "User Memory not sufficient for communication"
#define LIB_OUT_OF_RANGE   "Value out of range"
#define LIB_PROP_NOT_FOUND   "Property not found"
#define LIB_EXEC_NOT_FOUND   "Execute command not found"
#define LIB_PROP_NOT_IMPL   "Property yet not avalaible"
#define LIB_EXEC_NOT_IMPL   "Execute command not yet avalaible"
#define LIB_NOT_GET_PROP   "No Get Property"
#define LIB_NOT_SET_PROP   "No Set Property"
#define LIB_NOT_SYS_PROP   "No System Property"
#define LIB_NOT_EXEC_COMM  "No Execute Command"
#define LIB_SYS_CONF_CHANGE "System configuration is change"
#define LIB_PAR_PROP_NOT_FOUND "Param Property Not Found"
#define LIB_PAR_NOT_FOUND "Param Not Found"

/* comandi (L = Session, O = Other, G = Get, S = Set) */
#define	LIB_NULLCMD	 ""                           /* di servizio */
#define L_LOGIN "LOGIN"
#define L_LOGOUT "LOGOUT"
#define O_ENAMSG "ENABLEMSG"
#define O_DISAMSG "DISABLEMSG"
#define O_ADDCHGRP "ADDCHTOGRP"
#define O_REMCHGRP "REMCHFROMGRP"
#define G_CHNAME "GETCHNAME"
#define S_CHNAME "SETCHNAME"
#define G_CHPARAM "GETCHPARAM"
#define S_CHPARAM "SETCHPARAM"
#define G_GRPCOMP "GETGRPCOMP"
#define G_GRPPARAM "GETGRPPARAM"
#define S_GRPPARAM "SETGRPPARAM"
#define G_SYSCOMP "GETSYSCOMP"
#define O_TESTBDPRES "TSTBDPRES"
#define O_GETCRATEMAP "GETCRATEMAP"
#define G_SYSINFO "GETSYSINFO"
#define G_BDPARAM "GETBDPARAM"
#define S_BDPARAM "SETBDPARAM"
#define O_G_ACTLOG "GETACTLOG"
#define O_G_SYSNAME "GETSYSNAME"
#define O_S_SYSNAME "SETSYSNAME"
#define O_G_SWREL "GETSWREL"
#define O_KILL "KILL"
#define O_KILL_SURE "KILL_1"
#define O_KILL_OK "KILL_2"
#define O_CLRALARM "CLRALARM"
#define O_CLRALARM_SURE "CLRALARM_1"
#define O_CLRALARM_OK "CLRALARM_2"
#define G_GENCONF "GETGENCFG"
#define S_GENCONF "SETGENCFG"
#define O_FORMAT "FORMAT"
#define G_RESETCONF "GETRSFLAGCFG"
#define S_RESETCONF "SETRSFLAGCFG"
#define G_RESETFLAG "GETRSFLAG"
#define G_IPADDR "GETIPADDR"
#define S_IPADDR "SETIPADDR"
#define G_IPMASK "GETIPNETMSK"
#define S_IPMASK "SETIPNETMSK"
#define G_IPGTW "GETIPGWY"
#define S_IPGTW "SETIPGWY"
#define G_RSPARAM "GETRS232PARAM"
#define S_RSPARAM "SETRS232PARAM"
#define G_CRATENUM "GETCNETCRNUM"
#define S_CRATENUM "SETCNETCRNUM"
#define G_HVPS "GETHVPSM"
#define G_FANSTAT "GETFANSTAT"
#define G_CLOCKFREQ "GETCLKFREQ"
#define G_HVCLOCKCFG "GETHVCLKCFG"
#define S_HVCLOCKCFG "SETHVCLKCFG"       /* Ver. 1.01L */
#define O_GFPIN "GETFPIN"
#define O_GFPOUT "GETFPOUT"
#define L_RSCMDOFF "RS232CMDOFF"
#define O_EXECNET "CNETCOMM"             /* Ver. 2.00L */
#define O_SUBSCR "SUBSCRIBE"             /* Ver. 3.00  */
#define O_UNSUBSCR "UNSUBSCRIBE"         /* Ver. 3.00  */

#define LIBGET     0
#define LIBSET     1
#define LIBGETSET  2
#define LIBEXEC    3

/*
   Le define seguenti derivano da come e' stato organizzato l'indirizzamento
   CANBUS ( 5 bit per lo slot e 3 bit per il crate nel cluster )
*/
#define MAX_CRATES                             8
#define MAX_SLOTS                             32
#define MAX_BOARDS    ( MAX_SLOTS * MAX_CRATES )


#define MAX_CHANNEL_TYPES           1

#define MAX_PARAM_NAME             10
#define MAX_SYMBOLIC_PARAM_NAME	   16   /* Ver. 3.01 */
#define MAX_CHANNEL_NAME           12
#define MAX_BOARD_NAME             12
#define MAX_BOARD_DESC             28  /* Ver. 1.01L - Prima erano 24 */
#define SET                         1
#define MON                         0
#define SIGNED                      1
#define UNSIGNED                    0


/*
  Il tipo del parametro puo' essere:
  Numerico (quindi in set, es, lo interpreto come un numero con virgola o no)
  Onoff (quindi in set, es, lo intendo "togglante")
  Status (quindi avro' bisogno di caratterizzare il suo valore come un bitfield)
*/
#define PARAM_TYPE_NUMERIC          0
#define PARAM_TYPE_ONOFF            1
#define PARAM_TYPE_CHSTATUS         2
#define PARAM_TYPE_BDSTATUS         3
#define PARAM_TYPE_BINARY	    4		/* Rel 2.16 */
#define PARAM_TYPE_STRING	    5		/* REL 3.00 */

/*
  Le unita' di misura che ci sono venute in mente finora; possono essere usate
  per es come indici di un array di stringhe
*/
#define PARAM_UM_NONE               0
#define PARAM_UM_AMPERE             1
#define PARAM_UM_VOLT               2
#define PARAM_UM_WATT               3
#define PARAM_UM_CELSIUS            4
#define PARAM_UM_HERTZ              5
#define PARAM_UM_BAR                6
#define PARAM_UM_VPS                7
#define PARAM_UM_SECOND             8

#define TYPE_CHANNEL_PAR			0    /* Rel 3.00 */
#define TYPE_BOARD_PAR				1
#define TYPE_SYSTEM_PROP			2
#define TYPE_ALARM					3

/****************************************************************************
 * Struttura che definisce il tipo di parametro:                            *
 *  - Name    : Nome del parametro                                          *
 *  - Flag    : Bit field per parametri vari                                *
 *    - Type  : Se e' num., o onoff o status                                *
 *    - Dir   : Set o Mon, cioe' il parametro e' di Scrittura o di Lettura  *
 *    - Sign  : Flag che dice se il valore e' con il segno o no             *
 *  - Min     : Valore minimo settabile/leggibile                           *
 *  - Max     : Valore massimo settabile/leggibile                          *
 *  - Dec     : Numero di cifre decimali                                    *
 *  - Res     : Risoluzione                                                 *
 *  - Um      : Unita di misura del valore (Volt, Ampere ecc.)              *
 *  - Exp     : Moltiplicatore/Divisore dell Unita di misura                *
 *  - Thr     : Soglia di segnalazione di cambiamento del parametro espressa*
 *              in multipli della risoluzione                               *
 *  - On      : Stringa di On per i parametri di tipo ONOFF                 *
 *  - Off     : Stringa di Off per i parametri di tipo ONOFF                 *
 ****************************************************************************/
typedef
  struct tag_param_type
    {
      char            Name[MAX_PARAM_NAME];
      struct
        {
           unsigned   Type  : 16;
           unsigned   Dir   :  1;
           unsigned   Sign  :  1;
           unsigned   Dummy : 14;
        } Flag;
      union
        {
          struct
            {
              unsigned long   Min; /* per ora solo per le rampe */
              unsigned long   Max;
              unsigned short  Dec;
              unsigned short  Res;
              unsigned short  Um;
              short           Exp;
              unsigned short  Thr;
            } Car;
          struct
            {
              char On[8];
              char Off[8];
            } Str;
       } Un;
    } PARAM_TYPE;

/****************************************************************************
 * Tipo che definisce lo stato del canale                                   *
 ****************************************************************************/

typedef
 struct chsts_tag
  {
   unsigned is_on     :  1;
   unsigned is_up     :  1;
   unsigned is_dwn    :  1;
   unsigned is_ovc    :  1;
   unsigned is_ovv    :  1;
   unsigned is_unv    :  1;
   unsigned is_etrp   :  1;
   unsigned is_maxv   :  1;
   unsigned is_edis   :  1;
   unsigned is_itrp   :  1;
   unsigned is_calerr :  1;
   unsigned is_unplug :  1;  
   unsigned dummy     : 20;
  } CHSTATUS;


/* tipi e strutture dati */
typedef struct prop
{
  char PropIt[LIB_MAXLENPROP];
  short PropId;
}PROPER;

typedef struct getsetex 
{
  char Item[LIB_MAXLENITEM];
  char GetSetExec;   /* 0->Get, 1->Set, 2->GetSet, 3->Exec, -1->niente */
  char SetComm[LIB_MAXLENCOM];
  char GetComm[LIB_MAXLENCOM];
  char ExecComm[LIB_MAXLENCOM];
  int Impl;         /* 0->Not Impl., 1->Impl. */
  int CommId;
}GETSETEXE;

typedef struct cmdinfo  
{
  char nome[LIB_MAXSTRLEN];
  int  numparHeadIn;             /* num di campi dell'header in input */
  int  numparIn;                 /* num di campi variabili in input */
  int  numparHeadOut;            /* num di campi dell'header di out */
  int  numparOut;                /* num di par. variabili di out */
}COMANDO;

typedef struct comm  
{
  char nameComm[LIB_MAXLENCOM]; /* tipo di canle */
  int systemInd;
  unsigned long sessionID;       /* numero di sessione corrente */
#ifdef WIN32
  SOCKET canaleFD;
#endif
#ifdef UNIX
  int canaleFD;
#endif
}CTYPE;

typedef struct indici  
{
  unsigned short NrOfChPar; /* numero di parametri di scheda */
  unsigned short *IndChPar; /* indici di ParTypes: sono NrOfChPar */
}INDPARAM;

typedef struct boa  
{
  unsigned short Position; /* posizione: crate(byte MSB),slot(byte LSB) */
  unsigned short NrOfBdPar; /* numero di parametri di scheda */
  unsigned short *IndBdPar; /* indici di ParTypes: sono NrOfBdPar */
  unsigned short NrOfCh;
  INDPARAM *IndParCh;      /* sono NrOfCh */
}SYSBOARD;

typedef struct sys  
{
  unsigned short NrOfSysParam; /* numero di parametri del sistema */
  PARAM_TYPE *ParTypes;
  unsigned short NrOfBd;       /* numero di schede */
  SYSBOARD *Board;             /* sono NrOfBd */
}SYSINFO;

typedef struct statusinfo  
{
  COMANDO *cmdNow;
  CTYPE commType;
  SYSINFO *system;
  short errCode;                 /* codice di errore */
  char errString[LIB_MAXSTRLEN]; /* stringa di errore corrispondente */
  int headLen;                   /* lunghezza header comando */
} STATUS;

typedef struct IDinfo        /* Ver. 3.00 */
{
  long		    ID;
  short			Type;
  char			Item[48];
  PARAM_TYPE	ParItem;
  struct IDinfo	*Prev;
  struct IDinfo	*Next;
} IDINFO;

/* Rel. 3.00 */
typedef struct {    
	char	Type;
	char	ItemID[64];
	char	Lvalue[4];
	char	Tvalue[256];
} CAENHVEVENT_TYPE_ALIAS;

/* variabili esterne */
extern COMANDO CommandList[];
extern GETSETEXE GSEList[];
extern int MaxSysLogNow;
extern STATUS **stNow;
extern PROPER ParamProp[];
extern IDINFO *IDsNow;        /* Ver. 3.00 */

/* definizioni delle funzioni */
GETSETEXE *PropExecSearch(int indSys, const char *name);
COMANDO *FindCmdInfo(char *cmd);
int FindSystemStruct(int indSys, STATUS **st);
void FindSetAck(STATUS *st, int rislen, char *ris);
void doHeader(STATUS *st, char **str, int *len);
char *setSystemError(void);
int apriCanale(STATUS *st, const char *canale, const char *address);
int chiudiCanale(STATUS *st);
int sendReceive(STATUS *st,char *mess,unsigned long messLen,int *outlen,char **outmsg);
int salvaErrore(STATUS *st, int errore, char *frase);
void freeSystemStruct(int strInd);
void freeStNowStruct(int IndStr);
int ConvListItems(unsigned short NrOfItems, const char *ListOfItem, char **ListItemsConv, 
				  unsigned long *len);
int ConvListID(int idx, unsigned short NrOfItems, char *ListOfItem, char **ListIDConv, 
				  unsigned long *len);

void AddIDsNow( int IndSys, long ID, char *Item );
void RemIDsNow( int IndSys, char *Item );
int FindBdChValPar(char *buffer, PARAM_TYPE *p, CAENHVEVENT_TYPE_ALIAS *EventData);
void FindSysPar(int SysId, char *buffer, PARAM_TYPE *p, CAENHVEVENT_TYPE_ALIAS *EventData);
void FindAlarmPar(char *buffer, PARAM_TYPE *p, CAENHVEVENT_TYPE_ALIAS *EventData);
IDINFO *FindID(int IndSys, long ID);
void RemoveIDsNow(int IndSys);
#if !defined(DARWIN)
void RemoveIDsNowInfo(int IndSys); 
#endif /* DARWIN */
#endif
