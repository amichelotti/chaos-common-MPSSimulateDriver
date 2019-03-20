/*****************************************************************************/
/*                                                                           */
/*       --- CAEN Engineering Srl - Computing Division ---                   */
/*                                                                           */
/*   SY1527 Software Project                                                 */
/*                                                                           */
/*   SY1527INTERFACE.C                                                       */
/*   This file contains the implementation  of the wrappers for the          */
/*   Command FrontEnd commands                                               */
/*                                                                           */
/*   Created: January 2000                                                   */
/*                                                                           */
/*   Auth: E. Zanetti, A. Morachioli                                         */
/*                                                                           */
/*   Release: 1.0L                                                           */
/*   Release 1.01L: introdotto SETHVCLKCFG e corretti alcuni bachi           */
/*   Release 1.02L: corretto un errore di allocazione in SetChParam          */
/*   Release 2.00L: introdotta una nuova funzione per i comandi CAENET       */
/*                  Sy1527CaenetComm					     */
/*   Release 2.08 : introdotta SY1527SetBdParam                              */ 
/*                                                                           */
/*****************************************************************************/
#include <string.h>
#ifdef UNIX
#include <unistd.h>
#include <netinet/in.h>
#endif
#include <stdlib.h>
#include <stdio.h>
/* #include "libclient.h" Ver. 3.00 */
#include "sy1527interface.h"

/******************************************************* // !!! CAEN
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
// ******************************************************* // !!! CAEN */
 

#ifndef uchar
#define uchar  unsigned char
#endif

#ifndef ushort
#define ushort unsigned short
#endif

#ifndef ulong
#define ulong  unsigned long
#endif

STATUS **stNow = (STATUS **)NULL;
static int NumberSys = 0;
int MaxSysLogNow = 0;
IDINFO *IDsNow = (IDINFO *)NULL;   /* Ver. 3.00 */

/* dove memorizzo la stringa di errore dell'ultima login o logout effettuata */
static char LogInOut[64];

/* SY1527LIBSWREL *************************************************************
    Return the software release of the library as parameter out
    Parameter Out
     SoftwareRel    string null terminate of software release 
 *******************************************************************************/
 SY1527RESULT Sy1527LibSwRel(char *SoftwareRel)
{
strcpy(SoftwareRel, "2.01");
return SY1527_OK;
}

/* SY1527GETSYSINFO **********************************************************
it's update the struct stNow of the system with ID SysID
Parametri In
 SysId: ID of system
Globali
 stNow
 *****************************************************************************/
 SY1527RESULT Sy1527GetSysInfo(int SysId)
{
int indStr, ret, rislen, indexLen, i, j, y, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *pc, *risposta = (char *)NULL;
ulong *pl, appl;
ushort *ps;
ushort app;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(G_SYSINFO);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);
if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return st->errCode;
  }

if( st->system != (SYSINFO *)NULL ) /* libero la memoria della struttura che voglio */
  freeSystemStruct(indStr);         /* aggiornare */

st->system = malloc(sizeof(SYSINFO)); /* alloco memoria SYSINFO */
if( st->system == NULL )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEMORY);
  }

st->system->Board = (SYSBOARD *)NULL;
st->system->ParTypes =(PARAM_TYPE *)NULL;

/* leggo il risultato */
indexLen = st->headLen;
ps = (ushort *)(risposta + indexLen);
st->system->NrOfSysParam = ntohs(*ps);
indexLen += sizeof(short) + strlen(LIB_SEP);

/* alloco memoria per le strutture PARAM_TYPE */
st->system->ParTypes = malloc(st->system->NrOfSysParam * sizeof(PARAM_TYPE));
if( st->system->ParTypes == (PARAM_TYPE *)NULL )
  {
   freeSystemStruct(indStr);
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEMORY);
  }

for( i = 0; i < st->system->NrOfSysParam; i++ ) /* memorizzo le strutture PARAM_TYPE */
  {
   pc = risposta + indexLen;
   for( j = 0; j < MAX_PARAM_NAME; j++ )
     st->system->ParTypes[i].Name[j] = *pc++;
   indexLen += MAX_PARAM_NAME;
   pl = (ulong *)(risposta + indexLen);
   appl = ntohl(*pl);
   st->system->ParTypes[i].Flag.Type = appl>>16;
   st->system->ParTypes[i].Flag.Dir = (appl&0x8000)>>15;
   st->system->ParTypes[i].Flag.Sign = (appl&0x4000)>>14;
   indexLen += sizeof(ulong);
   if( st->system->ParTypes[i].Flag.Type == PARAM_TYPE_ONOFF )
     {
      pc = risposta + indexLen;
      for( j = 0; j < 8; j++ )
        st->system->ParTypes[i].Un.Str.On[j] = *pc++;
      for( j = 0; j < 8; j++ )
        st->system->ParTypes[i].Un.Str.Off[j] = *pc++;
     }
   else
     {
      pl = (ulong *)(risposta + indexLen);
      st->system->ParTypes[i].Un.Car.Min = ntohl(*pl);
      pl++;
      st->system->ParTypes[i].Un.Car.Max = ntohl(*pl);
      pl++;
      ps = (ushort *)pl;
      st->system->ParTypes[i].Un.Car.Dec = ntohs(*ps);
      ps++;
      st->system->ParTypes[i].Un.Car.Res = ntohs(*ps);
      ps++;
      st->system->ParTypes[i].Un.Car.Um = ntohs(*ps);
      ps++;
      st->system->ParTypes[i].Un.Car.Exp = ntohs(*ps);
      ps++;
      st->system->ParTypes[i].Un.Car.Thr = ntohs(*ps);
      ps++;
     }
   indexLen += 2*sizeof(long) + 5*sizeof(short);
  }
indexLen += strlen(LIB_SEP);

ps = (ushort *)(risposta + indexLen);
st->system->NrOfBd = ntohs(*ps);                  /* NrOfBd */
indexLen += sizeof(short) + strlen(LIB_SEP);

/* alloco memoria per le strutture SYSBOARD */
st->system->Board = malloc(st->system->NrOfBd * sizeof(SYSBOARD));
if( st->system->Board == (SYSBOARD *)NULL )
  {
   freeSystemStruct(indStr);
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEMORY);
  }

for( i = 0; i < st->system->NrOfBd;i++ )          /* Posizione delle schede */
  {
   st->system->Board[i].IndBdPar = (ushort *)NULL;    /* inizializzo i puntatori */
   st->system->Board[i].IndParCh = (INDPARAM *)NULL;
   ps = (ushort *)(risposta + indexLen);
   st->system->Board[i].Position = ntohs(*ps);
   indexLen += sizeof(short);
  }
indexLen += strlen(LIB_SEP);

for( i = 0; i < st->system->NrOfBd;i++ )          /* numero di canali per scheda */
  {
   ps = (ushort *)(risposta + indexLen);
   st->system->Board[i].NrOfCh = ntohs(*ps);
   /* alloco memoria per le strutture INDPARAM */
   st->system->Board[i].IndParCh = malloc(st->system->Board[i].NrOfCh *
                                                               sizeof(INDPARAM));
   if( st->system->Board[i].IndParCh == (INDPARAM *)NULL )
     {
      freeSystemStruct(indStr);
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEMORY);
     }

   for( j = 0; j < st->system->Board[i].NrOfCh; j++ )
     st->system->Board[i].IndParCh[j].IndChPar = (ushort *)NULL; /* inizializzo */

   indexLen += sizeof(short);
  }
indexLen += strlen(LIB_SEP);

for( i = 0; i < st->system->NrOfBd;i++ )          /* numero  di parametri e indici */
  {
   ps = (ushort *)(risposta + indexLen);
   st->system->Board[i].NrOfBdPar = ntohs(*ps);
   indexLen += sizeof(short) + strlen(LIB_SEP);
   /* alloco memoria per gli indici dei parametri di scheda */
   st->system->Board[i].IndBdPar = malloc(st->system->Board[i].NrOfBdPar
                                                                * sizeof(short));
   if( st->system->Board[i].IndBdPar == (ushort *)NULL )
     {
      freeSystemStruct(indStr);
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEMORY);
     }

   for( j = 0; j < st->system->Board[i].NrOfBdPar; j++ )
     {
      ps = (ushort *)(risposta + indexLen);
      st->system->Board[i].IndBdPar[j] = ntohs(*ps);
      indexLen += sizeof(short);
     }
   indexLen += strlen(LIB_SEP);

   for( j = 0; j < st->system->Board[i].NrOfCh; j++ )
     {
      ps = (ushort *)(risposta + indexLen);         /* punta al NrOfChParam */
      st->system->Board[i].IndParCh[j].NrOfChPar = ntohs(*ps);

      /* alloco memoria per gli indici ai parametri di canale */
      app = st->system->Board[i].IndParCh[j].NrOfChPar;
      st->system->Board[i].IndParCh[j].IndChPar = malloc(app*sizeof(short));

      if( st->system->Board[i].IndParCh[j].IndChPar == (ushort *)NULL )
        {
         freeSystemStruct(indStr);
         if (cmdstr) free(cmdstr);
         if (risposta) free(risposta);
         return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEMORY);
        }

      indexLen += sizeof(short) + strlen(LIB_SEP);

      for( y = 0; y < st->system->Board[i].IndParCh[j].NrOfChPar; y++ )
        {
         ps = (ushort *)(risposta + indexLen);
         st->system->Board[i].IndParCh[j].IndChPar[y] = ntohs(*ps);
         indexLen += sizeof(short);
        }
      indexLen += strlen(LIB_SEP);
     }
  }

/* libero la memoria allocata dalle funzioni precendenti */
if(cmdstr) free(cmdstr);
if(risposta) free(risposta);

return st->errCode;
}

/* SY1527LOGIN ****************************************************************
    It is the first function to be used to communicate with the required system.
    It returns the ID of the system (>=0), -1 if the error occured
    Parametri In:
     CommPath  string which identifies the communication channel chosen to
               communicate with the SY1527: "TCP/IP" or "RS232"
     Device    string containing, according to the chosen communication channel,
               the address TCP/IP of the system or the gate (COMM1/COMM2 ) of
               the RS232
     User      name (among the allowed ones) with which the user chooses to log
     Passwd    password associated to the name
    Globals
     stNow        
     NumberSys    
     MaxSysLogNow 
 ******************************************************************************/
 int Sy1527Login(const char *canale, const char *device, 
                                            const char *user, const char *passwd)
{
int ret, purecmdlen, rislen, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned long *s;
STATUS *st;

if( NumberSys < 0 )    /* nel caso che NumberSys abbia superato INT_MAX */
   {
    strcpy(LogInOut,LIB_NO_MORE_LOG);
    return SY1527_LOG_ERR;
   }

strcpy(LogInOut,LIB_OK_STR);

/* alloco memoria al puntatore alla nuova struttura */
MaxSysLogNow++;
if( (stNow=realloc(stNow,MaxSysLogNow*sizeof(STATUS *))) == (STATUS **)NULL )
   {
    MaxSysLogNow--;
    strcpy(LogInOut,LIB_NO_MORE_MEM);
    return SY1527_LOG_ERR;
   }

/* alloco memoria per la nuova struttura */
if( (st = malloc(sizeof(STATUS))) == (STATUS *)NULL )
   {
    MaxSysLogNow--;
    stNow=realloc(stNow,MaxSysLogNow*sizeof(STATUS *));
    strcpy(LogInOut,LIB_NO_MORE_MEM);
    return SY1527_LOG_ERR;
   }
stNow[MaxSysLogNow - 1] = st;

/* apro la connessione */
if( apriCanale(st, canale, device) != SY1527_OK )
   {
    strcpy(LogInOut,st->errString);
    MaxSysLogNow--;
    stNow = realloc(stNow, MaxSysLogNow * sizeof(STATUS *));

/* Rel. 1.4 - Introdotta una chiudiCanale per risolvere il 
//            seguente baco: se la prima login ad un sistema
//            falliva a seguito di una sua mancata connessione,
//            una volta ricollegatolo alla rete anche tutte le 
//            altre login sarebbero poi fallite */
    chiudiCanale(st);

    free(st);
    return SY1527_LOG_ERR;
   }

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(L_LOGIN);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);
purecmdlen = cmdlen;

 /* costruisco il comando ed eseguo invia e ricevi */
