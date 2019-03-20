/*****************************************************************************/
/*                                                                           */
/*       --- CAEN Engineering Srl - Computing Division ---                   */
/*                                                                           */
/*   SY1527 Software Project                                                 */
/*                                                                           */
/*   TRANSPORT.C                                                             */
/*                                                                           */
/*   Created: Janauary 2000                                                  */
/*                                                                           */
/*   Version: 1.0L                                                           */
/*                                                                           */
/*   Auth: E. Zanetti, A. Morachioli                                         */
/*                                                                           */
/*****************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef UNIX
#include <netinet/in.h>
#include <sys/types.h>               /* Rel. 2.0 - Linux */
#include <sys/socket.h>              /* Rel. 2.0 - Linux */
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#define SOCKET_ERROR SO_ERROR
#endif
#include "libclient.h"
#include "sy1527interface.h"
#ifdef WIN32 
#include<io.h>
#endif

/******************************************************* // !!! CAEN
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
// ******************************************************* // !!! CAEN */

/*
	Rel. 2.10 - Non dovrebbe generare troppi problemi con i thread
	            quindi la lasciamo cosi' com'e'.
*/
static int    tipocanale = LIB_TCPCOD;

/* CONNECT_NONB *************************************************************
Opera una connect non bloccante per poter introdurre il timeout anche sulla 
Login

Parametri In
 st
 saptr  puntatore alla struttura sockaddr
 salen  lunghezza della struttura
 sec    timeout sulla Login

 Rel. 2.14 - Provo a lasciare il socket in non blocking. Solo WIn32 */
/*****************************************************************************/
int connect_nonb(STATUS *st, const struct sockaddr *saptr, int salen, int sec)
{
int n, retval;
#ifdef WIN32
unsigned long block;
#endif
fd_set writemask;
struct timeval timeout;
#ifdef UNIX
int flags, error;
fd_set readmask;
socklen_t len;
#endif

#ifdef WIN32
block = 1;
ioctlsocket(st->commType.canaleFD, FIONBIO, &block); /* non blocking */
#endif
#ifdef UNIX
/*ioctl(st->commType.canaleFD, FIONBIO, &block);  non blocking */ 
flags = fcntl(st->commType.canaleFD, F_GETFL, 0);
fcntl(st->commType.canaleFD, F_SETFL, flags | O_NONBLOCK);
error = 0;
#endif

if( (n = connect(st->commType.canaleFD, saptr, salen)) < 0 )
#ifdef WIN32
  if( n != SOCKET_ERROR )
#endif
#ifdef UNIX
  if( errno != EINPROGRESS )
#endif
    return SY1527_LOG_ERR;

FD_ZERO(&writemask);
FD_SET(st->commType.canaleFD, &writemask);
#ifdef UNIX
readmask = writemask;
#endif
timeout.tv_sec = sec;
timeout.tv_usec = 0;
#ifdef WIN32
retval = select(st->commType.canaleFD + 1, NULL, &writemask, NULL, &timeout);
if(!retval)
  { /* timeout */
   close(st->commType.canaleFD);
   return salvaErrore(st, SY1527_TIMEERR, LIB_TIMEERR_STR);
  }
#endif

#ifdef UNIX
retval = select(st->commType.canaleFD + 1, &readmask, &writemask, NULL, &timeout);
if(!retval)
  { /* timeout */
   close(st->commType.canaleFD);
   errno = ETIMEDOUT;
   return SY1527_LOG_ERR;
  }
  
if( FD_ISSET(st->commType.canaleFD, &readmask) || FD_ISSET(st->commType.canaleFD, &writemask) )
  {
   len = sizeof(error);
   if( getsockopt(st->commType.canaleFD, SOL_SOCKET, SO_ERROR, &error, &len) < 0 )
     return SY1527_LOG_ERR;
  }
else
  return salvaErrore(st, SY1527_TIMEERR, LIB_TIMEERR_STR);
#endif

#ifdef WIN32
/* Rel. 2.14 Provo a lasciare il socket in non blocking */
block = 0;
ioctlsocket(st->commType.canaleFD, FIONBIO, &block); /* rimetto blocking */
*/
#endif
#ifdef UNIX
/*ioctl(st->commType.canaleFD, FIONBIO, &block); non blocking */ 
fcntl(st->commType.canaleFD, F_SETFL, flags); /* restore file status register */
if( error )
  {
   close(st->commType.canaleFD);
   errno = error;
   return SY1527_LOG_ERR;
  }
#endif

return SY1527_OK;
}

