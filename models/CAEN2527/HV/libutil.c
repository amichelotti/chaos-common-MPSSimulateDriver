/*****************************************************************************/
/*                                                                           */
/*       --- CAEN Engineering Srl - Computing Division ---                   */
/*                                                                           */
/*   SY1527 Software Project                                                 */
/*                                                                           */
/*   LIBUTIL.C                                                               */
/*                                                                           */
/*   Created: January 2000                                                   */
/*                                                                           */
/*   Auth: E. Zanetti, A. Morachioli                                         */
/*                                                                           */
/*   Release: 1.0L                                                            */
/*                                                                           */
/*****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#ifdef UNIX
#include <netinet/in.h>
#endif
#include "libclient.h"
#include "sy1527interface.h"

/******************************************************* // !!! CAEN
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
// ******************************************************* // !!! CAEN */


#ifdef UNIX
extern int errno;
#endif

#ifndef ushort
#define ushort unsigned short
#endif

/* ----- Ver. 3.00 ----- */ 
typedef struct{        
	char *p;
	unsigned short size;
	unsigned short current;
} MyString;

static void AddShort( MyString *s, short val )
{
	short l = htons(val);
	s->p = realloc(s->p, s->size + sizeof(short)+1);
	s->size += sizeof(short)+1;
	memcpy(&(s->p[s->current]),&l,sizeof(short));
	s->current += sizeof(short);
	s->p[s->current] = LIBSEP;
	s->current++;
}

static void AddLong( MyString *s, long val )
{
	long l = htonl(val);
	s->p = realloc(s->p, s->size + sizeof(long)+1);
	s->size += sizeof(long)+1;
	memcpy(&(s->p[s->current]),&l,sizeof(long));
	s->current += sizeof(long);
	s->p[s->current] = LIBSEP;
	s->current++;
}

static void AddString( MyString *s, char *str, int len )
{
	s->p = realloc(s->p, s->size + len + 1);
	s->size += len + 1;
	memcpy((&s->p[s->current]), str, len);
	s->current += len;
	s->p[s->current] = LIBSEP;
	s->current++;
}

static void InitMyString(MyString *s)
{
	s->p = NULL;
	s->size = 0;
	s->current = 0;
}

static void FreeMyString(MyString *s)
{
	free(s->p);
	s->size = 0;
	s->current = 0;
}

IDINFO *FindID(int IndSys, long ID)
{
	IDINFO q = IDsNow[IndSys];
	IDINFO *p;
	char   trovato = 0;
	p = q.Next;
	while( p != NULL && !trovato )
		{
		 if( p->ID == ID )
			 trovato = 1;
		 else
			 p = p->Next;
		}
	if( trovato )
		return p;
	else
		return NULL;
}

int FindParamType( char *Item, unsigned short *Board, unsigned short *Ch, char *ParName )
{
	char *p;
	int  ret = -1;

    p = strchr(Item,'.');
	if (p == NULL) {
		if( !strncmp(Item,"KeepAlive",9) || !strncmp(Item,"LiveInsRem",10) )
			 ret = TYPE_ALARM;
			else
			 ret = TYPE_SYSTEM_PROP;
	}
	else {
		if( !strncmp(p+1,"Board",5) )
			{
			 p = p + 1 + 5;
			 *Board = (*p-48)*10 + (*(p+1)-48);
			 p = p + 2;
			 if( !strncmp(p+1,"Chan",4) )
				{
				 p = p + 1 + 4;
				 *Ch = (*p-48)*100 + (*(p+1)-48)*10 + (*(p+2)-48);
				 p += 3;
				 ret = TYPE_CHANNEL_PAR;
				}
			 else
				 ret = TYPE_BOARD_PAR;
			}
		else
		   if( !strncmp(p+1,"KeepAlive",9) || !strncmp(p+1,"LiveInsRem",10) )
			 ret = TYPE_ALARM;
			else
			 ret = TYPE_SYSTEM_PROP;

		strcpy(ParName,p+1);
		return ret;
	}
	strcpy(ParName,Item);
	return ret;
}