cmdlen = cmdlen + strlen(user) + strlen(passwd) +
         strlen(LIB_SEP) * (st->cmdNow->numparIn -1);
cmdstr = realloc(cmdstr, cmdlen + 1);

cmdstr[purecmdlen] = '\0';    /* Altrimenti la sprintf seg. non va ... */
sprintf(cmdstr,"%s%s%s%s", cmdstr, user, LIB_SEP, passwd);

if( sendReceive(st, cmdstr, cmdlen, &rislen, &risposta) != SY1527_OK )
   {
    chiudiCanale(st);
    strcpy(LogInOut,st->errString);
    MaxSysLogNow--;
    stNow = realloc(stNow, MaxSysLogNow * sizeof(STATUS *));
    free(st);
    if (cmdstr) free(cmdstr);
    return SY1527_LOG_ERR;
   }

/* comunicazione tutta OK: quindi setto gli AckCode e String con quelli
   della risposta del server CFE */
FindSetAck(st, rislen, risposta);

/* sessione e l'indice di sistema che ritorno se non ho errori */
if( !st->errCode )
   {
    s = (unsigned long *)(risposta + st->headLen);
    st->commType.sessionID = ntohl(*s);
    st->commType.systemInd = NumberSys;

    st->system = (SYSINFO *)NULL;         /* inizializzo a NULL il puntatore */

    if( Sy1527GetSysInfo(NumberSys) )
      {
       chiudiCanale(st);
       strcpy(LogInOut,st->errString);
       MaxSysLogNow--;
       stNow = realloc(stNow, MaxSysLogNow * sizeof(STATUS *));
       free(st);
       if (cmdstr) free(cmdstr);
       return SY1527_LOG_ERR;
      }

    ret = NumberSys;
    NumberSys++;
   }
else
   {
    chiudiCanale(st);
    strcpy(LogInOut,st->errString);
    MaxSysLogNow--;
    stNow = realloc(stNow, MaxSysLogNow * sizeof(STATUS *));
    free(st);
    if (cmdstr) free(cmdstr);
    return SY1527_LOG_ERR;
   }

/* libero la memoria allocata alla send o receive precedente */
if (cmdstr != NULL) free(cmdstr);
if (risposta != NULL) free(risposta);

/* Alloco la struttura IDsNow    Ver. 3.00 */
if( (IDsNow=realloc(IDsNow,MaxSysLogNow*sizeof(IDINFO))) == NULL )
   {
	chiudiCanale(st);
    freeStNowStruct(MaxSysLogNow-1);
	MaxSysLogNow--;
    strcpy(LogInOut,LIB_NO_MORE_MEM);
    return SY1527_LOG_ERR;
   }
IDsNow[MaxSysLogNow-1].Prev = NULL;
IDsNow[MaxSysLogNow-1].Next = NULL;
IDsNow[MaxSysLogNow-1].ID = -1;
strcpy(IDsNow[MaxSysLogNow-1].Item,"Root");
IDsNow[MaxSysLogNow-1].ParItem.Name[0] = 0;
IDsNow[MaxSysLogNow-1].Type = 0;

return ret;
}

/* SY1527LOGOUT **************************************************************
Function which allow to close the communication , previously opened by a Login,
with sistem of ID SysId
It return the error code

Parametri In
 SysId: ID of System

Globals
 stNow  
 MaxSysLogNow 
 *****************************************************************************/
 int Sy1527Logout(int SysId)
{
int indStr, ret, rislen, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
STATUS *st;

strcpy(LogInOut,LIB_OK_STR);

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   {
    strcpy(LogInOut, LIB_SYS_NOT_PRES);
    return SY1527_NOTPRES;
   }
st = stNow[indStr];


/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(L_LOGOUT);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);


if ( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
    {
     if (cmdstr) free(cmdstr);
     strcpy(LogInOut,st->errString);
	 /*Rel. 3.02 */
	freeStNowStruct(indStr);
	RemoveIDsNow(indStr); /*dealloca struttura IDsNow */
	MaxSysLogNow--;
     return ret;
    }

FindSetAck(st, rislen, risposta);

/* chiudo la connessione solo se Logout e' OK */
ret = st->errCode;
strcpy(LogInOut,st->errString);
if( !ret )
   {
    chiudiCanale(st);
    /* freeStNowStruct(indStr); //rel 3.02 */
   }
/*Rel. 3.02 */

freeStNowStruct(indStr);
RemoveIDsNow(indStr); /*dealloca struttura IDsNow */
MaxSysLogNow--;
/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return ret;
}

/* SY1527GETCHNAME ************************************************************
    Command to receive a list of strings containg the names of the channels
    belonging to the board in slot Slot
    Parameter In
     SysId           Sistem ID
     Slot            It identifies the board in terms of crate (high byte) and
                     slot (low byte)
     ChNum           number of channels of which you want to know the name
     ChList          array of ChNUm channels
    Parameter Out
     ChNameList      array of ChNUm strings containing the name of the channels
    Globals
     stNow        
 ******************************************************************************/
 SY1527RESULT Sy1527GetChName(int SysId, unsigned short Slot,
                             unsigned short ChNum, const unsigned short *ChList,
                                               char (*ChNameList)[MAX_CH_NAME])
{
int ret, n, rislen, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
char *pName;
unsigned short *pp;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(G_CHNAME);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + 2 * sizeof(short) + ChNum * sizeof(short) +
        (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(Slot);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(ChNum);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( n = 0; n < ChNum; n++ )
  {
   pp = (unsigned short *)(cmdstr + cmdlen);
   *pp = htons(ChList[n]);
   cmdlen += sizeof(unsigned short);
  }

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
   {
    if (cmdstr) free(cmdstr);
    if (risposta) free(risposta);
    return st->errCode;
   }

pName = risposta + st->headLen;
for( n = 0; n < ChNum; n++ )
    {
     strcpy(ChNameList[n], pName);
     pName += strlen(ChNameList[n]) + 1;
    }

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527SETCHNAME ************************************************************
    Command to assign the "ChName" to the channels belonging to the board in slot
    Slot
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ChNum        number of channels
     ChName       name to give to the specified channels in the list
     ChList       array of ChNUm channels
    Globals
     stNow        
 ******************************************************************************/
 SY1527RESULT Sy1527SetChName(int SysId, unsigned short Slot,
          unsigned short ChNum, const unsigned short *ChList, const char *ChName)
{
int ret, n, rislen, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *pc, *risposta = (char *)NULL;
unsigned short *ps;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

if( strlen(ChName) >= LIB_MAXSTRLEN )
  return salvaErrore(st, SY1527_OUTOFRANGE, LIB_OUT_OF_RANGE);

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(S_CHNAME);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + 2 * sizeof(short) + ChNum * sizeof(short) +
         (st->cmdNow->numparIn -1) * strlen(LIB_SEP) + LIB_MAXSTRLEN);

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(Slot);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pc = cmdstr + cmdlen;
strcpy(pc, ChName);
cmdlen += strlen(ChName);
cmdstr[cmdlen++]= LIB_SEP[0];

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(ChNum);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( n = 0; n < ChNum; n++ )
  {
   ps = (unsigned short *)(cmdstr + cmdlen);
   *ps = htons(ChList[n]);
   cmdlen += sizeof(unsigned short);
  }

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527GETCHPARAMINFO *******************************************************
    Command to know the  names of the tupes of parameters associated to each
    channel
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     Ch           channel of which you want to know the parameter list
    Parameter Out
     ParNameList  array of strings containing the name of the parameters
                  associated to that channel. The last element of the array
                  is the null string
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetChParamInfo(int SysId, unsigned short Slot,
                                          unsigned short Ch, char **ParNameList)
{
int  i, indStr, indSB, indPar;
unsigned short NrOfPar;
char (*List)[MAX_PARAM_NAME];
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
  return SY1527_NOTPRES;

st = stNow[indStr];

/* cerco la scheda nella mia struttura interna di libreria */
for( i = 0; i < st->system->NrOfBd; i++ )
  if( st->system->Board[i].Position == Slot )
    {
	 indSB = i;
	 break;
	}

if( i == st->system->NrOfBd )
  return salvaErrore(st, SY1527_SLOTNOTPRES, LIB_SLOT_NOT_PRES);

if( Ch >= st->system->Board[indSB].NrOfCh )
  return salvaErrore(st, SY1527_OUTOFRANGE, LIB_OUT_OF_RANGE);

/* cerco i parametri del canale */
pParType = st->system->ParTypes;

NrOfPar = st->system->Board[indSB].IndParCh[Ch].NrOfChPar;

List = malloc((NrOfPar + 1) * MAX_PARAM_NAME);

for( i = 0; i < NrOfPar; i++ )
  {
   indPar = st->system->Board[indSB].IndParCh[Ch].IndChPar[i];
   strcpy(List[i], pParType[indPar].Name);
  }
strcpy(List[i],"");
*ParNameList = (char *)List;

st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527GETCHPARAMPROP *******************************************************
    Function to receive the value associated to the particular property of the
    parameter of the channel identified by slot and ch. This can be a numeric
    value, a string or a bit field
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     Ch           channel
     ParName      parameter name
     PropName     property name
    Parameter Out
     retval       value of the property of the parameter
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetChParamProp(int SysId, unsigned short Slot,
      unsigned short Ch, const char *ParName, const char *PropName, void *retval)
{
int i, indStr, indSB, indPar;
unsigned short indProp;
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* cerco la scheda nella mia struttura interna di libreria */
for( i = 0; i < st->system->NrOfBd; i++ )
  if( st->system->Board[i].Position == Slot )
    {
	 indSB = i;
	 break;
	}

if( i == st->system->NrOfBd )
  return salvaErrore(st, SY1527_SLOTNOTPRES, LIB_SLOT_NOT_PRES);

if( Ch >= st->system->Board[indSB].NrOfCh )
  return salvaErrore(st, SY1527_OUTOFRANGE, LIB_OUT_OF_RANGE);

pParType = st->system->ParTypes;
for( i = 0; i < st->system->Board[indSB].IndParCh[Ch].NrOfChPar; i++ )
  {
   indPar = st->system->Board[indSB].IndParCh[Ch].IndChPar[i];
   if( !strcmp(pParType[indPar].Name, ParName ) )
    break;
  }
if( i== st->system->Board[indSB].IndParCh[Ch].NrOfChPar )
  return salvaErrore(st, SY1527_PARAMNOTFOUND, LIB_PAR_NOT_FOUND);

/* cerco la param property */
indProp = 0;
while(strcmp(ParamProp[indProp].PropIt, "NULL"))
  {
   if( !strcmp(ParamProp[indProp].PropIt, PropName) )
     break;
   indProp++;
  }
if( !strcmp(ParamProp[indProp].PropIt, "NULL") )
  return salvaErrore(st, SY1527_PARAMPROPNOTFOUND, LIB_PAR_PROP_NOT_FOUND);
  
switch( ParamProp[indProp].PropId )
  {
   case 0: /* .Type */
    *(unsigned long *)retval = pParType[indPar].Flag.Type;
    break; 
   case 1: /* .Dir (Mode) */
    if( !pParType[indPar].Flag.Dir )
      *(unsigned long *)retval = PARAM_MODE_RDONLY;
	else if( pParType[indPar].Flag.Dir == 1 )
      *(unsigned long *)retval = PARAM_MODE_RDWR;
	else
      *(unsigned long *)retval = PARAM_MODE_WRONLY;
    break;
   case 2: /* .Sign */
    *(unsigned long *)retval = pParType[indPar].Flag.Sign;
    break;
   case 3: /* .Min */
/* Rel. 1.4 - Devo gestire la possibilita' che il par. abbia segno */
     if( pParType[indPar].Flag.Sign == SIGNED ) 
     {
       int val = (int)pParType[indPar].Un.Car.Min;

       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)val / (float)pParType[indPar].Un.Car.Res;	                                       
       else
        *(float *)retval = (float)val * (float)pParType[indPar].Un.Car.Res;	                                      
     }
     else
     {
       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)pParType[indPar].Un.Car.Min / 
	                                       (float)pParType[indPar].Un.Car.Res;
       else
        *(float *)retval = (float)pParType[indPar].Un.Car.Min * 
	                                       (float)pParType[indPar].Un.Car.Res;
     }
    break;
   case 4: /* .Max */
/* Rel. 1.4 - Devo gestire la possibilita' che il par. abbia segno */
     if( pParType[indPar].Flag.Sign == SIGNED ) 
     {
       int val = (int)pParType[indPar].Un.Car.Max;

       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)val / (float)pParType[indPar].Un.Car.Res;		                                       
       else
        *(float *)retval = (float)val * (float)pParType[indPar].Un.Car.Res;                                           
     }
     else
     {
       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)pParType[indPar].Un.Car.Max / 
		                                       (float)pParType[indPar].Un.Car.Res;
        else
        *(float *)retval = (float)pParType[indPar].Un.Car.Max * 
                                           (float)pParType[indPar].Un.Car.Res;
     }
    break;
   case 5: /* .Unit */
    *(unsigned short *)retval = pParType[indPar].Un.Car.Um;
    break;
   case 6: /* .Exp */
    *(short *)retval = pParType[indPar].Un.Car.Exp;
    break;
   case 7: /* On */
    strcpy((char *)retval,pParType[indPar].Un.Str.On);
    break;
   case 8: /* Off */
    strcpy((char *)retval,pParType[indPar].Un.Str.Off);
    break;
  }