/* APRICANALE *****************************************************************
Apre la comunicazione con il canale scelto. Ritorna il codice di errore

Parametri In
 st
 canale     canale scelto: TCP/IP o RS232
 address    indirizzo IP o COMM

 *****************************************************************************/
int apriCanale(STATUS *st, const char *canale, const char *address)
{
int                on = 1;
#ifdef WIN32
int                a; 			/* Rel. 2.4 - Used only in WIN32 */
WSADATA            dummy;
#endif
struct sockaddr_in ser;
struct sockaddr_in cli;

if (!strcmp(canale, LIB_TCP))  /* sono su TCP/IP */
   {
    tipocanale = LIB_TCPCOD;
    /* creo il socket */

    strcpy(st->commType.nameComm,canale);
#ifdef WIN32
    a = WSAStartup(MAKEWORD(2,2), &dummy);
    if( (st->commType.canaleFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
                                                           == INVALID_SOCKET )
#endif
#ifdef UNIX
    if( (st->commType.canaleFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
                                                           < 0 )
#endif
       return salvaErrore(st, SY1527_SYSERR, setSystemError());

    (void)setsockopt(st->commType.canaleFD, SOL_SOCKET, SO_REUSEADDR,
                                                     (char *)&on, sizeof(on));
    memset((char *)&ser, '\0', sizeof(ser));
    ser.sin_family = AF_INET;
    ser.sin_addr.s_addr = inet_addr(address);
    ser.sin_port = htons(LIB_SOCKPORT);

    memset((char *)&cli, '\0', sizeof(cli));

    if( connect_nonb(st, (struct sockaddr *)&ser, sizeof(ser), LIB_CONNECT_TIMEOUT) )
       return salvaErrore(st, SY1527_SYSERR, setSystemError());
   }
else            /* sono su linea seriale */
   {
    tipocanale = LIB_RS232COD;
    return salvaErrore(st, SY1527_NOSERIAL, LIB_NOSERIAL_STR);
   }

return salvaErrore(st, SY1527_OK, LIB_OK_STR);
}


/* CHIUDICANALE ***************************************************************
Chiude il canale di comunicazione

Parametri In
 st

  Rel. 1.4 - Introdotta la WSACleanup in parallelo alla WSAStartup presente
             nella apriCanale
           - Sostituita la shutdown con la closesocket, se defined WIN32
           
           Tutto ciò per maggior "pulizia" ed anche per correggere il baco 
           descritto in SY1527Login

 *****************************************************************************/
int chiudiCanale(STATUS *st)
{
#ifdef WIN32
/* Rel. 1.4 prima era cosi' 
    shutdown(st->commType.canaleFD, SD_BOTH);
*/
    closesocket(st->commType.canaleFD);
    WSACleanup();

#endif
#ifdef UNIX
    shutdown(st->commType.canaleFD, 2);
#endif
/* qui bisognera' disallocare la struttura st corrispondente!!!!!!! */
    return SY1527_OK;
}

/* SENDRECEIVE ****************************************************************
Funzione per inviare e ricevere un messaggio al CFE del sistema SY1527.
Ritorna il codice di errore.

Parametri In
 st        puntatore alla struttura stNow
 mess      messaggio da inviare
 messlen   lunghezza del messaggio
Parametri Out
 outlen    lunghezza del messaggio ricevuto
 outmsg    messaggio ricevuto

Rel. 2.13 - modificata la gestione degli errori, perche' probabilmente alcuni errori
            sulla rete malgestiti possono creare dei fault
Rel. 2.14 - introdotta la gestione delle recv non blocking per poter gestire la discon=
            nessione "abrupt" del peer, senza aspettare il ritorno dalla recv, visto che
			col socket in modalita' blocking, quando avviene una chiusura abrupt del peer
			la recv anziche' tornare un errore subito, torna dopo una vita (baco ?)
			Versione Win32 only
Rel. 2.15 - irrobustito il tutto introducendo la chiamata alla select dopo ogni WSAEWOULDBLOCK.
			Definite 2 funzioni separate per Win32 e Linux. Tutti questi accorgimenti riguarda=
			no solo Win32. La versione Linux per ora e' rimasta alla 2.11.
            Da tutto cio' si deduce che non si puo' usare un socket in modalita' block: se infatti
			la diconnessione avviene durante, es, la recv, questa rimane bloccata per ore (caso OPC
			Server).
			Se pero' la disconnessione avviene fuori, sembra che la recv bloccante rimanga comunque
			bloccata. 
			Versione Win32 only
 *****************************************************************************/
int sendReceive(STATUS *st, char *mess, unsigned long messLen, int *outlen, char **outmsg)
{
/*
//	Rel. 2.15 - Versione Windows
*/
#ifdef WIN32
	int				n, retval, app;
	unsigned long	*len, num_byte, ttt;
	char			*p = (char *)NULL;
	char			*msgin;
	fd_set			readmask;
	struct timeval	timeout;

	p = (char *)malloc( sizeof(unsigned long) + messLen );
	if( p == (char *)NULL )
		return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEM_FOR_MSG);
   
	len = (unsigned long *)p;
	*len = htonl(messLen);
	memcpy(p + sizeof(unsigned long), mess, messLen);

   /* si spedisce num.byte+messaggio in un colpo solo per ottimizzare la comunicazione */
	n = send(st->commType.canaleFD, p, sizeof(unsigned long) + messLen, 0);
/*   if (n < 0)   Rel. 2.13 */
	if( n == SOCKET_ERROR )	/* Rel. 2.13 */
	{
		free(p);
		return salvaErrore(st, SY1527_WRITEERR, setSystemError());
	}
	free(p);

/* 
// Aspetto la risposta 
//
// Rel. 2.15 - Gestita meglio la recv, introducendo la chiamata alla select per ogni WSAEWOULDBLOCK
//             come si fara' anche per l'RFID */
	timeout.tv_sec = LIB_TIMEOUT;
	timeout.tv_usec = 0;

/* leggo la lunghezza del messaggio */
	n = 0;
	do
	{
		int err;

		FD_ZERO(&readmask);
		FD_SET(st->commType.canaleFD, &readmask);

		retval = select(st->commType.canaleFD + 1, &readmask, NULL, NULL, &timeout);
		if( !retval )       /* timeout */
			return salvaErrore(st, SY1527_TIMEERR, LIB_TIMEERR_STR);
		else if( retval == SOCKET_ERROR )	
			return salvaErrore(st, SY1527_READERR, setSystemError());

		app = recv( st->commType.canaleFD, (char *)&ttt+n, 
					(size_t)(sizeof(unsigned long)-n), 0 );
		if( app == SOCKET_ERROR && ( err = WSAGetLastError() ) != WSAEWOULDBLOCK )
			return salvaErrore(st, SY1527_READERR, setSystemError());	/* Errore vero */
		if( app == SOCKET_ERROR )						/* WSAEWOULDBLOCK */
			continue;
		if( app == 0 )
			return salvaErrore(st, SY1527_DOWN, LIB_DOWN_STR);		/* Chiusura pulita del peer */*

		n += app;
	}
	while( (unsigned long)n < sizeof(unsigned long) );  

	num_byte = ntohl(ttt);

/* ora leggo il comando */
	msgin = malloc(num_byte);
	if( msgin == (char *)NULL )
		return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEM_FOR_MSG);
/*
devo controllare che siano arrivati tutti i num_byte, perchè la recv prende 
 byte che sono arrivati senza verificare che siano tutti quelli che aspettiamo
 Rel. 2.13 - modificata la gestione degli errori: il test su app <= 0 va fatto
             prima di sommare !!
*/
	n = 0;
	do
	{
		int err;

		FD_ZERO(&readmask);
		FD_SET(st->commType.canaleFD, &readmask);

		retval = select(st->commType.canaleFD + 1, &readmask, NULL, NULL, &timeout);
		if( !retval )       /* timeout */
			return salvaErrore(st, SY1527_TIMEERR, LIB_TIMEERR_STR);
		else if( retval == SOCKET_ERROR )	
			return salvaErrore(st, SY1527_READERR, setSystemError());

		app = recv( st->commType.canaleFD, msgin+n, (size_t)(num_byte-n), 0 );
		if( app == SOCKET_ERROR && ( err = WSAGetLastError() ) != WSAEWOULDBLOCK )
			return salvaErrore(st, SY1527_READERR, setSystemError());
		if( app == SOCKET_ERROR )
			continue;
		if( app == 0 )
			return salvaErrore(st, SY1527_DOWN, LIB_DOWN_STR);

		n += app;
	}
	while( (unsigned long)n < num_byte );  
	
/*
// setto le var. di ritorno del messaggio 
*/
	*outlen = num_byte;
	*outmsg = malloc(num_byte);
	if( *outmsg == (char *)NULL )
	{
		if (msgin) free(msgin);
		return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEM_FOR_MSG);
	}
   
	memcpy(*outmsg, msgin, (size_t)num_byte);

	if (msgin) free(msgin);

 /* la comunicazione e' andata a buon fine. Per comodita' della funzione
    chiamante setto anche i campi ack* dello stato stNow. Comunque, questi
    campi saranno immediatamente sovrascritti dalla funz. trovaAck() chiamata
    successivamente */
   return salvaErrore(st, SY1527_OK, LIB_OK_STR);