void RemIDsNow(int IndSys, char *Item) {

IDINFO s = IDsNow[IndSys];
IDINFO *p;
	p = s.Next;
	while (p != NULL) {
		if (!strcmp(p->Item,Item) ) {
			IDINFO *q = p->Prev;
			q->Next = p->Next;
			if (p->Next != NULL) p->Next->Prev = q;
			free(p);
			break;
		}
		else p = p->Next;
	}
}

/*REl 3.02 */
void RemoveIDsNowInfo(IndStr) { /*REl 3.02 */

	IDINFO *p;
	p = IDsNow[IndStr].Next;
	IDsNow[IndStr].Next = p->Next;
	if (p->Next != NULL) p->Next->Prev = p->Prev;
	free(p);
}

/*REl 3.02 */
void RemoveIDsNow(int IndSys) { /*REl 3.02 */
int i;

while (IDsNow[IndSys].Next != NULL) RemoveIDsNowInfo(IndSys);

for( i = IndSys; i+1 < MaxSysLogNow; i++ )
    memcpy(&IDsNow[i],&IDsNow[i+1],sizeof(IDINFO));

if( MaxSysLogNow == 1)
  IDsNow = NULL;
else
 IDsNow = realloc(IDsNow, (MaxSysLogNow-1) * sizeof(IDINFO));

return;
}


void AddIDsNow( int IndSys, long ID, char *Item )
{
	IDINFO *p = malloc(sizeof(IDINFO));
	STATUS *st;
	PARAM_TYPE *pParType;
	/* bug del SymbolicName che ha lunghezza maggiore di MAX_PARAM_NAME per cui ho definito nel .h MAX_SYMBOLIC_PARAM_NAME // ver 3.01 */
	char ParName[MAX_SYMBOLIC_PARAM_NAME];
	unsigned short bd, ch;
	int type, indSB, indPar, i,trovato;

	p->Prev = (IDINFO *) &IDsNow[IndSys];
	p->Next = IDsNow[IndSys].Next;
	if (IDsNow[IndSys].Next != NULL ) IDsNow[IndSys].Next->Prev = p;
	IDsNow[IndSys].Next = p;

	p->ID = ID;
	strcpy(p->Item, Item);

	st = stNow[IndSys];

	type = FindParamType( Item, &bd, &ch, ParName );
	p->Type = type;
	pParType = st->system->ParTypes;
    switch( type )
		{
		 case TYPE_CHANNEL_PAR:
			 /* cerco la scheda nella mia struttura StNow */
			 for( i = 0; i < st->system->NrOfBd; i++ )
				if( st->system->Board[i].Position == bd )
					{
					 indSB = i;
					 break;
					}
			 trovato =0;
			 for( i = 0; i < st->system->Board[indSB].IndParCh[ch].NrOfChPar; i++ )
				{
				 indPar = st->system->Board[indSB].IndParCh[ch].IndChPar[i];
/*Ver. 3.01 messo come sotto if( !strcmp(pParType[indPar].Name, ParName ) ) { */
				 if( !strncmp(pParType[indPar].Name, ParName,MAX_PARAM_NAME ) ) { 
					 trovato = 1;
					break;
				 }
			    }
			 if (trovato ) 
				memcpy(&(p->ParItem),&pParType[indPar],sizeof(PARAM_TYPE));
			 else p->ParItem.Name[0] = 0;
			 break;

		 case TYPE_BOARD_PAR:
			 /* cerco la scheda nella mia struttura StNow */
			 for( i = 0; i < st->system->NrOfBd; i++ )
				if( st->system->Board[i].Position == bd )
					{
					 indSB = i;
					 break;
					}
			 trovato =0;
			 for( i = 0; i < st->system->Board[indSB].NrOfBdPar; i++ )
			     {
			      indPar = st->system->Board[indSB].IndBdPar[i];
/* Ver. 3.01 messo come sotto  if( !strcmp(pParType[indPar].Name, ParName ) ) { */
				  if( !strncmp(pParType[indPar].Name, ParName,MAX_PARAM_NAME ) ) {
					trovato = 1;
				    break;
				  }
			     }

			 if (trovato ) 
				memcpy(&(p->ParItem),&pParType[indPar],sizeof(PARAM_TYPE));
			 else p->ParItem.Name[0] = 0;
			 break;

		 default:
			 p->ParItem.Name[0] = 0;
			 break;
		}
}