st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527GETCHPARAM ***********************************************************
    Command to receive a list of the values of parameter "ParName" of the
    channels belonging to the board in slot Slot
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ChNum        number of channels
     ChList       array of ChNUm channels
     ParName      parameter name
    Parameter Out
     ParValList   list of ChNum elements containig the values of the parameter
                  specified
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetChParam(int SysId, unsigned short Slot,
         const char *ParName, unsigned short ChNum, const unsigned short *ChList,
                                                                void *ParValList)
{
int ret, n, i, rislen, indStr, indSB, indPar, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *pc, *risposta = (char *)NULL;
unsigned short *pp;
unsigned long *pl, ParValue;
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(G_CHPARAM);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + 2 * sizeof(short) + ChNum * sizeof(short) +
         strlen(ParName) + (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(Slot);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pc = cmdstr + cmdlen;
strcpy(pc, ParName);
cmdlen += strlen(ParName);
cmdstr[cmdlen++]= LIB_SEP[0];

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(ChNum);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( i = 0; i < ChNum; i++ )
  {
   pp = (unsigned short *)(cmdstr + cmdlen);
   *pp = htons(ChList[i]);
   cmdlen += sizeof(unsigned short);
  }

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return st->errCode;
  }

/* cerco la scheda nella mia struttura interna di libreria */
for( i = 0; i < st->system->NrOfBd; i++ )
  if( st->system->Board[i].Position == Slot )
    {
	 indSB = i;
	 break;
	}

if( i == st->system->NrOfBd )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
  }
/*
ParValList = (char **)malloc(ChNum*sizeof(char *));
for( i = 0; i < ChNum; i++ )
   ParValList[i] = (char *)malloc(30);
*/

/* cerco il parametro di canale */
pParType = st->system->ParTypes;
pl = (unsigned long *)(risposta + st->headLen);
for( i = 0; i < ChNum; i++, pl++ )
  {
   if( ChList[i] >= st->system->Board[indSB].NrOfCh )
     {
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
     }
     
   for( n = 0; n < st->system->Board[indSB].IndParCh[ChList[i]].NrOfChPar; n++ )
     {
      indPar = st->system->Board[indSB].IndParCh[ChList[i]].IndChPar[n];
	  if( !strcmp(pParType[indPar].Name, ParName ) )
	    break;
     }
   if( n == st->system->Board[indSB].IndParCh[ChList[i]].NrOfChPar )
     {
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
     }

   /* ora che ho il parametro posso fare le conversioni */
   ParValue = ntohl(*pl);
   switch( pParType[indPar].Flag.Type )
     {
	  case PARAM_TYPE_NUMERIC:
/* Rel. 1.4 - Devo gestire la possibilita' che il par. abbia segno */
    if( pParType[indPar].Flag.Sign == SIGNED ) 
     {
       int val = (int)ParValue;

  	   if( pParType[indPar].Un.Car.Dec > 0 )
           ((float *)ParValList)[i] = (float)val /(float)pParType[indPar].Un.Car.Res; 
	    else
           ((float *)ParValList)[i] = (float)val *  
		                                       (float)pParType[indPar].Un.Car.Res;
     }
     else
     {
  	   if( pParType[indPar].Un.Car.Dec > 0 )
           ((float *)ParValList)[i] = (float)ParValue / 
		                                       (float)pParType[indPar].Un.Car.Res;
	    else
           ((float *)ParValList)[i] = (float)ParValue * 
		                                       (float)pParType[indPar].Un.Car.Res;
     }
	   break;
	  case PARAM_TYPE_ONOFF:
	   ((long *)ParValList)[i] = ParValue;
	   break;
	  case PARAM_TYPE_CHSTATUS:
	   ((long *)ParValList)[i] = ParValue;
	   break;
	  case PARAM_TYPE_BINARY:		/* Rel 2.16 */
	   ((long *)ParValList)[i] = ParValue;	  
		  break;

	 }   
  }

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527SETCHPARAM ***********************************************************
    Command to assign a new value to parameter "ParName" to the channels
    belonging of the board in slot Slot
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ChNum        number of channels
     ChList       array of ChNUm channels
     ParName      parameter name
     ParValue     parameter value
    Globals
     stNow
     
Ver 1.02L corretto un errore di allocazione
 ******************************************************************************/
 SY1527RESULT Sy1527SetChParam(int SysId, unsigned short Slot,
        const char *ParName, unsigned short ChNum, const unsigned short *ChList,
                                                                  void *ParValue)
{
int ret, n, i, rislen, indStr, cmdlenloop, *CopiaChList, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *pc, *risposta = (char *)NULL;
unsigned short *pp, NrChStessoInd, *ChStessoInd;
unsigned short indSB, indPar, indParNow, j;
unsigned long *pl, Value;
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(S_CHPARAM);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

/* Ver 1.02L introdotto sizeof(ChNum) che mancava */
cmdstr = realloc(cmdstr, cmdlen + sizeof(Slot) + ChNum * sizeof(short) + sizeof(ChNum) +
            strlen(ParName) + (st->cmdNow->numparIn -1) * strlen(LIB_SEP) + sizeof(float));

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(Slot);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pc = cmdstr + cmdlen;
strcpy(pc, ParName);
cmdlen += strlen(ParName);
cmdstr[cmdlen++]= LIB_SEP[0];

/* cerco la scheda nella mia struttura interna di libreria */
for( i = 0; i < st->system->NrOfBd; i++ )
  if( st->system->Board[i].Position == Slot )
    {
	 indSB = i;
	 break;
	}
if( i == st->system->NrOfBd )
  {
   if (cmdstr) free(cmdstr);
   return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
  }

ChStessoInd = malloc(ChNum*sizeof(unsigned short));
CopiaChList = malloc(ChNum*sizeof(int));
for( i = 0; i < ChNum; i++ )
  CopiaChList[i] = (int)ChList[i];

/* cerco il parametro di canale */
pParType = st->system->ParTypes;
for( j = 0; j < ChNum; )
  {
   NrChStessoInd = 0;
   for( i = 0; i < ChNum; i++ )
     {
	  if( CopiaChList[i] < 0 )
	    continue;
	  else
	    {
         if( CopiaChList[i] >= st->system->Board[indSB].NrOfCh )
           {
            if (cmdstr) free(cmdstr);
			free(ChStessoInd);
			free(CopiaChList);
            return salvaErrore(st, SY1527_OUTOFRANGE, LIB_OUT_OF_RANGE);
           }
     
         for( n = 0; n < st->system->Board[indSB].IndParCh[CopiaChList[i]].NrOfChPar; n++ )
           {
            indPar = st->system->Board[indSB].IndParCh[CopiaChList[i]].IndChPar[n];
	        if( !strcmp(pParType[indPar].Name, ParName ) )
	          break;
           }
         if( n == st->system->Board[indSB].IndParCh[CopiaChList[i]].NrOfChPar )
           {
            if (cmdstr) free(cmdstr);
			free(ChStessoInd);
			free(CopiaChList);
            return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
           }
		 
		 if( !NrChStessoInd )
		   {
            ChStessoInd[NrChStessoInd] = (ushort)CopiaChList[i];
			CopiaChList[i] = -1;
			indParNow = indPar;
			NrChStessoInd++;
			continue;
           }

		 if( indPar == indParNow )
		   {
            ChStessoInd[NrChStessoInd] = (ushort)CopiaChList[i];
			CopiaChList[i] = -1;
            NrChStessoInd++;
		   }
		}
     }

   switch( pParType[indParNow].Flag.Type )
     {
      case PARAM_TYPE_NUMERIC:
/* Rel. 1.4 - Devo gestire il caso che il par. abbia il segno */
       if( pParType[indParNow].Flag.Sign == SIGNED )
       {
         float  val = *(float *)ParValue;
         float  arr = val < (float)0.0 ? -(float)0.5 : +(float)0.5;
/* Rel. 2.0 - cast (float) per far tacere il compilatore */

         if( pParType[indParNow].Un.Car.Dec > 0 )
           Value = (unsigned long)( (val * pParType[indParNow].Un.Car.Res) + arr );		                                     
         else
           Value = (unsigned long)( (val / pParType[indParNow].Un.Car.Res) + arr );		                                     
       }
       else
       {
         if( pParType[indParNow].Un.Car.Dec > 0 )
           Value = (unsigned long)( ((*(float *)ParValue) * 
		                                     pParType[indParNow].Un.Car.Res) + 0.5 );
         else
           Value = (unsigned long)( ((*(float *)ParValue) / 
		                                     pParType[indParNow].Un.Car.Res) + 0.5 );
       }
	   break;
      case PARAM_TYPE_ONOFF:
        Value = (*(unsigned long *)ParValue);
       break;
      case PARAM_TYPE_CHSTATUS:
       break;
	  case PARAM_TYPE_BINARY:		/* Rel 2.16 */
	   Value = (*(unsigned long *)ParValue);	  
	   break;
     }
  
   pl = (unsigned long *)(cmdstr + cmdlen);
   *pl = htonl(Value);
   cmdlenloop = cmdlen + sizeof(unsigned long);
   cmdstr[cmdlenloop++]= LIB_SEP[0];
      
   pp = (unsigned short *)(cmdstr + cmdlenloop);
   *pp = htons(NrChStessoInd);
   cmdlenloop +=  sizeof(short);
   cmdstr[cmdlenloop++]= LIB_SEP[0];

   for( n = 0; n < NrChStessoInd; n++ )
     {
      pp = (unsigned short *)(cmdstr + cmdlenloop);
      *pp = htons(ChStessoInd[n]);
      cmdlenloop += sizeof(unsigned short);
     }

   if( (ret = sendReceive(st, cmdstr, cmdlenloop, &rislen, &risposta)) != SY1527_OK )
     {
      if (cmdstr) free(cmdstr);
      free(ChStessoInd);
	  free(CopiaChList);
      return ret;
     }

   FindSetAck(st, rislen, risposta);
   if( st->errCode )
     {
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      free(ChStessoInd);
	  free(CopiaChList);
      return st->errCode;
     }

   j += NrChStessoInd;
  } /* end loop casino */

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);
free(ChStessoInd);
free(CopiaChList);