#endif

/*
//	Rel. 2.15 - Versione Linux
*/
#ifdef UNIX
   int i;								/* Rel. 2.14 */
   int n, retval, app;
   unsigned long *len, num_byte, ttt;
   char *p = (char *)NULL;
   char  *msgin;
   fd_set readmask;
   struct timeval timeout;

   p = (char *)malloc( sizeof(unsigned long) + messLen );
   if( p == (char *)NULL )
     return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEM_FOR_MSG);
   
   len = (unsigned long *)p;
   *len = htonl(messLen);
   memcpy(p + sizeof(unsigned long), mess, messLen);

   /* si spedisce num.byte+messaggio in un colpo solo per ottimizzare la comunicazione */
   n = write(st->commType.canaleFD, p, sizeof(unsigned long) + messLen);

/*   if (n < 0)   Rel. 2.13 */
	 if( n == SOCKET_ERROR )	/* Rel. 2.13 */
      {
      free(p);
      return salvaErrore(st, SY1527_WRITEERR, setSystemError());
      }

   free(p);

   /* aspetto la risposta */
/* Rel. 2.15 - Gestita meglio la recv, introducendo la chiamata alla select per ogni WSAEWOULDBLOCK
//             come si fara' anche per l'RFID
//             Per ora lascio la versione Linux cosi' come era nelle release precedenti */
   FD_ZERO(&readmask);
   FD_SET(st->commType.canaleFD, &readmask);
   timeout.tv_sec = LIB_TIMEOUT;
   timeout.tv_usec = 0;
   retval = select(st->commType.canaleFD + 1, &readmask, NULL, NULL, &timeout);
   if (!retval)       /* timeout */
       return salvaErrore(st, SY1527_TIMEERR, LIB_TIMEERR_STR);