int FindBdChValPar(char *buffer, PARAM_TYPE *p, CAENHVEVENT_TYPE_ALIAS *EventData)
{
	unsigned long ParValue;
	unsigned long len;

	len = ntohl(*(long *)buffer);
	buffer += 4;
	if( p->Name != "" ) {
		ParValue = ntohl(*(long *)buffer);
		switch( p->Flag.Type )
			{
			case PARAM_TYPE_NUMERIC:
			 if( p->Flag.Sign == SIGNED ) 
			 {
			   int val = (int)ParValue;

  			   if( p->Un.Car.Dec > 0 )
					*(float *) EventData->Lvalue = (float)((float)val /(float)p->Un.Car.Res);
				else
				    *(float *) EventData->Lvalue = (float)((float)val * (float)p->Un.Car.Res);
			 }
			 else
			 {
  			   if( p->Un.Car.Dec > 0 )
				   *(float *) EventData->Lvalue = (float)((float)ParValue / (float)p->Un.Car.Res);
				else
				   *(float *) EventData->Lvalue = (float)((float)ParValue * (float)p->Un.Car.Res);
			 }
			   break;
			  case PARAM_TYPE_ONOFF:
			   *(long *) EventData->Lvalue = ParValue;
			   break;
			  case PARAM_TYPE_CHSTATUS:
			   *(long *) EventData->Lvalue = ParValue;
			   break;
			  case PARAM_TYPE_BINARY:		/* Rel 2.16 */
			   *(long *) EventData->Lvalue = ParValue;	  
				  break;
		 }
		EventData->Type = EVENTTYPE_PARAMETER;
		return 0;
	}
	else {
		EventData->Type = EVENTTYPE_PARAMETER;
		return -1;
	}
}