return st->errCode;
}

/* SY1527TESTBDPRESENCE ******************************************************
    Command which verifies if the boards in slot Slot is present and, if it is, it
    provides some basic info
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
    Parameter Out
     Model        string containig the name of the board
     Description  string containig the description of the board
     SerialNum    Serial Number of Boad
     FmwRelMin    minor revision
     FmwRelMax    major revision
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527TestBdPresence(int SysId, unsigned short Slot,
                     char *Model, char *Description, unsigned short *SerialNum,
                            unsigned char *FmwRelMin, unsigned char *FmwRelMax)
{
int ret, rislen, index_len, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short *tser, *pfmwRel, fmwRel, *pp;
char *temp;
STATUS *st;

/* aggiorno le informazioni sul sistema e quindi la struttura interna SYSINFO */
if( (ret = Sy1527GetSysInfo(SysId)) != SY1527_OK )
  return ret;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_TESTBDPRES);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + sizeof(short) +
        (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(Slot);
cmdlen += sizeof(short);

if ( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
    {
    if (cmdstr) free(cmdstr);
    return ret;
    }

FindSetAck(st, rislen, risposta);
if( st->errCode )
   {
    if (cmdstr) free(cmdstr);
    if (risposta) free(risposta);
    return st->errCode;
   }

/* leggo il risultato */
index_len = st->headLen;
temp = risposta + index_len;
temp = strtok(temp, LIB_SEP);  /* Board Model */
strcpy(Model,temp);

index_len += strlen(Model)+strlen(LIB_SEP);
temp = risposta + index_len;
temp = strtok(temp, LIB_SEP); /* Board Descr. */
strcpy(Description,temp);

index_len += strlen(Description)+strlen(LIB_SEP);
tser = (unsigned short *)(risposta + index_len);
*SerialNum = ntohs(*tser);

index_len += sizeof(*SerialNum)+strlen(LIB_SEP);
pfmwRel = (unsigned short *)(risposta + index_len);
fmwRel = ntohs(*pfmwRel);
*FmwRelMax = fmwRel>>8;
*FmwRelMin = fmwRel&0xff;

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527GETBDPARAMINFO *******************************************************
    Function with which it is possible to know the names of the parameters of the
    board
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
    Parameter Out
     ParNameList  array of strings containing the name of the parameters of the
                  board. The last element of the array is the null string
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetBdParamInfo(int SysId, unsigned short Slot,
                                                             char **ParNameList)
{
int  i, indStr, indSB, indPar;
unsigned short NrOfPar;
char (*List)[MAX_PARAM_NAME];
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
  return SY1527_NOTPRES;

st = stNow[indStr];

/* cerco la scheda nella mia struttura interna di libreria */
for( i = 0; i < st->system->NrOfBd; i++ )
  if( st->system->Board[i].Position == Slot )
    {
	 indSB = i;
	 break;
	}

if( i == st->system->NrOfBd )
  return salvaErrore(st, SY1527_SLOTNOTPRES, LIB_SLOT_NOT_PRES);

/* cerco i parametri di scheda */
pParType = st->system->ParTypes;

NrOfPar = st->system->Board[indSB].NrOfBdPar;

List = malloc((NrOfPar + 1) * MAX_PARAM_NAME);

for( i = 0; i < NrOfPar; i++ )
  {
   indPar = st->system->Board[indSB].IndBdPar[i];
   strcpy(List[i], pParType[indPar].Name);
  }
strcpy(List[i],"");
*ParNameList = (char *)List;

st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527GETBDPARAMPROP **********************************************************
    Function to receive the value associated to the particular property of the
    parameter of the board. This can be a numeric value, a string or a bit field
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ParName      parameter name
     PropName     property name
    Parameter Out
     retval       value of the property of the parameter
   Globals
    stNow

  Rel. 2.8 - Corrected a bug in the reading of the Mode param property
 ******************************************************************************/
 SY1527RESULT Sy1527GetBdParamProp(int SysId, unsigned short Slot,
                         const char *ParName, const char *PropName, void *retval)
{
int i, indStr, indSB, indPar;
unsigned short indProp;
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* cerco la scheda nella mia struttura interna di libreria */
for( i = 0; i < st->system->NrOfBd; i++ )
  if( st->system->Board[i].Position == Slot )
    {
	 indSB = i;
	 break;
	}
if( i == st->system->NrOfBd )
  return salvaErrore(st, SY1527_SLOTNOTPRES, LIB_SLOT_NOT_PRES);

/* cerco il parametro di scheda */
pParType = st->system->ParTypes;
for( i = 0; i < st->system->Board[indSB].NrOfBdPar; i++ )
  {
   indPar = st->system->Board[indSB].IndBdPar[i];
   if( !strcmp(pParType[indPar].Name, ParName ) )
    break;
  }
if( i== st->system->Board[indSB].NrOfBdPar )
  return salvaErrore(st, SY1527_PARAMNOTFOUND, LIB_PAR_NOT_FOUND);

/* cerco la param property */
indProp = 0;
while(strcmp(ParamProp[indProp].PropIt, "NULL"))
  {
   if( !strcmp(ParamProp[indProp].PropIt, PropName) )
     break;
   indProp++;
  }
if( !strcmp(ParamProp[indProp].PropIt, "NULL") )
  return salvaErrore(st, SY1527_PARAMPROPNOTFOUND, LIB_PAR_PROP_NOT_FOUND);
  
switch( ParamProp[indProp].PropId )
  {
   case 0: /* .Type */
    *(unsigned long *)retval = pParType[indPar].Flag.Type;
    break; 

/*   case 1: // .Dir							                  // Rel. 2.8 
//    *(unsigned long *)retval = pParType[indPar].Flag.Dir;		  // Rel. 2.8 */

/* Rel. 2.8 - Corrected the decoding of the Mode Property. 
//            Correction derived form GetChParamProp */
   case 1: /* .Dir (Mode) */
    if( !pParType[indPar].Flag.Dir )
      *(unsigned long *)retval = PARAM_MODE_RDONLY;
	else if( pParType[indPar].Flag.Dir == 1 )
      *(unsigned long *)retval = PARAM_MODE_RDWR;
	else
      *(unsigned long *)retval = PARAM_MODE_WRONLY;
    break;

   case 2: /* .Sign  */
    *(unsigned long *)retval = pParType[indPar].Flag.Sign;
    break;

   case 3: /* .Min */
/* Rel. 1.4 - Devo gestire la possibilita' che il par. abbia segno */
     if( pParType[indPar].Flag.Sign == SIGNED ) 
     {
       int val = (int)pParType[indPar].Un.Car.Min;

       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)val / (float)pParType[indPar].Un.Car.Res;	                                       
       else
        *(float *)retval = (float)val * (float)pParType[indPar].Un.Car.Res;	                                      
     }
     else
     {
       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)pParType[indPar].Un.Car.Min / 
	                                       (float)pParType[indPar].Un.Car.Res;
       else
        *(float *)retval = (float)pParType[indPar].Un.Car.Min * 
	                                       (float)pParType[indPar].Un.Car.Res;
     }
    break;

   case 4: /* .Max */
/* Rel. 1.4 - Devo gestire la possibilita' che il par. abbia segno */
     if( pParType[indPar].Flag.Sign == SIGNED ) 
     {
       int val = (int)pParType[indPar].Un.Car.Max;

       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)val / (float)pParType[indPar].Un.Car.Res;	                                       
       else
        *(float *)retval = (float)val * (float)pParType[indPar].Un.Car.Res;	                                      
     }
     else
     {
       if( pParType[indPar].Un.Car.Dec > 0 )
        *(float *)retval = (float)pParType[indPar].Un.Car.Max / 
	                                       (float)pParType[indPar].Un.Car.Res;
       else
        *(float *)retval = (float)pParType[indPar].Un.Car.Max * 
	                                       (float)pParType[indPar].Un.Car.Res;
     }
    break;

   case 5: /* .Unit */
    *(unsigned short *)retval = pParType[indPar].Un.Car.Um;
    break;

   case 6: /* .Exp */
    *(short *)retval = pParType[indPar].Un.Car.Exp;
    break;

   case 7: /* On */
    strcpy((char *)retval,pParType[indPar].Un.Str.On);
    break;

   case 8: /* Off */
    strcpy((char *)retval,pParType[indPar].Un.Str.Off);
    break;
  }   

st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527GETBDPARAM ***********************************************************
   Command to receive the value of parameter "ParName" of the board in slot Slot
   Parameter In
    SysId        Sistem ID
    NrOfSlot     number of slot
    SlotList     list of NrOfSlot Slot identifies in terms of crate (high byte)
                 and slot (low byte)
    ParName      parameter name
   Parameter Out
    ParVal       parameter value
   Globals
    stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetBdParam(int SysId, unsigned short NrOfSlot,
           const unsigned short *SlotList, const char *ParName, void *ParValList)
{
int ret, n, i, j, rislen, indStr, indSB, indPar, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *pc, *risposta = (char *)NULL;
unsigned short *pp;
unsigned long *pl, ParValue;
PARAM_TYPE *pParType;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(G_BDPARAM);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + sizeof(short) + NrOfSlot * sizeof(short) +
         strlen(ParName) + (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

pc = cmdstr + cmdlen;
strcpy(pc, ParName);
cmdlen += strlen(ParName);
cmdstr[cmdlen++]= LIB_SEP[0];

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(NrOfSlot);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( i = 0; i < NrOfSlot; i++ )
  {
   pp = (unsigned short *)(cmdstr + cmdlen);
   *pp = htons(SlotList[i]);
   cmdlen += sizeof(unsigned short);
  }

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return st->errCode;
  }

/* cerco il parametro di scheda */
pParType = st->system->ParTypes;
pl = (unsigned long *)(risposta + st->headLen);
for( i = 0; i < NrOfSlot; i++, pl++ )
  {
   /* cerco la scheda nella mia struttura interna di libreria */
   for( j = 0; j < st->system->NrOfBd; j++ )
     if( st->system->Board[j].Position == SlotList[i] )
       {
	    indSB = j;
	    break;
	   }
   if( j == st->system->NrOfBd )
     {
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
     }

   for( n = 0; n < st->system->Board[indSB].NrOfBdPar; n++ )
     {
      indPar = st->system->Board[indSB].IndBdPar[n];
	  if( !strcmp(pParType[indPar].Name, ParName ) )
	    break;
     }
   if( n == st->system->Board[indSB].NrOfBdPar )
     {
      if (cmdstr) free(cmdstr);
      if (risposta) free(risposta);
      return salvaErrore(st, SY1527_SYSCONFCHANGE, LIB_SYS_CONF_CHANGE);
     }

   /* ora che ho il parametro posso fare le conversioni */
   ParValue = ntohl(*pl);
   switch( pParType[indPar].Flag.Type )
     {
	  case PARAM_TYPE_NUMERIC:
/* Rel. 1.4 - Devo gestire la possibilita' che il par. abbia segno */
     if( pParType[indPar].Flag.Sign == SIGNED ) 
     {
       int val = (int)ParValue;

  	   if( pParType[indPar].Un.Car.Dec > 0 )
           ((float *)ParValList)[i] = (float)val / 
		                                       (float)pParType[indPar].Un.Car.Res;
	    else
           ((float *)ParValList)[i] = (float)val * 
		                                       (float)pParType[indPar].Un.Car.Res;
     }
     else
     {
  	   if( pParType[indPar].Un.Car.Dec > 0 )
           ((float *)ParValList)[i] = (float)ParValue / 
		                                       (float)pParType[indPar].Un.Car.Res;
	    else
           ((float *)ParValList)[i] = (float)ParValue * 
		                                       (float)pParType[indPar].Un.Car.Res;
     }
     break;
    case PARAM_TYPE_ONOFF:
	   ((long *)ParValList)[i] = ParValue;
	   break;
	  case PARAM_TYPE_BDSTATUS:
	   ((long *)ParValList)[i] = ParValue;
	   break;
	   case PARAM_TYPE_BINARY:		/* Rel 2.16 */
	   ((long *)ParValList)[i] = ParValue;	  
		break;
	 }   
  }

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527SETBDPARAM ***********************************************************
  Command to assign a new value to parameter "ParName" of the board in slot Slot
   Parameter In
    SysId        Sistem ID
    NrOfSlot     number of slot
    SlotList     list of NrOfSlot Slot identifies in terms of crate (high byte)
                 and slot (low byte)
    ParName      parameter name
    ParVal       parameter value

  Rel. 2.8 - Introduced
 ******************************************************************************/
 SY1527RESULT Sy1527SetBdParam(int SysId, unsigned short NrOfSlot,
            const unsigned short *SlotList, const char *ParName, void *ParValue)
{
	int            bd, ret, i, rislen, indStr, cmdlen = 0, CMDLEN = 0;
	char           *cmdstr = (char *)NULL;
	char           *pc, *risposta = (char *)NULL;
	unsigned short *pp;
	unsigned short indSB, indPar;
	unsigned long  *pl, Value;
	PARAM_TYPE     *pParType;
	STATUS         *st;

/* trovo la stNow associata al sistema tramite il suo indice */
	if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
		return SY1527_NOTPRES;

	st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
	st->cmdNow = FindCmdInfo(S_BDPARAM);

/* in caso di logout l'header == comando */
	doHeader(st, &cmdstr, &cmdlen);

	CMDLEN = cmdlen;

	cmdstr = realloc(cmdstr, cmdlen + sizeof(unsigned short) + strlen(ParName) 
									+ (st->cmdNow->numparIn -1) * strlen(LIB_SEP) 
									+ sizeof(float));

	pParType = st->system->ParTypes;

	for( bd = 0 ; bd < NrOfSlot ; bd++ )
	{
		cmdlen = CMDLEN;
		pp = (unsigned short *)(cmdstr + CMDLEN);
		*pp = htons(SlotList[bd]);
		cmdlen += sizeof(short);
		cmdstr[cmdlen++]= LIB_SEP[0];

		pc = cmdstr + cmdlen;
		strcpy(pc, ParName);
		cmdlen += strlen(ParName);
		cmdstr[cmdlen++]= LIB_SEP[0];

/* cerco la scheda nella mia struttura interna di libreria */
		for( i = 0; i < st->system->NrOfBd; i++ )
			if( st->system->Board[i].Position == SlotList[bd] )
			{
				indSB = i;
				break;
			}

		if( i == st->system->NrOfBd )
		{
			if( cmdstr ) 
				free(cmdstr);
			return salvaErrore(st, SY1527_SYSCONFCHANGE, 
								   LIB_SYS_CONF_CHANGE);
		}

/* cerco se la scheda ha quel parametro */
		for( i = 0; i < st->system->Board[indSB].NrOfBdPar; i++ )
		{
			indPar = st->system->Board[indSB].IndBdPar[i];
			if( !strcmp(pParType[indPar].Name, ParName ) )
				break;
		}

		if( i == st->system->Board[indSB].NrOfBdPar )
		{
			if( cmdstr) 
				free(cmdstr);
			return salvaErrore(st, SY1527_SYSCONFCHANGE, 
								   LIB_SYS_CONF_CHANGE);
		}

		switch( pParType[indPar].Flag.Type )
		{
			case PARAM_TYPE_NUMERIC:
				if( pParType[indPar].Flag.Sign == SIGNED )
				{
					float  val = *(float *)ParValue;
					float  arr = val < (float)0.0 ? -(float)0.5 : +(float)0.5;

					if( pParType[indPar].Un.Car.Dec > 0 )
						Value = (unsigned long)( (val * pParType[indPar].Un.Car.Res) + arr );		                                     
					else
						Value = (unsigned long)( (val / pParType[indPar].Un.Car.Res) + arr );		                                     
				}
				else
				{
					if( pParType[indPar].Un.Car.Dec > 0 )
						Value = (unsigned long)( ((*(float *)ParValue) * 
										pParType[indPar].Un.Car.Res) + 0.5 );
					else
						Value = (unsigned long)( ((*(float *)ParValue) / 
										pParType[indPar].Un.Car.Res) + 0.5 );
				}
				break;

			case PARAM_TYPE_ONOFF:
				Value = (*(unsigned long *)ParValue);
				break;

			case PARAM_TYPE_BDSTATUS:
				break;
			case PARAM_TYPE_BINARY:		/* Rel 2.16 */
				Value = (*(unsigned long *)ParValue);	  
				break;
		}
  
		pl = (unsigned long *)(cmdstr + cmdlen);
		*pl = htonl(Value);
		cmdlen += sizeof(unsigned long);
      
		ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta);
		if( ret != SY1527_OK )
		{
			if( cmdstr ) 
				free(cmdstr);
			return ret;
		}

		FindSetAck(st, rislen, risposta);
		if( st->errCode )
		{
			if( cmdstr ) 
				free(cmdstr);
			if( risposta ) 
				free(risposta);
			return st->errCode;
		}
	} /* end for bd */

/* libero la memoria allocata dalle funzioni precendenti */
	if( cmdstr ) 
		free(cmdstr);
	if( risposta ) 
		free(risposta);

	return st->errCode;
}

/* SY1527GETCRATEMAP **********************************************************
    Command which verifies the Sy1527's boards population
    Parameter In
     SysId            Sistem ID
    Parameter Out
     NrOfSl           number of slot of system
     ModelList        list of Model or ""
     DescriptionList  list of description. Null if board not present
     SerNumList       list of serial number. 0 if board not present
     FmwRelMinList    list minor revision. 0 if board not present
     FmwRelMaxList    list of major revision. 0 if board not present
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetCrateMap(int SysId, unsigned short *NrOfSl,
         char **ModelList, char **DescriptionList, unsigned short **SerNumList,
                  unsigned char **FmwRelMinList, unsigned char **FmwRelMaxList)
{
int ret, rislen, index_len, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short i, fmwRel, *ps, ModelLen, DescrLen;
char *pc, *temp;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
  return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_GETCRATEMAP);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return st->errCode;
  }

/* leggo il risultato */
index_len = st->headLen;
ps = (ushort *)(risposta + index_len);
*NrOfSl = ntohs(*ps);        /* numero di slot totali */
index_len += sizeof(short);

*ModelList = (char *)NULL;
*DescriptionList = (char *)NULL;
*SerNumList = malloc((*NrOfSl)*sizeof(unsigned short));
*FmwRelMinList = malloc((*NrOfSl)*sizeof(unsigned char));
*FmwRelMaxList = malloc((*NrOfSl)*sizeof(unsigned char));
ModelLen = DescrLen = 0;

for( i = 0; i < *NrOfSl; i++ )
  {
   index_len++;               /* separetor */
   pc = risposta + index_len;
   if( *pc == '\0' )
     {
	  *ModelList = realloc(*ModelList,ModelLen + strlen("") + 1);
      memcpy(*ModelList + ModelLen, "", strlen("") + 1);
	  ModelLen += strlen("") + 1;
      *DescriptionList = realloc(*DescriptionList, DescrLen + 1);
      memcpy(*DescriptionList + DescrLen, pc, 1);
	  DescrLen++;
      (*SerNumList)[i] = 0;
      (*FmwRelMinList)[i] = 0;
      (*FmwRelMaxList)[i] = 0;
      index_len++;
     }
   else
     {
	  temp = pc;                       /* Board Model */
	  temp = strtok(temp, LIB_SEP );
	  *ModelList = realloc(*ModelList,ModelLen + strlen(temp) + 1);
      memcpy(*ModelList + ModelLen, temp, strlen(temp) + 1);
	  ModelLen += strlen(temp) + 1;

	  index_len += strlen(temp) + strlen(LIB_SEP); /* Board Descr. */
      temp = risposta + index_len;
	  temp = strtok(temp, LIB_SEP );
      *DescriptionList = realloc(*DescriptionList, DescrLen + strlen(temp) + 1);
      memcpy(*DescriptionList + DescrLen, temp, strlen(temp) + 1);
	  DescrLen += strlen(temp) + 1;

      index_len += strlen(temp) + strlen(LIB_SEP);
	  ps = (unsigned short *)(risposta + index_len);
      (*SerNumList)[i] = ntohs(*ps);

	  index_len += sizeof(*SerNumList[i])+strlen(LIB_SEP);
	  ps = (unsigned short *)(risposta + index_len);
      fmwRel = ntohs(*ps);
	  (*FmwRelMaxList)[i] = fmwRel>>8;
	  (*FmwRelMinList)[i] = fmwRel&0xff;

	  index_len += sizeof(unsigned short);
     }
  }

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

/* aggiorno le informazioni sul sistema e quindi la struttura interna SYSINFO */
Sy1527GetSysInfo(SysId);

return st->errCode;
}