/* rel. 2.13 */
   else if( retval == SOCKET_ERROR )	
       return salvaErrore(st, SY1527_READERR, setSystemError());

   /* leggo la lunghezza del messaggio */
   n = read(st->commType.canaleFD, (char *)&ttt, (size_t)sizeof(unsigned long));
/*   if (n < 0)   Rel. 2.13 */
   if( n == SOCKET_ERROR )	/* Rel. 2.13 */
      return salvaErrore(st, SY1527_READERR, setSystemError());
   if (n == 0)
      return salvaErrore(st, SY1527_DOWN, LIB_DOWN_STR);

   num_byte = ntohl(ttt);

   /* ora leggo il comando */
   msgin = malloc(num_byte);
   if( msgin == (char *)NULL )
     return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEM_FOR_MSG);
/*
devo controllare che siano arrivati tutti i num_byte, perchè la recv prende 
 byte che sono arrivati senza verificare che siano tutti quelli che aspettiamo
 Rel. 2.13 - modificata la gestione degli errori: il test su app <= 0 va fatto
             prima di sommare !!
*/
   n = 0;

	do
	{
		app = read(st->commType.canaleFD, msgin+n, (size_t)(num_byte-n));
		if( app == SOCKET_ERROR )
			return salvaErrore(st, SY1527_READERR, setSystemError());
		if( app == 0 )
			return salvaErrore(st, SY1527_DOWN, LIB_DOWN_STR);
		n += app;
	}
	while( (unsigned long)n != num_byte );  
	
/* Rel. 2.0 - cast (unsigned long)n per far tacere il compilatore */


   /* setto le var. di ritorno del messaggio */
   *outlen = num_byte;
   *outmsg = malloc(num_byte);
   if( *outmsg == (char *)NULL )
     {
      if (msgin) free(msgin);
      return salvaErrore(st, SY1527_MEMORYFAULT, LIB_NO_MEM_FOR_MSG);
     }
   
   memcpy(*outmsg, msgin, (size_t)num_byte);

   if (msgin) free(msgin);

 /* la comunicazione e' andata a buon fine. Per comodita' della funzione
    chiamante setto anche i campi ack* dello stato stNow. Comunque, questi
    campi saranno immediatamente sovrascritti dalla funz. trovaAck() chiamata
    successivamente */
   return salvaErrore(st, SY1527_OK, LIB_OK_STR);
#endif
}