void FindSysPar(int SysId, char *buffer, PARAM_TYPE *p, CAENHVEVENT_TYPE_ALIAS *EventData)
{
	unsigned long len;
	GETSETEXE *gse;
	char *pc, buff[80];
	int index_len, i , y;
	void *result;
	ushort *ps;
	long  *pl;
	unsigned int z;

	len = ntohl(*(long *)buffer);
	buffer += 4;
	gse = PropExecSearch(SysId, (char *)(strrchr(EventData->ItemID,'.')+1));
	index_len = 0;
	pc = buffer;
	switch(gse->CommId)
	  {
	   case 0:   /* GETACTLOG	 */
		result = (void *)EventData->Tvalue;
		strncpy(result,buffer,256);
	   break;
	   case 1:
			
		   break;
	   case 2:

		   break;
	   case 3:

		   break;
	   case 4:

		   break;
	   case 5:

		   break;
	   case 6:

		   break;
	   case 7:   /* GETSYSNAME: solo nome del modello di sistema */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy(buff,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		strtok(buff,",");
		memcpy((char *)result, buff, strlen(buff) + 1);
		break;
	   case 8:   /* GETSWREL */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy((char *)result,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		*((char *)result + strlen(buffer)) = '\0';
		break;
	   case 9:   /* GETGENCFG */
		result = (void *)EventData->Lvalue;
		pc = buffer + index_len;
		*(unsigned short *)result = *pc;    
		break;
	   case 10:  /* GETFPIN */
	   case 11:  /* GETFPOUT */
		result = (void *)EventData->Lvalue;
		pl = (long *)(buffer + index_len);
		*(unsigned long *)result = ntohl(*pl);
		break;
	   case 12:  /* GETRSFLAGCFG */
		result = (void *)EventData->Lvalue;
		pc = buffer + index_len;
		*(unsigned short *)result = *pc;    
		break;
	   case 13:  /* GETRSFLAG */
		break;
	   case 14:  /* GETHVPSM */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		for(y = 0, i = 0; i < 5; i++, pc++ )
		  {/* converte il dato in stringa per essere visualizato */
		   sprintf(((char *)result + y), y ? ":%d" : "%d", *pc);
		   y += sprintf(buff, y ? ":%d" : "%d", *pc); 
		  }
		break;
	   case 15:  /* GETFANSTAT */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		for( y = 0, z = index_len; z < len; pc += 3 )
		  {/* converte il dato in stringa per essere visualizato */
		   sprintf(((char *)result + y), y ? ":%d" : "%d", *pc);
		   y += sprintf(buff,y ? ":%d" : "%d", *pc); 
		   z++;
		   ps = (unsigned short *)(pc + 1);
		   sprintf(((char *)result + y),":%d", ntohs(*ps));
		   y += sprintf(buff,":%d", ntohs(*ps));
		   z += sizeof(unsigned short);
		  }
		break;
	   case 16:  /* GETCLKFREQ */
		result = (void *)EventData->Lvalue;
		pc = buffer + index_len;
		*( short *)result = *pc;    
		break;
	   case 17:  /* GETHVCLKCFG */
		result = (void *)EventData->Tvalue;
		y = 0;
		pc = buffer + index_len;
		sprintf((char *)result,"%d:", *pc);
		y += sprintf(buff,"%d:", *pc); 
		pc++;
		sprintf(((char *)result + y), "%d", *pc);
		break;
	   case 18:  /* GETIPADDR */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy((char *)result,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		*((char *)result + strlen(buffer)) = '\0';
		break;
	   case 19:  /* GETIPNETMSK */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy((char *)result,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		*((char *)result + strlen(buffer)) = '\0';
		break;
	   case 20:  /* GETIPGWY */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy((char *)result,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		*((char *)result + strlen(buffer)) = '\0';
		break;
	   case 21:  /* GETRS232PARAM */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy((char *)result,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		*((char *)result + strlen(buffer)) = '\0';
		break;
	   case 22:  /* GETCNETCRNUM */
		result = (void *)EventData->Lvalue;
		pl = (long *)(buffer + index_len);
		*(unsigned long *)result = ntohl(*pl);
		break;
	   case 23:  /* SymbolicName: nome simbolico del Sistema */
		result = (void *)EventData->Tvalue;
		pc = buffer + index_len;
		memcpy(buff,pc,strlen(buffer)); /* x' non e' null terminata !!! */
		*(buff + strlen(buffer)) = '\0';
		strtok(buff,",");
		memcpy((char *)result, buff+strlen(buff)+1, strlen(buffer)-strlen(buff));
		break;
	  }  
	EventData->Type = EVENTTYPE_PARAMETER;
}


void FindAlarmPar(char *buffer, PARAM_TYPE *p, CAENHVEVENT_TYPE_ALIAS *EventData)
{
	unsigned long ParValue;
	unsigned long len;

	len = ntohl(*(long *)buffer);
	buffer += 4;
	ParValue = ntohl(*(long *)buffer);
	*(long *) EventData->Lvalue = ParValue;
	if (strstr(EventData->ItemID,"KeepAlive") != NULL) EventData->Type = EVENTTYPE_KEEPALIVE; else EventData->Type = EVENTTYPE_ALARM;
}


/* ConvListItems **************************************************************
Save the error string and return the error code

Parameter In
 st
 errore
 frase
Globals
 stNow

Ver. 3.00
 *****************************************************************************/
int ConvListItems(unsigned short NrOfItems, const char *ListOfItem, char **ListItemsConv, 
				   unsigned long *len)
{
	/* bug del SymbolicName che ha lunghezza maggiore di MAX_PARAM_NAME per cui ho definito nel .h MAX_SYMBOLIC_PARAM_NAME // ver 3.01 */
char *p=NULL,*q=NULL, *listCopy, parName[MAX_SYMBOLIC_PARAM_NAME];
unsigned long  indexLen, lenItIn;
unsigned short i, board, chan;
MyString s;
IDINFO **localIDsNow;

InitMyString(&s);

/* Verifico la stringa */
p = malloc(strlen(ListOfItem)+1);
q = p;
memset(p,0,strlen(ListOfItem)+1);
memcpy(p,ListOfItem,strlen(ListOfItem));

i = 0;
while( p != NULL )
	{
	 p = strchr(p+1, ':');
	 i++;
	}
if( i != NrOfItems )
  return -1;

localIDsNow = malloc((NrOfItems) * sizeof(IDINFO *));
for( i = 0; i < NrOfItems; i++ )
	 localIDsNow[i] = malloc(sizeof(IDINFO));

lenItIn = strlen(ListOfItem);
listCopy = malloc(lenItIn + 1);
strcpy(listCopy,ListOfItem);

indexLen = 0;
p = listCopy;

if (NrOfItems > 1) p = strtok(p, LIB_SEP);

for( i = 0; i < NrOfItems - 1; i++ )  /* copio gli Item nella struttura */
  {
   strcpy(localIDsNow[i]->Item,p);
   indexLen += strlen(p);
   p = strtok(NULL, LIB_SEP);
  }

strcpy(localIDsNow[i]->Item,p);   /* l'ultimo Item non e' terminato dalla LIBSEP */

/* preparo la lista per il sistema */
for( i = 0; i < NrOfItems; i++ )
  {
   int ret = FindParamType( localIDsNow[i]->Item, &board, &chan, parName );
    
   switch( ret )
	{
	 case TYPE_CHANNEL_PAR:
		 AddShort(&s,TYPE_CHANNEL_PAR);
		 AddShort(&s,board);
		 AddShort(&s,chan);
		 AddString(&s,parName,strlen(parName));
		 break;
	 case TYPE_BOARD_PAR:
		 AddShort(&s,TYPE_BOARD_PAR);
		 AddShort(&s,board);
		 AddString(&s,parName,strlen(parName));
		 break;
	 case TYPE_ALARM:
		 AddShort(&s,TYPE_ALARM);
		 AddString(&s,parName,strlen(parName));
		 break;
	 case TYPE_SYSTEM_PROP:
		 AddShort(&s,TYPE_SYSTEM_PROP);
		 AddString(&s,parName,strlen(parName));
		 break;
	}
  }

*ListItemsConv = malloc(s.size);
memcpy(*ListItemsConv,s.p,s.size-1);
(*ListItemsConv)[s.size-1] = 0;
*len = s.size;
FreeMyString(&s);
for( i = 0; i < NrOfItems; i++ )
	 free(localIDsNow[i]);
free(localIDsNow);
if( q !=NULL ) free(q);
return 0;
}

/* ConvListItems **************************************************************
Save the error string and return the error code

Parameter In
 st
 errore
 frase
Globals
 stNow

Ver. 3.00
 *****************************************************************************/
int ConvListID(int idx, unsigned short NrOfItems, char *ListOfItem, char **ListIDConv, 
				   unsigned long *len)
{
char *p, *listCopy;
unsigned long  indexLen, lenItIn;
unsigned short i;
MyString s;
unsigned long LongID;
IDINFO **localIDsNow;

InitMyString(&s);

/* Verifico la stringa */
p = ListOfItem;
i = 0;
while( p != NULL )
	{
	 p = strchr(p+1, ':');
	 i++;
	}
if( i != NrOfItems )
  return -1;

localIDsNow = malloc((NrOfItems) * sizeof(IDINFO *));
for( i = 0; i < NrOfItems; i++ )
	 localIDsNow[i] = malloc(sizeof(IDINFO));

lenItIn = strlen(ListOfItem);
listCopy = malloc(lenItIn + 1);
strcpy(listCopy,ListOfItem);

indexLen = 0;
p = listCopy;

if (NrOfItems > 1) p = strtok(p, LIB_SEP);

for( i = 0; i < NrOfItems - 1; i++ )  /* copio gli Item nella struttura */
  {
   strcpy(localIDsNow[i]->Item,p);
   indexLen += strlen(p);
   p = strtok(NULL, LIB_SEP);
  }

strcpy(localIDsNow[i]->Item,p);   /* l'ultimo Item non e' terminato dalla LIBSEP */

/* preparo la lista per il sistema */
for( i = 0; i < NrOfItems; i++ )
  {
	IDINFO *o;
	o = IDsNow[idx].Next;
	LongID = -1;
	while (o != NULL) {
		if (!strcmp(o->Item,localIDsNow[i]->Item)) {
			LongID = o->ID;
			break;
		}
		else o = o->Next;
	}
	if (LongID == -1) LongID = -2;
	AddLong(&s,LongID);
  }

*ListIDConv = malloc(s.size);
memcpy(*ListIDConv,s.p,s.size-1);
(*ListIDConv)[s.size-1] = 0;
*len = s.size;
FreeMyString(&s);
for( i = 0; i < NrOfItems; i++ )
	 free(localIDsNow[i]);
free(localIDsNow);
return 0;
}

/* PROPEXECSEARCH *************************************************************

Ricerca le get/set property o gli execute command del SY1527.
Ritorna un puntatore alla lista GSEList.

 Parametri In 
  indSys    indice di sistema
  name      nome comando
 Globali
  GSEList
      
Rel. 2.10 - Aggiunto il qualificatore const.

 *****************************************************************************/
GETSETEXE *PropExecSearch(int indSys, const char *name)
{
int i = 0;

while( GSEList[i].GetSetExec != -1 )
  {
   if( !strcmp(GSEList[i].Item, name) )
      return &(GSEList[i]);
   i++;
  }

return (GETSETEXE *)NULL;
}

/* FINDCMDINFO ****************************************************************
                                                                           
A partire dal nome del comando, trova la riga corrispondente al comando 

 Parametri In 
 cmd          nome del comando
 Globali
  CommandList
                                                                           
 *****************************************************************************/
COMANDO *FindCmdInfo(char *cmd)
{
int i = 0;

while(strcmp(CommandList[i].nome, LIB_NULLCMD))
  {
   if( !strcmp(CommandList[i].nome, cmd) )
      return &(CommandList[i]);
   i++;
  }

return (COMANDO *)NULL;
}

/* FINDSYSTEMSTRUCT ***********************************************************
                                                                        
Ritorna l'indice i per puntare alla struttura con indice indSys a cui   
viene associato il sistema tramite la Login                             
                                                                           
 Parametri In 
  indSys       indice di sistema
  st           puntatore alla struttura STATUS
 Globali
  MaxSysLogNow numero di sistemi colleganti al momento
  stNow

 *****************************************************************************/
int FindSystemStruct(int indSys, STATUS **st)
{
int i;

for( i = 0; i < MaxSysLogNow; i++, st++ )
    if( (*st)->commType.systemInd == indSys )
       return i;

return -1;
}

/* FREESYSTEMSTRUCT ***********************************************************
                                                                                                        
Libera la memoria dalla struttura SYSINFO di indice IndStr
                                                                           
 Parametri In 
  indSys       indice di sistema
 Globali
  stNow

 *****************************************************************************/
void freeSystemStruct(int IndStr)
{
int i, j;
STATUS *st = stNow[IndStr];

if( st->system == (SYSINFO *)NULL )
  return;

/* 
libero le strutture PARAM_TYPE: disalloco tutto quello alloca con la 
malloc
*/
if( st->system->ParTypes != (PARAM_TYPE *)NULL )
  free(st->system->ParTypes);

st->system->ParTypes = (PARAM_TYPE *)NULL;

/* libero l'array di strutture SYSBOARD */
if( st->system->Board )
  {
   for( i = 0; i < st->system->NrOfBd; i++ )
     {
      /* libero l'array IndBdPar di ciascuna Board */
      if( st->system->Board[i].IndBdPar )
        free(st->system->Board[i].IndBdPar);
      st->system->Board[i].IndBdPar = (ushort *)NULL;
      /* libero le strutture INDPARAM di ciascun canale */
      for( j = 0; j < st->system->Board[i].NrOfCh; j++ )
        {
         if( st->system->Board[i].IndParCh[j].IndChPar )
           free(st->system->Board[i].IndParCh[j].IndChPar);
         st->system->Board[i].IndParCh[j].IndChPar = (ushort *)NULL;
        }
      if( st->system->Board[i].IndParCh )
         free(st->system->Board[i].IndParCh);
      st->system->Board[i].IndParCh = (INDPARAM *)NULL;
     }
   free(st->system->Board);
  }

st->system->Board = (SYSBOARD *)NULL;

free(st->system);
st->system = (SYSINFO *)NULL;

return;
}

/*FREESTNOWSTRUCT  ************************************************************
                                                             
 Riordina l'array di strutture STATUS                                    
                                                                           
 Parametri In 
  indSys       indice di sistema
 Globali
  MaxSysLogNow numero di sistemi colleganti al momento
  stNow

 *****************************************************************************/
void freeStNowStruct(int IndStr)
{
int i;

if( stNow[IndStr]->system )
  freeSystemStruct(IndStr);

/* Ver. 1.3 - Aggiunta la free */
free(stNow[IndStr]);

for( i = IndStr; i+1 < MaxSysLogNow; i++ )
    stNow[i] = stNow[i+1];

stNow = realloc(stNow, (MaxSysLogNow-1) * sizeof(STATUS *));

if( MaxSysLogNow == 1 )
  stNow = (STATUS **)NULL;

return;
}

/* FINDSETACK *****************************************************************

Dalla risposta ricavo l'ackCode e l'ackString e setto i campi della    
struttura stNow corrispondenti                                          
                                                                           
 Parametri In 
  st        puntatore alla struttura stNow
  rislen    lunghezza della stringa ris
  ris       stringa di risposta da una comunicazione con il sistema
 Globali
  stNow

 *****************************************************************************/
void FindSetAck(STATUS *st, int rislen, char *ris)
{
char *pp, *ack, *stt = (char *)NULL;
short *pnts;
int headLen;

/* leggo l'ackCode */
headLen = strlen(st->cmdNow->nome) + strlen(LIB_SEP);

/* caso particolare del comando Login che non ha ancora la sessione */
if( strcmp(st->cmdNow->nome, L_LOGIN) )
   headLen += sizeof(st->commType.sessionID) + strlen(LIB_SEP);

pp = ris + headLen;
pnts = (short *)pp;
st->errCode = ntohs(*pnts);
headLen += sizeof(st->errCode) + strlen(LIB_SEP);

if( !st->errCode && st->cmdNow->numparOut > 0 )
   {
    stt = malloc(rislen);
/* Rel 1.3 qui la stringa non termina con 0
    strcpy(stt, ris + headLen);
*/
    strncpy(stt, ris + headLen, rislen - headLen);
    ack = (char *)strtok(stt, LIB_SEP);
    strcpy(st->errString, ack);
    headLen += strlen(st->errString) + strlen(LIB_SEP);
   }
else
   {
    strncpy(st->errString, ris + headLen, rislen - headLen);
    st->errString[rislen - headLen]  = '\0';
    headLen += strlen(st->errString);
   }

st->headLen = headLen;
if (stt) free(stt);
return;

#ifdef pippo
/* troppo casino!!!!!! */

  /* leggo l'ackCode */
  pp = ris + strlen(st->cmdNow->nome) + strlen(LIB_SEP);

  /* caso particolare del comando Login che non ha ancora la sessione */
  if (strcmp(st->cmdNow->nome, L_LOGIN))
      pp += sizeof(st->sessionID) + strlen(LIB_SEP);   /* sessione */
  pnts = (short *)pp;
  st->errCode = ntohs(*pnts);

  /* leggo l'ackString: per trovarla devo prima sapere quanto e' lunga, quindi
     calcolo la lunghezza della parte fissa dell'header dela risposta
     (il calcolo della lunghezza va bene per tutti i comandi, compreso LOGIN) */
  if (!st->errCode) /* no error */
     numsep = (st->cmdNow->numparHeadOut + st->cmdNow->numparOut - 1);
  else
     numsep = (st->cmdNow->numparHeadOut - 1);
  length = strlen(st->cmdNow->nome) + strlen(LIB_SEP)*numsep +
           sizeof(st->errCode) + sizeof(st->sessionID);

  if (!strcmp(st->cmdNow->nome, L_LOGIN))   /* caso LOGIN */
    {
    if (!st->errCode)   /* no error */
        {
        tl = length - sizeof(st->sessionID) - strlen(LIB_SEP);
        strncpy(st->errString, ris + tl, rislen - length);
        st->errString[rislen - length]  = '\0';
        }
    else
        {
        tl = length - sizeof(st->sessionID);
        strncpy(st->errString, ris + tl, rislen - tl);
        st->errString[rislen - tl]  = '\0';
        }
    }
  else /* altro comando */
    {
    /* la strtok puo' essere usata solo nel caso che non ho errori E ho dei parametri
        di ritorno dopo l'ackstring */
    if (!st->errCode && st->cmdNow->numparOut > 0)
        {
        stt = malloc(rislen);
        length = length - strlen(LIB_SEP);
        strcpy(stt, ris + length);  /* nel calcolo prima ho aggiunto un sep.
                                                           che ora non devo considerare */
        ack = (char *)strtok(stt, LIB_SEP);
        strcpy(st->errString, ack);
        }
    else
        {
        strncpy(st->errString, ris + length, rislen - length);
        st->errString[rislen - length]  = '\0';
        }
    tl = length;
    }

   /* setto la lunghezza dell'header del comando corrente */
   st->headLen = tl + strlen(st->errString);

   /* se e' il cmd e' OK e se ci sono parametri di out devo conteggiare anche il separatore
      che compare in fondo all'header */
   if (st->errCode == SY1527_OK && st->cmdNow->numparOut)
     st->headLen += strlen(LIB_SEP);

   if (stt) free(stt);
   return;

#endif
}

/* DOHEADER *******************************************************************
Costruisce l'header per i comandi in input                              

 Parametri In 
  st        puntatore alla struttura stNow
 Parametri Out
  str       stringa di risposta
  len       lunghezza della stringa str 
 Globali
  stNow

 *****************************************************************************/
void doHeader(STATUS *st, char **str, int *len)
{
unsigned long *p;

*len = strlen(st->cmdNow->nome);
if (st->cmdNow->numparHeadIn > 1) /* almeno il nome del comando c'e' sempre */
  *len += sizeof(st->commType.sessionID)
                       + strlen(LIB_SEP) * (st->cmdNow->numparHeadIn -1);

if (st->cmdNow->numparIn > 0) /* ci sono altri parametri, quindi devo appendere un sep */
  *len += strlen(LIB_SEP);

*str = malloc(*len + 1);
strcpy(*str, st->cmdNow->nome);
if (st->cmdNow->numparHeadIn > 1)
  {
  sprintf(*str,"%s%s", *str, LIB_SEP);
  p = (unsigned long *)(*str + strlen(*str));
  *p = htonl(st->commType.sessionID);
  }
if (st->cmdNow->numparIn > 0)
  (*str)[*len-1] = LIB_SEP[0];

return;
}

/* SETSYSTEMERROR**************************************************************

Per la compatibilita' fra i vari OS in caso di errore di sistema.
La versione per Win32 per ora prevede solo errori di rete e quindi la funzione 
da chiamare non e' la GetLastError.
La funzione ritorna la stringa di errore

 *****************************************************************************/
char *setSystemError(void) {

#ifdef UNIX
    return strerror(errno);
#else
        {
         static char buf[80];

     sprintf(buf, "Operating System Error: %d", WSAGetLastError());
     return buf;
    }
#endif
}

/* SALVAERRORE ****************************************************************
Save the error string and return the error code

Parameter In
 st
 errore
 frase
Globals
 stNow
 *****************************************************************************/
int salvaErrore(STATUS *st, int errore, char *frase)
   {
    strcpy(st->errString, frase);
    st->errCode = errore;
    return st->errCode;
   }