/* SY1527GETGRPCOMP ***********************************************************
    Command to get the group composition in term of pairs (Slot, ChInSlot)
    Parameter In
     SysId        Sistem ID
     Group        group number
    Parameter Out
     NrOfCh       number of channels
     ChList       list of NrOfCh channels identified by Slot (two MSB: crate (high byte),
                  slot(low byte)) e ChInSlot (two LSB: channel)
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetGrpComp(int SysId, unsigned short Group,
                                unsigned short *NrOfCh, unsigned long **ChList)
{
int ret, rislen, index_len, indStr, i, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short *ps;
unsigned long *pl, *app;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(G_GRPCOMP);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + sizeof(short) +
        (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(Group);
cmdlen += sizeof(short);

if ( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
    {
    if (cmdstr) free(cmdstr);
    return ret;
    }

FindSetAck(st, rislen, risposta);
if( st->errCode )
   {
    if (cmdstr) free(cmdstr);
    if (risposta) free(risposta);
    return st->errCode;
   }

/* leggo il risultato */
index_len = st->headLen;
ps = (unsigned short *)(risposta + index_len);
*NrOfCh = ntohs(*ps);

*ChList = (unsigned long *)malloc((*NrOfCh)*sizeof(unsigned long));
app = *ChList;
index_len += sizeof(short)+strlen(LIB_SEP);
pl = (unsigned long *)(risposta + index_len);
for( i = 0; i < *NrOfCh; i++, pl++ )
  app[i] = ntohl(*pl);
  /* oppure *ChList[i] = ntohl(*pl); */

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527ADDCHTOGRP ***********************************************************
    Command to add to the group the channels indicated in ChLIst
    Parameter In
     SysId        Sistem ID
     Group        group number
     NrOfCh       number of channels
     ChList       list of NrOfCh channels identified by Slot (two MSB: 
                  crate (high byte), lot(low byte)) e ChInSlot (two LSB: channel)
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527AddChToGrp(int SysId, unsigned short Group,
                              unsigned short NrOfCh, const unsigned long *ChList)
{
int ret, rislen, indStr, i, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short *ps;
unsigned long *pl;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_ADDCHGRP);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + 2 * sizeof(short) + NrOfCh * sizeof(long) +
                              (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(Group);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(NrOfCh);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( i = 0; i < NrOfCh; i++ )
  {
   pl = (unsigned long *)(cmdstr + cmdlen);
   *pl = htonl(ChList[i]);
   cmdlen += sizeof(unsigned long);
  }

if ( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
    {
    if (cmdstr) free(cmdstr);
    return ret;
    }

FindSetAck(st, rislen, risposta);

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527REMCHTOGRP ***********************************************************
    Command to remove from the group the channels indicated in ChLIst
    Parameter In
     SysId        Sistem ID
     Group        group number
     NrOfCh       number of channels
     ChList       list of NrOfCh channels identified by Slot (two MSB: crate (high byte),
                  slot(low byte)) e ChInSlot (two LSB: channel)
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527RemChToGrp(int SysId, unsigned short Group,
                              unsigned short NrOfCh, const unsigned long *ChList)
{
int ret, rislen, indStr, i, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short *ps;
unsigned long *pl;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_REMCHGRP);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + 2 * sizeof(short) + NrOfCh * sizeof(long) +
                              (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(Group);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(NrOfCh);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( i = 0; i < NrOfCh; i++ )
  {
   pl = (unsigned long *)(cmdstr + cmdlen);
   *pl = htonl(ChList[i]);
   cmdlen += sizeof(unsigned long);
  }

if ( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
    {
    if (cmdstr) free(cmdstr);
    return ret;
    }

FindSetAck(st, rislen, risposta);

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527GETSYSCOMP ***********************************************************
   Command which gets info about the system connected in a cluster via LocalNet.
   (number, list of crate(crate,number of slot), number of channel for each 
   slot)
    Parameter In
     SysId            Sistem ID
    Parameter Out
     NrOfCr           number of slot of system
     CrateList        list of crate(Crate (MSB), nr. of Slot(LSB))
     NrOfChList       list of Nr. of Ch. for each Slot. They are Nr of total Slot.
	                  Slot (two bytes (MSB)) and Nr. of Ch (two bytes (LSB))
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetSysComp(int SysId, unsigned char *NrOfCr,
                        unsigned short **CrNrOfSlList, unsigned long **SlotChList)
{
int ret, rislen, index_len, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short i, *ps, NrOfSlot;
unsigned long *pl;
char *pc;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
  return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(G_SYSCOMP);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return st->errCode;
  }

/* leggo il risultato */
index_len = st->headLen;
pc = risposta + index_len;
*NrOfCr = *pc;
index_len += sizeof(char) +1;

*CrNrOfSlList = (unsigned short *)NULL;
*CrNrOfSlList = malloc((*NrOfCr)*sizeof(unsigned short));
NrOfSlot = 0;
for( i = 0; i < *NrOfCr; i++ )
  {
   ps = (ushort *)(risposta + index_len);
   (*CrNrOfSlList)[i] = ntohs(*ps);
   NrOfSlot += ((*CrNrOfSlList)[i])&0xff;
   index_len += sizeof(unsigned short);
  }
index_len++;

*SlotChList = (unsigned long *)NULL;
*SlotChList = malloc(NrOfSlot*sizeof(unsigned long));
for( i = 0; i < NrOfSlot; i++ )
  {
   pl = (unsigned long *)(risposta + index_len);
   (*SlotChList)[i] = ntohl(*pl);
   index_len += sizeof(unsigned long);
  }

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527GETEXECCOMMANDLIST ***************************************************
    It returns the commands which can be executed by SY1527ExecComm
    Parameter In
     SysId        Sistem ID
    Parameter Out
     NrOfComm     number of commands
     CommNameList list of NrOfComm name of the commands
	Globals
	 GSEList[]
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetExecCommList(int SysId,
                                  unsigned short *NrOfComm, char **CommNameList)
{
int indStr, i = 0;
unsigned short ListLen = 0, Len;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

*CommNameList = (char *)NULL;
*NrOfComm = 0;
while( GSEList[i].GetSetExec != -1 )
  {
   if( GSEList[i].GetSetExec == LIBEXEC )
     {
	  Len = strlen(GSEList[i].Item) + 1;
	  *CommNameList = realloc(*CommNameList,ListLen + Len);
	  memcpy(*CommNameList + ListLen,GSEList[i].Item, Len);
	  ListLen += Len;
	  (*NrOfComm)++;
	 }
   i++;
  }


st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527EXECCOMM *************************************************************
    It execute the command specified in CommName
    Parameter In
     SysId       Sistem ID
     CommName    command name
	Globals
	 GSEList[]
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527ExecComm(int SysId, const char *CommName)
{
int ret, rislen, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
STATUS *st;
GETSETEXE *gse;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

if( (gse = PropExecSearch(SysId, CommName)) == (GETSETEXE *)NULL )
  return salvaErrore(st, SY1527_EXECNOTFOUND, LIB_EXEC_NOT_FOUND);

if( gse->GetSetExec != LIBEXEC )
  return salvaErrore(st, SY1527_NOTEXECOMM, LIB_NOT_EXEC_COMM);

if( !gse->Impl )
  return salvaErrore(st, SY1527_EXECCOMNOTIMPL, LIB_EXEC_NOT_IMPL);

switch(gse->CommId)
  {
   case 1:  /* Kill */
    st->cmdNow = FindCmdInfo(O_KILL_SURE); /* Kill parte prima */

    /* in caso di logout l'header == comando */
    doHeader(st, &cmdstr, &cmdlen);

    if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
      {
       if(cmdstr) free(cmdstr);
       return ret;
      }

    FindSetAck(st, rislen, risposta);
    if( st->errCode )
      {
       if(cmdstr) free(cmdstr);
       if(risposta) free(risposta);
       return st->errCode;
      }

    /* libero la memoria allocata dalle funzioni precendenti */
    if(cmdstr) free(cmdstr);
    if(risposta) free(risposta);

    /* Kill parte seconda */
    st->cmdNow = FindCmdInfo(O_KILL_OK); /* Kill parte prima */

    /* in caso di logout l'header == comando */
    doHeader(st, &cmdstr, &cmdlen);

    if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
      {
       if(cmdstr) free(cmdstr);
       return ret;
      }
    break;

   case 2:  /* Clear Alarm */
    st->cmdNow = FindCmdInfo(O_CLRALARM_SURE); /* Clear parte prima */

    /* in caso di logout l'header == comando */
    doHeader(st, &cmdstr, &cmdlen);

    if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
      {
       if(cmdstr) free(cmdstr);
       return ret;
      }

    FindSetAck(st, rislen, risposta);
    if( st->errCode )
      {
       if(cmdstr) free(cmdstr);
       if(risposta) free(risposta);
       return st->errCode;
      }

    /* libero la memoria allocata dalle funzioni precendenti */
    if(cmdstr) free(cmdstr);
    if(risposta) free(risposta);

    /* Kill parte seconda */
    st->cmdNow = FindCmdInfo(O_CLRALARM_OK); /* Kill parte seconda */

    /* in caso di logout l'header == comando */
    doHeader(st, &cmdstr, &cmdlen);

    if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
      {
       if(cmdstr) free(cmdstr);
       return ret;
      }
    break;

   default:
    st->cmdNow = FindCmdInfo(gse->GetComm);

    /* in caso di logout l'header == comando */
    doHeader(st, &cmdstr, &cmdlen);

    if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
      {
       if (cmdstr) free(cmdstr);
       return ret;
      }
    break;
  }  

FindSetAck(st, rislen, risposta);

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);
 
return st->errCode;
}

/* SY1527GETSYSPROPLIST -------------------------------------------------------
    It returns the properties which can be executed by Sy1527Get/SetSysProp
    Parameter In
     SysId        Sistem ID
    Parameter Out
     NrOfProp     number of properties
	Globals
	 GSEList[]
     PropNameList list of NrOfProp name of the properties
 *----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetSysPropList(int SysId,
                                 unsigned short *NrOfProp, char **PropNameList)
{
int indStr, i = 0;
unsigned short ListLen = 0, Len;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

*PropNameList = (char *)NULL;
*NrOfProp = 0;
while( GSEList[i].GetSetExec != -1 )
  {
   if( GSEList[i].GetSetExec != LIBEXEC )
     {
	  Len = strlen(GSEList[i].Item) + 1;
	  *PropNameList = realloc(*PropNameList,ListLen + Len);
	  memcpy(*PropNameList + ListLen,GSEList[i].Item, Len);
	  ListLen += Len;
	  (*NrOfProp)++;
	 }
   i++;
  }

st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527GETSYSPROPINFO *******************************************************
    It returns for each properties of system, the type of variable (to set or 
	 returned) and property mode (read,write or read/write)
	use
    Parameter In
     SysId        Sistem ID
     PropSysName  name of system property
    Parameter Out
	 Mode         type of property: property of read, write or read/write
	 Type         type of variable: string, ushort, short, real, int, uint, boolean

Ver 1.01L modificato il case 17
 ******************************************************************************/
 SY1527RESULT Sy1527GetSysPropInfo(int SysId,
                       const char *PropSysName, unsigned *Mode, unsigned *Type)
{
int indStr;
STATUS *st;
GETSETEXE *gse;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

if( (gse = PropExecSearch(SysId, PropSysName)) == (GETSETEXE *)NULL )
  return salvaErrore(st, SY1527_PROPNOTFOUND, LIB_PROP_NOT_FOUND);

if( gse->GetSetExec != LIBGET && gse->GetSetExec != LIBGETSET )
  return salvaErrore(st, SY1527_NOTSYSPROP, LIB_NOT_SYS_PROP);

switch(gse->CommId)
  {
   case 0:   /* Sessions */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 7:   /* ModelName: solo nome del modello di sistema */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 8:   /* SwRelease */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 9:   /* GenSignCfg */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_UINT2;
	break;
   case 10:  /* FrontPanIn */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_UINT2;
    break;
   case 11:  /* FrontPanOut */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_UINT2;
    break;
   case 12:  /* ResFlagCfg */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_UINT2;
    break;
   case 13:  /* ResFlag */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_UINT2;
    break;
   case 14:  /* HvPwSM */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 15:  /* FanStat */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 16:  /* ClkFreq */
    *Mode = SYSPROP_MODE_RDONLY;
	*Type = SYSPROP_TYPE_INT2;
    break;
   case 17:  /* HVClkConf */
/* Ver. 1.01L in realta' e' anche di set *Mode = SYSPROP_MODE_RDONLY; */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 18:  /* IPAddr */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 19:  /* IPNetMsk */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 20:  /* IPGw */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 21:  /* RS232Par */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_STR;
    break;
   case 22:  /* CnetCrNum */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_UINT2;
    break;
   case 23:  /* SymbolicName: nome simbolico del Sistema */
    *Mode = SYSPROP_MODE_RDWR;
	*Type = SYSPROP_TYPE_STR;
    break;
  }  
 
st->errCode = SY1527_OK;
return st->errCode;
}

/* SY1527GETSYSPROP ***********************************************************
    It returns the value of the property PropName
    Parameter In
     SysId        Sistem ID
     PropName     property name
    Parameter Out
     result
    Globals
     stNow
 ******************************************************************************/
 SY1527RESULT Sy1527GetSysProp(int SysId, const char *PropName,
                                                                   void *result)
{
int ret, rislen, indStr, index_len, indexRif, i, y, resultLen, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *pc, buff[80], *risposta = (char *)NULL;
short sApp;
ushort *ps;
STATUS *st;
GETSETEXE *gse;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

if( (gse = PropExecSearch(SysId, PropName)) == (GETSETEXE *)NULL )
  return salvaErrore(st, SY1527_PROPNOTFOUND, LIB_PROP_NOT_FOUND);

if( gse->GetSetExec != LIBGET && gse->GetSetExec != LIBGETSET )
  return salvaErrore(st, SY1527_NOTGETPROP, LIB_NOT_GET_PROP);

if( !gse->Impl )
  return salvaErrore(st, SY1527_GETPROPNOTIMPL, LIB_PROP_NOT_IMPL);

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(gse->GetComm);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   return st->errCode;
  }

index_len = st->headLen;

switch(gse->CommId)
  {
   case 0:   /* GETACTLOG */
    ps = (ushort *)(risposta + index_len);
    sApp = ntohs(*ps);
	index_len += sizeof(short) + strlen(LIB_SEP);
    indexRif = index_len;
 
    resultLen = 0;
	for( i = 0; i < sApp; i++ )
	  {
       pc = risposta + index_len;
	   pc = strtok(pc, LIB_SEP ); /* user */
       memcpy((char *)result + resultLen, pc, strlen(pc));
	   resultLen += strlen(pc);
	   *((char *)result + resultLen) = LIB_SEP[0];
	   resultLen++;
	   index_len = indexRif + resultLen;

       pc = risposta + index_len;
	   pc = strtok(pc, LIB_SEP ); /* level */
       memcpy((char *)result + resultLen, pc, strlen(pc));
	   resultLen += strlen(pc);
	   *((char *)result + resultLen) = LIB_SEP[0];
	   resultLen++;
	   index_len = indexRif + resultLen;

       pc = risposta + index_len;
	   pc = strtok(pc, LIB_SEP ); /* interface */
       memcpy((char *)result + resultLen, pc, strlen(pc));
	   resultLen += strlen(pc);
	   *((char *)result + resultLen) = LIB_SEP[0];
	   resultLen++;
	   index_len = indexRif + resultLen;

       pc = risposta + index_len;
       *strchr(pc, '\n' ) = '\0'; /* metto \0 al posto di \n: login time */
       memcpy((char *)result + resultLen, pc, strlen(pc));
       resultLen += strlen(pc);	      

	   *((char *)result + resultLen) = '\n';
	   resultLen++;
	   indexRif++; /* +1 x' ci sono i : che in result non si mettono */
	   index_len = indexRif + resultLen ; 
      }                                     
    *((char *)result + resultLen) = '\0';
    break;
#ifdef pippo /* se alloco io allora devo un puntatore a puntatore */
   case 0:   /* GETACTLOG */
    ps = (short *)(risposta + index_len);
    sApp = ntohs(*ps);
	index_len += sizeof(short) + strlen(LIB_SEP);
    indexRif = index_len;
 
    resultLen = 0;
	*(char **)result = (char *)NULL;

	for( i = 0; i < sApp; i++ )
	  {
       pc = risposta + index_len;
	   pc = strtok(pc, LIB_SEP ); /* user */
	   *(char **)result = realloc(*(char **)result, resultLen + strlen(pc) + 1);
       memcpy(*(char **)result + resultLen, pc, strlen(pc));
	   resultLen += strlen(pc);
	   *(*(char **)result + resultLen) = LIB_SEP[0];
	   resultLen++;
	   index_len = indexRif + resultLen;

       pc = risposta + index_len;
	   pc = strtok(pc, LIB_SEP ); /* level */
	   *(char **)result = realloc(*(char **)result, resultLen + strlen(pc) + 1);
       memcpy(*(char **)result + resultLen, pc, strlen(pc));
	   resultLen += strlen(pc);
	   *(*(char **)result + resultLen) = LIB_SEP[0];
	   resultLen++;
	   index_len = indexRif + resultLen;

       pc = risposta + index_len;
	   pc = strtok(pc, LIB_SEP ); /* interface */
	   *(char **)result = realloc(*(char **)result, resultLen + strlen(pc) + 1);
       memcpy(*(char **)result + resultLen, pc, strlen(pc));
	   resultLen += strlen(pc);
	   *(*(char **)result + resultLen) = LIB_SEP[0];
	   resultLen++;
	   index_len = indexRif + resultLen;

       pc = risposta + index_len;
       *strchr(pc, '\n' ) = '\0'; /* metto \0 al posto di \n: login time */
       *(char **)result = realloc(*(char **)result, resultLen + strlen(pc) + 1);
       memcpy(*(char **)result + resultLen, pc, strlen(pc));
       resultLen += strlen(pc);	      

	   *(*(char **)result + resultLen) = '\n';
	   resultLen++;
	   indexRif++; /* +1 x' ci sono i : che in result non si mettono */
	   index_len = indexRif + resultLen ; 
      }                                     
    *(char **)result = realloc(*(char **)result, resultLen + 1);
    *(*(char **)result + resultLen) = '\0';
    break;
#endif
   case 7:   /* GETSYSNAME: solo nome del modello di sistema */
    pc = risposta + index_len;
    memcpy(buff,pc,rislen-index_len); /* x' non e' null terminata !!! */
	strtok(buff,",");
    memcpy((char *)result, buff, strlen(buff) + 1);
    break;
   case 8:   /* GETSWREL */
#ifdef pippo
    index_len = stNow->headLen;
    temp = risposta + index_len;
    memcpy(*SwRel,temp,rislen-index_len); /* x' non e' null terminata !!! */
    (*SwRel)[rislen-index_len] = '\0';
#endif    
    pc = risposta + index_len;
    memcpy((char *)result,pc,rislen-index_len); /* x' non e' null terminata !!! */
    *((char *)result + rislen - index_len) = '\0';
    break;
   case 9:   /* GETGENCFG */
    pc = risposta + index_len;
    *(unsigned short *)result = *pc;    
    break;
   case 10:  /* GETFPIN */
   case 11:  /* GETFPOUT */
    ps = (ushort *)(risposta + index_len);
    *(unsigned short *)result = ntohs(*ps);
    break;
   case 12:  /* GETRSFLAGCFG */
    pc = risposta + index_len;
    *(unsigned short *)result = *pc;    
    break;
   case 13:  /* GETRSFLAG */
    break;
   case 14:  /* GETHVPSM */
    pc = risposta + index_len;
	for( y = 0, i = 0; i < 5; i++, pc++ )
	  {/* onverte il dato in stringa per essere visualizato */
       sprintf(((char *)result + y), y ? ":%d" : "%d", *pc);
       y += sprintf(buff, y ? ":%d" : "%d", *pc); 
	  }
#ifdef pippo
    pc = risposta + index_len;
	for( i = 0; i < 5; i++, pc++ )
      *((char *)result + i) = *pc; 
#endif
    break;
   case 15:  /* GETFANSTAT */
    pc = risposta + index_len;
	for( y = 0, i = index_len; i < rislen; pc += 3 )
	  {/* converte il dato in stringa per essere visualizato */
       sprintf(((char *)result + y), y ? ":%d" : "%d", *pc);
       y += sprintf(buff,y ? ":%d" : "%d", *pc); 
	   i++;
	   ps = (unsigned short *)(pc + 1);
	   sprintf(((char *)result + y),":%d", ntohs(*ps));
	   y += sprintf(buff,":%d", ntohs(*ps));
	   i += sizeof(unsigned short);
	  }
#ifdef pippo 
    pc = risposta + index_len;
	for( i = 0; i < 12; i += 2, pc += 3 )
	  {
       *((short *)result + i) = *pc; 
	   ps = (unsigned short *)(pc + 1);
       *((short *)result + i + 1) = ntohs(*ps); 
	  }
#endif
    break;
   case 16:  /* GETCLKFREQ */
#ifdef pippo
    pc = risposta + index_len;
    sprintf((char *)result,"%d", *pc);
#endif
    pc = risposta + index_len;
    *( short *)result = *pc;    
    break;
   case 17:  /* GETHVCLKCFG */
    y = 0;
    pc = risposta + index_len;
    sprintf((char *)result,"%d:", *pc);
    y += sprintf(buff,"%d:", *pc); 
	pc++;
    sprintf(((char *)result + y), "%d", *pc);
#ifdef pippo
    pc = risposta + index_len;
    *(short *)result = (*pc)<<8;  /* master o slave */
	pc++;
    *(short *)result += *pc;	  /* valid or not valid? */
#endif
    break;
   case 18:  /* GETIPADDR */
    pc = risposta + index_len;
    memcpy((char *)result,pc,rislen-index_len); /* x' non e' null terminata !!! */
    *((char *)result + rislen - index_len) = '\0';
    break;
   case 19:  /* GETIPNETMSK */
    pc = risposta + index_len;
    memcpy((char *)result,pc,rislen-index_len); /* x' non e' null terminata !!! */
    *((char *)result + rislen - index_len) = '\0';
    break;
   case 20:  /* GETIPGWY */
    pc = risposta + index_len;
    memcpy((char *)result,pc,rislen-index_len); /* x' non e' null terminata !!! */
    *((char *)result + rislen - index_len) = '\0';
    break;
   case 21:  /* GETRS232PARAM */
    pc = risposta + index_len;
    memcpy((char *)result,pc,rislen-index_len); /* x' non e' null terminata !!! */
    *((char *)result + rislen - index_len) = '\0';
    break;
   case 22:  /* GETCNETCRNUM */
#ifdef pippo
    ps = (short *)(risposta + index_len);
    sprintf((char *)result,"%d", ntohs(*ps));
#endif
    ps = (ushort *)(risposta + index_len);
    *(unsigned short *)result = ntohs(*ps);
    break;
   case 23:  /* SymbolicName: nome simbolico del Sistema */
    pc = risposta + index_len;
    memcpy(buff,pc,rislen-index_len); /* x' non e' null terminata !!! */
    *(buff + rislen - index_len) = '\0';
	strtok(buff,",");
    memcpy((char *)result, buff+strlen(buff)+1, rislen-index_len-strlen(buff));
    break;
  }  

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);
 
return st->errCode;
}

/* SY1527SETSYSPROP ***********************************************************
    Function to set the Property PropName
    Parameter In
     SysId     Sistem ID
     PropName  property name
     set
    Globals
     stNow

 Ver. 1.01L sostituita strcpy con memcpy in alcuni punti critici. Aggiunto il
           settaggio dell' HV clock
 ******************************************************************************/
 SY1527RESULT Sy1527SetSysProp(int SysId, const char *PropName,
                                                                     void *set)
{
int ret, rislen, indStr, cmdlen = 0;
char *pc;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
ushort *ps;
unsigned short master;   /* ver. 1.01L */
STATUS *st;
GETSETEXE *gse;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

if( (gse = PropExecSearch(SysId, PropName)) == (GETSETEXE *)NULL )
  return salvaErrore(st, SY1527_PROPNOTFOUND, LIB_PROP_NOT_FOUND);

if( gse->GetSetExec != LIBSET && gse->GetSetExec != LIBGETSET )
  return salvaErrore(st, SY1527_NOTSETPROP, LIB_NOT_SET_PROP);

if( !gse->Impl )
  return salvaErrore(st, SY1527_GETPROPNOTIMPL, LIB_PROP_NOT_IMPL);

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(gse->SetComm);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

switch(gse->CommId)
  {
   case 9:  /* SETGENCFG */
    cmdstr = realloc(cmdstr, cmdlen + sizeof(unsigned short) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
    *pc = (*(unsigned short *)set)>>8; /* Gen Mask; */
	pc++;
	*pc = LIB_SEP[0];
	pc++;
	*pc = (*(unsigned short *)set)&0xff; /* Gen Set */
    cmdlen += 2*sizeof(char) + 1;
    break;
   case 12: /* SETRSFLAGCFG */
    cmdstr = realloc(cmdstr, cmdlen + sizeof(unsigned short) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
    *pc = (*(unsigned short *)set)>>8; /* Res Mask; */
	pc++;
	*pc = LIB_SEP[0];
	pc++;
	*pc = (*(unsigned short *)set)&0xff; /* Res Set */
    cmdlen += 2*sizeof(char) + 1;
    break;
   case 17:  /* SETHVCLKCFG    ver. 1.01L */
    cmdstr = realloc(cmdstr, cmdlen + sizeof(unsigned short) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    master = (unsigned short)atoi((char *)set);
    ps = (unsigned short *)(cmdstr + cmdlen);
    *ps = htons(master);
    cmdlen += sizeof(unsigned short);
    break;
   case 18: /* SETIPADDR */
    cmdstr = realloc(cmdstr, cmdlen + strlen((char *)set) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
/* Ver. 1.01L sostituita strcpy(pc, (char *)set); con */
    memcpy(pc, (char *)set, strlen((char *)set));
    cmdlen += strlen((char *)set);
    break;
   case 19: /* SETIPNETMSK */
    cmdstr = realloc(cmdstr, cmdlen + strlen((char *)set) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
/* Ver. 1.01L sostituita strcpy(pc, (char *)set); con */
    memcpy(pc, (char *)set, strlen((char *)set));
    cmdlen += strlen((char *)set);
    break;
   case 20: /* SETIPGWY */
    cmdstr = realloc(cmdstr, cmdlen + strlen((char *)set) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
/* Ver. 1.01L sostituita strcpy(pc, (char *)set); con */
    memcpy(pc, (char *)set, strlen((char *)set));
    cmdlen += strlen((char *)set);
    break;
   case 21: /* SETRS232PARAM */
    cmdstr = realloc(cmdstr, cmdlen + strlen((char *)set) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
/* Ver. 1.01L sostituita strcpy(pc, (char *)set); con */
    memcpy(pc, (char *)set, strlen((char *)set));
    cmdlen += strlen((char *)set);
    break;
   case 22: /* SETCNETCRNUM */
    cmdstr = realloc(cmdstr, cmdlen + sizeof(unsigned short) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

    ps = (unsigned short *)(cmdstr + cmdlen);
    *ps = htons(*(unsigned short *)set);
    cmdlen += sizeof(short);
    break;
   case 23:  /* SETSYSNAME */
    cmdstr = realloc(cmdstr, cmdlen + strlen((char *)set) +
                                (st->cmdNow->numparIn -1) * strlen(LIB_SEP));
    pc = cmdstr + cmdlen;
/* Ver. 1.01L sostituita strcpy(pc, (char *)set); con */
    memcpy(pc, (char *)set, strlen((char *)set));
    cmdlen += strlen((char *)set);
    break;
  }  

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   return ret;
  }

FindSetAck(st, rislen, risposta);

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);
 
return st->errCode;
}

/* SY1527CAENETCOMM  ----------------------------------------------------------
    Function to get CAENET code to module CAEN via SY1527
    Parameter In
     SysId        Sistem ID
     Crate        crate number of system to communicate
	 Code         code 
	 NrWCode      nr. of additional word code
	 WCode        additional word code
    Parameter Out
	 Result       caenet error code 
	 NrOfData     nr. of data
     Data         response to caenet code

Release 2.00L
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527CaenetComm(int SysId, unsigned short Crate,
         unsigned short Code, unsigned short NrWCode, unsigned short *WCode,
		     short *Result, unsigned short *NrOfData, unsigned short **Data)
{
int ret, rislen, index_len, indStr, i, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *risposta = (char *)NULL;
unsigned short *ps, *app;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_EXECNET);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

cmdstr = realloc(cmdstr, cmdlen + sizeof(Crate) + sizeof(Code) + sizeof(NrWCode) +
        sizeof(short)*NrWCode + (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(Crate);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(Code);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

ps = (unsigned short *)(cmdstr + cmdlen);
*ps = htons(NrWCode);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

for( i = 0; i < NrWCode; i++ )
  {
   ps = (unsigned short *)(cmdstr + cmdlen);
   *ps = htons(WCode[i]);
   cmdlen += sizeof(unsigned short);
  }

if ( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
    {
    if (cmdstr) free(cmdstr);
    return ret;
    }

FindSetAck(st, rislen, risposta);
if( st->errCode )
   {
    if (cmdstr) free(cmdstr);
    if (risposta) free(risposta);
    return st->errCode;
   }

/* leggo il risultato */
index_len = st->headLen;
ps = (unsigned short *)(risposta + index_len);

/* conto il numero di parole inviate dal caenet */

*NrOfData = ntohs(*ps) - 1;

*Data = (unsigned short *)malloc((*NrOfData)*sizeof(unsigned short));
app = *Data;

index_len += sizeof(short)+strlen(LIB_SEP);
ps = (unsigned short *)(risposta + index_len);
*Result = ntohs(*ps);    /* error code del caenet ritornato a parte */

ps++;

for( i = 0; i < *NrOfData; i++, ps++ )
  app[i] = ntohs(*ps);
  /* oppure *Data[i] = ntohl(*pl); */

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);

return st->errCode;
}

/* SY1527SUBSCRIBE  ----------------------------------------------------------
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Parameter In
     SysId         Sistem ID
     UDPPort       UDP Port
	 NrOfItems     nr. of items to subscribe
	 ListOfItems   List of Items
    Parameter Out
	 ListOfCodes   List of codes which identify the items 

Release 3.00
 -----------------------------------------------------------------------------*/
SY1527RESULT Sy1527Subscribe(int SysId, short UDPPort, ushort NrOfItems, 
					   const char *ListOfItems, char *ListOfCodes)
{
int ret, i, rislen, index_len, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *ListItemsConv, *pc, *risposta = (char *)NULL;
unsigned short *pp;
unsigned long  len;
STATUS *st;
long *ps;
char *z;
char *p = NULL;
/*long tmp = 0;  va tolto perche' e' di debug */

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_SUBSCR);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

/* funzione che spacchetta ListOfItems e li mette nel formato x essere spedito 
   all'Sy1527. Ritorna la lunghezza della stringa */

ListItemsConv = NULL;
if( ConvListItems(NrOfItems, ListOfItems, &ListItemsConv, &len) )
   return CAENHV_PARAMNOTFOUND;

cmdstr = realloc(cmdstr, cmdlen + sizeof(UDPPort) + sizeof(NrOfItems) +
         len + (st->cmdNow->numparIn -1) * strlen(LIB_SEP));

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(UDPPort);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(NrOfItems);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pc = cmdstr + cmdlen;
memcpy(pc, ListItemsConv,len);
cmdlen += len;

if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   if (ListItemsConv) free(ListItemsConv);

   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   if (ListItemsConv) free(ListItemsConv);
   return st->errCode;
  }

index_len = st->headLen;
ps = (long *)(risposta + index_len +3);


p = malloc (strlen(ListOfItems)+1);
z = p;
strcpy(p,ListOfItems);

for (i=0;i<NrOfItems;i++) 
	{
	long tmp = ntohl(ps[i]);
	char item[64];
	
	
	if ( (i == 0) && (NrOfItems > 1) ) {
		p = strtok(p, LIB_SEP);
	}
	else {
			strcpy(item,p);  /* l'ultimo Item non e' terminato dalla LIBSEP ma da \0 */
	}
	if( i != NrOfItems - 1 )
		{
		strcpy(item,p);
		p = strtok(NULL, LIB_SEP);
		}
	switch(tmp) 
		{	
		case -1:
			ListOfCodes[i] = (char)-1;
			break;
		case -2:
			ListOfCodes[i] = (char)-2;
			break;
		case -3:
			ListOfCodes[i] = (char)-3;
			break;
		default:
			 AddIDsNow( indStr, tmp, item );
			 ListOfCodes[i] = 0;
			break;
		}

	}

/* libero la memoria allocata dalle funzioni precendenti */

if (cmdstr) free(cmdstr);
if (risposta) free(risposta);
if (ListItemsConv) free(ListItemsConv);
if (z != NULL) free(z);
return st->errCode;
/*return 0; */
}

/* SY1527UNSUBSCRIBE  ----------------------------------------------------------
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Parameter In
     SysId              Sistem ID
	 NrOfIDs            nr. of items to unsubscribe
    Parameter Out
	 ListOfErrorCodes   List of error which codes which identify the items 

Release 3.00
 -----------------------------------------------------------------------------*/
SY1527RESULT Sy1527UnSubscribe(int SysId, short UDPPort, short NrOfItems, const char *ListOfItems,
											char *ListOfCodes)
{
int ret, i, rislen, index_len, indStr, cmdlen = 0;
char *cmdstr = (char *)NULL;
char *ListIDConv, *pc, *risposta = (char *)NULL;
unsigned short *pp;
unsigned long  len;
STATUS *st;
long *ps;
char *z;
char *p = NULL;
long tmp = 0; /* va tolto perche' e' di debug */

/* trovo la stNow associata al sistema tramite il suo indice */
if( (indStr = FindSystemStruct(SysId, stNow)) < 0 )
   return SY1527_NOTPRES;

st = stNow[indStr];

/* inizializzo la var globale che identifica il comando attualmente in
   esecuzione */
st->cmdNow = FindCmdInfo(O_UNSUBSCR);

/* in caso di logout l'header == comando */
doHeader(st, &cmdstr, &cmdlen);

/* funzione che spacchetta ListOfItems e li mette nel formato x essere spedito 
   all'Sy1527. Ritorna la lunghezza della stringa */

p = malloc (strlen(ListOfItems)+1);
z = p;
strcpy(p,ListOfItems);

ListIDConv = NULL;

if( ConvListID(indStr, NrOfItems, p, &ListIDConv, &len) )
   return CAENHV_PARAMNOTFOUND;

if (z != NULL) free(z);

cmdstr = realloc(cmdstr, cmdlen + sizeof(NrOfItems) + sizeof(UDPPort) + 
         len + (st->cmdNow->numparIn-1) * strlen(LIB_SEP));

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(UDPPort);
cmdlen += sizeof(UDPPort);
cmdstr[cmdlen++]= LIB_SEP[0];

pp = (unsigned short *)(cmdstr + cmdlen);
*pp = htons(NrOfItems);
cmdlen += sizeof(short);
cmdstr[cmdlen++]= LIB_SEP[0];

pc = cmdstr + cmdlen;
memcpy(pc, ListIDConv,len);
cmdlen += len;
 
if( (ret = sendReceive(st, cmdstr, cmdlen, &rislen, &risposta)) != SY1527_OK )
  {
   if (cmdstr) free(cmdstr);
   if (ListIDConv) free(ListIDConv);
   return ret;
  }

FindSetAck(st, rislen, risposta);
if( st->errCode )
  {
   if (cmdstr) free(cmdstr);
   if (risposta) free(risposta);
   if (ListIDConv) free(ListIDConv);
   return st->errCode;
  }

index_len = st->headLen;
ps = (long *)(risposta + index_len +3);


p = malloc (strlen(ListOfItems)+1);
z = p;
strcpy(p,ListOfItems);

for (i=0;i<NrOfItems;i++) 
	{
	/*long tmp = ntohl(ps[i]); */
	char item[64];
	
	
	if ( (i == 0) && (NrOfItems > 1) ) {
		p = strtok(p, LIB_SEP);
	}
	else {
			strcpy(item,p);  /* l'ultimo Item non e' terminato dalla LIBSEP ma da \0 */
	}
	if( i != NrOfItems - 1 )
		{
		strcpy(item,p);
		p = strtok(NULL, LIB_SEP);
		}
	switch(tmp) 
		{	
		case 1:
			ListOfCodes[i] = (char)1;
			break;
		case 2:
			ListOfCodes[i] = (char)2;
			break;
		default:
/*			 printf("-->%s\n",item); */
			 RemIDsNow( indStr, item );
			 ListOfCodes[i] = 0;
			break;
		}

	}

/* libero la memoria allocata dalle funzioni precendenti */
if (cmdstr) free(cmdstr);
if (risposta) free(risposta);
if (ListIDConv) free(ListIDConv);
if (z != NULL) free(z);
return st->errCode;
}

/* Sy1527GetEventData  ----------------------------------------------------------
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Parameter In
     SysId              Sistem ID
	 NrOfIDs            nr. of items to unsubscribe
    Parameter Out
	 ListOfErrorCodes   List of error which codes which identify the items 

Release 3.00
 -----------------------------------------------------------------------------*/
int Sy1527GetEventData(SOCKET sck, char *buffer, struct sockaddr_in *addr, int *len)
{
	int result;

	result = recvfrom(sck, buffer, MAXLINE, 0, (struct sockaddr *)addr, (socklen_t*)len);
	if (result < UDPMINPACKETLENGTH) {	
		return -1;
	}
	return result;
}

/******************************* UTILITIES ***********************************/

/* SY1527GETERROR *************************************************************
Return the error string

Parameter In
 SysId
Globals
 stNow
 *****************************************************************************/
 char *Sy1527GetError(int SysId)
{
int i;
STATUS *st;

/* trovo la stNow associata al sistema tramite il suo indice */
if( (i = FindSystemStruct(SysId, stNow)) < 0 )
    return LIB_SYS_NOT_PRES;
st = stNow[i];

return st->errString;
}

/* SY1527GETLogInOutErr *******************************************************
Function which returns the error with the last Login/Logout call     

 *****************************************************************************/
 char *Sy1527LogInOutErr(void) { return LogInOut; }

