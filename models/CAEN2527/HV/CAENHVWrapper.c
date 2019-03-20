/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    CAENHVWRAPPER.C                                                 	   */
/*                                                                         */
/*    This is a Dynamic Link Library for Win32 platforms.                  */
/*                                                                         */
/*    Source code written in Microsoft Visual C++                          */
/*                                                                         */ 
/*    Created:  March 2000                                                 */
/*    February  2001: Rel. 1.1 : - Added CAENHVCaenetComm                  */
/*    June      2001: Rel. 1.2 : - Restored CAENHVSetSysProp               */
/*    September 2001: Rel. 1.3 : - Corrected a memory leakage problem      */
/*                               - Corrected a bug in FindSetAck           */
/*    December  2001: Rel. 1.4 : - Added "rpm" EU                          */
/*                               - Handled the close/reopen of the socket  */
/*                               - Handled the possibility of signed params*/
/*    December  2001: Rel. 2.0 : - Added support for SY127 and SY527       */
/*                                 via CAENET (A303)                       */
/*    April     2002: Rel. 2.1 : - Added support for A1303 by linking the  */
/*                                 HSCAENETLib library which also acts as  */
/*                                 a wrapper library for A303, if needed.  */
/*                                 As a by-product, we can have multiple   */	
/*                                 boards (= independent CAENET chains)    */
/*                                 in a single PC now                      */
/*                               - Modified the format of the string       */
/*                                 returned by CAENHVLibSwRel              */
/*                               - Added support for SY403                 */
/*                               - Added support of Busy (errror = -256)   */
/*                                 i.e. we don't report error but we retry */
/*                                 to execute the command                  */
/*    May       2002: Rel. 2.2 : - Added support for N470 and N570         */
/*    May       2002: Rel. 2.3 : - Optimized the N470/N570 access          */
/*                                 via CAENET                              */
/*    May       2002: Rel. 2.4 : - Added multiplatform support in the same */
/*                                 files                                   */
/*    June      2002: Rel. 2.5 : - Corrected a bug in SetChParam when the  */
/*                                 param is "Pw", the PS is N470 or N570   */
/*                                 and the card is an A303                 */
/*    September 2002: Rel. 2.6 : - Added support for N568                  */
/*    September 2002: Rel. 2.6 : - Added CAENHVSetBdParam for N568         */
/*    November  2002: Rel. 2.7 : - To generate 3.0                         */
/*    November  2002: Rel. 2.7 : - Bug corrected in N568GetBdParam and     */
/*                                 N568SetBdParam                          */
/*    December  2002: Rel. 2.8 : - Added CAENHVSetBdParam for SY1527       */
/*    March     2003: Rel. 2.9 : - Lowered the CAENET timeout from 5.0 sec */
/*                                 to 200 msec; this is due to the fact    */
/*                                 that during timeout periods the thread  */
/*                                 attached to CAENET remained "hang up"   */
/*                               - Eliminated explicit call to the routine */
/*                                 called HSCAENETSendCommand in SyIdent by*/
/*                                 substituing it with HSCAENETComm which  */
/*                                 conatins errors handling: this gave Ko  */
/*                                 connection status with opc server       */
/*                               - Delivered as testing release (only for  */
/*                                 Win32) to NA60 (Nicolas Guettet)        */
/*    May       2003: Rel. 2.10: - Made all the library functions (apart   */
/*                                 from CAENHVGetError) thread-safe by     */
/*                                 eliminating static variables and by     */
/*                                 adding critical sections where needed.  */
/*                               - Delivered as testing release (only for  */
/*                                 Win32) to NA60 (Nicolas Guettet)        */
/*    July      2003: Rel. 2.11: - Linux support and delivery              */
/*    May       2004: Rel. 2.12: - Added support for the A290C module, for */
/*                                 debugging purposes at the Finuda        */
/*                                 experiment.                             */
/*                               - Delivered as testing release (only for  */
/*                                 Win32)                                  */
/*    October   2004: Rel. 2.13: - Fixed a bug in the sendreceive routine  */
/*                                 which didn't handle correctly errors    */
/*                                 coming from the network.                */
/*                               - Delivered as testing release (only for  */
/*                                 Win32)                                  */
/*    October   2004: Rel. 2.14: - Made the socket access asynI/O          */
/*                                 Win32 only for the moment               */
/*                                 in order to avoid blocking the threads  */
/*                                 in the recv call                        */
/*                               - Delivered as testing release (only for  */
/*                                 Win32)                                  */
/*    October   2004: Rel. 2.15: - Some refinements to the 2.14            */
/*                               - Delivered as testing release (only for  */
/*                                 Win32)                                  */
/*                                                                         */
/***************************************************************************/

#define CAENHVLIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>                                  
#include "HSCAENETLib.h"		
#include "sy1527interface.h"
#include "CAENHVWrapper.h"
#include "sy127interface.h"                           /* Rel. 2.0 */
#include "sy527interface.h"                           /* Rel. 2.0 */
#include "sy403interface.h"                           /* Rel. 2.1 */
#include "n470interface.h"                            /* Rel. 2.2 */ 
#include "n568interface.h"                            /* Rel. 2.6 */
#include "a290cinterface.h"                           /* Rel. 2.12 */

/******************************************************* !!! CAEN
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
// ******************************************************* !!! CAEN */

#define  A290C_ID           -200					  /* Rel. 2.12 */
#define  N568_ID            -300					  /* Rel. 2.6  */
#define  N470_ID            -400					  /* Rel. 2.2  */
#define  SY403_ID           -500					  /* Rel. 2.1  */
#define  SY127_ID          -1000
#define  SY527_ID          -2000
#define  SY1527_ID         -3000


/*
//    Rel. 2.0 - Added cr and session fields
//  - The cr field is used only for Power Supplies connected via CAENET (i.e.
//    SY127 and SY527)
//  - The session field is >= 0 for SY1527/SY2527 and is <0 (the cr, univocally
//    defined), in particular from -1 to -99, for SY127 and SY527
//    
//    Rel. 2.1 - Added devAddr and devHandle fields in order to handle multiple 
//               CAENET controllers
//  - The devAddr field is the parameter passed by the user to identify the card
//    and must be saved to recognize when multiple CAENET Power Supplies are
//    connected to the same card 
//  - The devHandle field is used as parameter for the HSCAENETLib functions; this
//    field is opaque
*/
typedef struct HVPS_tag
{
	char			name[80];
	int			cr;
	int			Id;
	unsigned long		Ip;          /* Rel. 3.00 */
	int			session;
	unsigned		devAddr;
	int			devHandle;
} HVPS;


static int   HVPSNum = 0;

static HVPS  *sysList = NULL;

#ifdef WIN32
/* Rel. 2.10 */
static CRITICAL_SECTION sem;
static CRITICAL_SECTION semIDsNow;
#else
/* Rel. 2.11 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

static int sem,semIDsNow;

/* Rel. 2.11 - Added semaphore handling */
#if !defined(DARWIN)
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
       int val;                    /* value for SETVAL */
       struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
       unsigned short int *array;  /* array for GETALL, SETALL */
       struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif

#endif
#endif /* DARWIN */
/*
//+---------------------------------------------------------------+
//|  createsem                                                    |
//|  Rel. 2.11                                                    |
//+---------------------------------------------------------------+
*/
static void createsem()
{
#ifdef UNIX
	union semun semopts;
	key_t key;

	/* Create unique key via call to ftok() */
	key = ftok(".", '0');

	sem = semget(key, 1, IPC_CREAT|IPC_EXCL|0666);
	
	if( sem < 0 ) {
		fprintf(stderr, "Semaphore already present\r\n There is ");
		fprintf(stderr, "another process using the semaphore.\r\n O");
		fprintf(stderr, "r a process using the semaphore exited a");
		fprintf(stderr, "bnormally.\r\n In That case try to manu");
		fprintf(stderr, "ally release the semaphore with:\r\n   ");
		fprintf(stderr, "ipcrm sem XXX.\r\n");
		exit(1);
	}
	
	semopts.val = 1;
	
	semctl(sem, 0, SETVAL, semopts);
#else
	InitializeCriticalSection(&sem);	
	InitializeCriticalSection(&semIDsNow);
#endif
}

/*
//+---------------------------------------------------------------+
//|  locksem                                                      |
//|  Rel. 2.11                                                    |
//+---------------------------------------------------------------+
*/
void locksem()
{
#ifdef UNIX
	struct sembuf sem_lock={ 0, -1, 0};
		
	semop(sem, &sem_lock, 1);
#else
	EnterCriticalSection(&sem);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  locksem                                                      |
//|  Rel. 3.02                                                    |
//+---------------------------------------------------------------+
*/
void locksemIDsNow()
{
#ifdef UNIX
	struct sembuf sem_lockIDsNow={ 0, -1, 0};
		
	semop(semIDsNow, &sem_lockIDsNow, 1);
#else
	EnterCriticalSection(&semIDsNow);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  unlocksem                                                    |
//|  Rel. 2.11                                                    |
//+---------------------------------------------------------------+
*/
void unlocksem()
{
#ifdef UNIX
	struct sembuf sem_unlock={ 0, 1, 0};

	semop(sem, &sem_unlock, 1);
#else
	LeaveCriticalSection(&sem);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  unlocksem                                                    |
//|  Rel. 3.02                                                    |
//+---------------------------------------------------------------+
*/
void unlocksemIDsNow()
{
#ifdef UNIX
	struct sembuf sem_unlockIDsNow={ 0, 1, 0};

	semop(semIDsNow, &sem_unlockIDsNow, 1);
#else
	LeaveCriticalSection(&semIDsNow);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  removesem                                                    |
//|  Rel. 2.11                                                    |
//+---------------------------------------------------------------+
*/
static void removesem()
{
#ifdef UNIX
	semctl(sem, 0, IPC_RMID, 0);
#else
	DeleteCriticalSection(&sem);
	DeleteCriticalSection(&semIDsNow);
#endif
}

#ifdef WIN32
/*
	Rel. 2.10 - HVPSNum and sysList are thread unsafe, so we use a critical
				section to protect them.
			  - Added DllMain to initialize the critical section.

*/

/*
//+---------------------------------------------------------------+
//|  DLLMain                                                      |
//|                                                               |
//+---------------------------------------------------------------+
*/
BOOL WINAPI DllMain( HANDLE hModule, DWORD ul_reason_for_call, 
                     LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
				createsem(); /* Rel. 2.11 */
				/*InitializeCriticalSection(&cs);  // Rel. 2.10 */
			break;

		case DLL_PROCESS_DETACH:
				removesem(); /* Rel. 2.11 */
				/*DeleteCriticalSection(&cs);  // Rel. 2.10 */
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }
    return TRUE;
}

#else /* UNIX */
#ifndef LINUX
/*
//+---------------------------------------------------------------+
//  _init  
//  Rel. 2.11
//+---------------------------------------------------------------+
*/
void _init( void )
{
	createsem();
}

/*
//+---------------------------------------------------------------+
//  _fini   
//  Rel. 2.11
//+---------------------------------------------------------------+
*/
void _fini( void )
{	
	removesem();
}
#endif
/*
//+---------------------------------------------------------------+
//|  Sleep                                                        |
//|  Rel. 2.4 							                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
void Sleep(unsigned long x)
{
	struct timeval t;

	t.tv_sec = 0;
	t.tv_usec = (x)*1000;
	select(0, NULL, NULL, NULL, &t);
}

#endif

/*
//+----------------------------------------------------------------+
//|  SyIdent                                                       |
//|  Rel. 2.0  - Needed to identify the Power Supply ID via CAENET |
//|              We execute A303Init only the first time           |
//|  Rel. 2.1  - Modified to handle A1303 together with A303 via   |
//|              the HSCAENETLib library; we don't call A303...    |
//|              directly anymore                                  |
//|              We must handle the fact that we add more P.S.s to |
//|              the same card                                     |
//|            - Added support for Sy403                           |
//|  Rel. 2.2  - Added support for N470 and N570                   |
//|  Rel. 2.3  - Modified in order to cope with the behaviour of   |
//|              the A1303 device driver which is not shareable    |
//|  Rel. 2.6  - Added support for N568                            |
//|  Rel. 2.9  - Lowered the CAENET timeout from 5.0 sec to 50 msec|
//|  Rel. 2.9  - Eliminated HSCAENETSendCommand by substituing it  |
//|              with HSCAENETComm which handles the CAENET errors |
//|  Rel. 2.12 - Added support for A290C                           |
//|                                                                |
//+----------------------------------------------------------------+
*/
static int SyIdent(char *devName, int cr, unsigned A303IO, int *devHandle)
{
	char    tempbuff[256], syident[12];
	int     i, res, response; /* Rel. 2.9 dummy is no more needed */
	int		CardAlreadyInitd = 1;            /* Rel. 2.3 */

	res = 0;

	for( i = 0 ; i < HVPSNum ; i++ )
		if( sysList[i].devAddr == A303IO )
			break;

	if( i == HVPSNum )		   /* The card at A303IO hasn't been init yet */
	{    
		if( ( *devHandle = HSCAENETCardInit( devName, &A303IO ) ) == -1 )
			return res;

		CardAlreadyInitd = 0;				/* Rel. 2.3 */

		if( HSCAENETCardReset( *devHandle ) != TUTTOK )
			goto fine;

/* Rel. 2.9		if( HSCAENETTimeout( *devHandle, 500 ) != TUTTOK )  // 5.0 sec. */
		if( HSCAENETTimeout( *devHandle, 20 ) != TUTTOK )  /* 200 msec. */
			goto fine;
	}
	else
		*devHandle = sysList[i].devHandle;

/* Rel. 2.9 */
/*	if( HSCAENETSendCommand( *devHandle, 0, cr, NULL, 0 ) != TUTTOK )
//		goto fine;
//	else 
//	    response = HSCAENETReadResponse( *devHandle, tempbuff, &dummy );
*/
	response = HSCAENETComm( *devHandle, 0, cr, NULL, 0, tempbuff );

	if( response == TUTTOK )          
	{
		for( i = 0 ; i < 11 ; i++ )
			syident[i] = tempbuff[2*i];
		syident[i] = '\0';

		if( !strncmp(syident, "SY527", 5) )
			if( Sy527Init( *devHandle, cr ) == TUTTOK )
				res = SY527_ID;

		if( !strncmp(syident, "SY127", 5) )
			if( Sy127Init( *devHandle, cr ) == TUTTOK )
				res = SY127_ID;

		/* Rel. 2.1 - Added support for SY403 */
		if( !strncmp(syident, "SY403", 5) )
			if( Sy403Init( *devHandle, cr ) == TUTTOK )
				res = SY403_ID;

		/* Rel. 2.2 - Added support for N470, N470A and N570 */
		if( !strncmp(syident, "N470", 4) )
		{
			if( syident[4] == 'A' )
			{
				if( N470AInit( *devHandle, cr ) == TUTTOK )
					res = N470_ID;
			}
			else
			{
				if( N470Init( *devHandle, cr ) == TUTTOK )
					res = N470_ID;
			}
		}
		if( !strncmp(syident, "N570", 4) )
			if( N570Init( *devHandle, cr ) == TUTTOK )
				res = N470_ID;

		/* Rel. 2.6 - Added support for N568 and N568B */
		if( !strncmp(syident, "N568", 4) )
		{
			if( tempbuff[26]-'0' >= 2 )
			{
				if( N568BInit( *devHandle, cr ) == TUTTOK )
					res = N568_ID;
			}
			else
			{
				if( N568Init( *devHandle, cr ) == TUTTOK )
					res = N568_ID;
			}
		}

		/* Rel. 2.12 - Added support for A290C */
		if( !strncmp(syident, "VME P.S.", 8) )
			if( A290CInit( *devHandle, cr ) == TUTTOK )
				res = A290C_ID;

	}

fine:								/* Rel. 2.3 */
	if( res == 0 && CardAlreadyInitd == 0 )
		HSCAENETCardEnd( *devHandle );

	return res;
}

/*
//+----------------------------------------------------------------+
//|  AddHVPS                                                       |
//|  Rel. 2.00 - Added the control of CAENET devices.              |
//|              In case of CAENET connection, the Device field    |
//|              is: aaa_cc where cc is the caenet crate number    |
//|              ans aaa is the A303 IO Address (this must be      |
//|              better implemented ...)                           |
//|  Rel. 2.01 - Now the Arg parameter is CARD_aaa_cc where CARD   |
//|              is "A303" or "A303A" or "A1303", cc is the caenet |
//|              crate number and, if CARD == "A1303", aaa is the  |
//|              index of the board starting from 0.               |
//|              For backward compatibility reasons, we still      |
//|              accept the aaa_cc format (for A303 or A303A only) |
//|  Rel. 2.10 - Added critical section to protect golobal vars.   |
//|                                                                |
//+----------------------------------------------------------------+
*/
static int AddHVPS(const char *sysName, const char *CommPath, 
				   const char *Device, const char *User, const char *Passwd)
{
	int i, res = -1;

/*	EnterCriticalSection(&cs);	// Rel. 2.10 */
	locksem();	/* Rel. 2.11 */

	for( i = 0 ; i < HVPSNum ; i++ )
		if( !strcmp( sysList[i].name, sysName ) )
			break;

	if( i == HVPSNum )
	{
		int      id, cr, devHandle;
		unsigned A303IO;

		if(    !strcmp(CommPath,"TCP/IP") 
			|| !strcmp(CommPath,"RS232")   )
		{
			res = Sy1527Login(CommPath, Device, User, Passwd);
			id  = SY1527_ID;
		}
		else						/* Rel. 2.1 - Added devName */
		{
			char *p;
			char devName[80];

			/* Backward compatibility with Rel. 2.0 */
			if( ( p = strchr( Device, '_' ) ) == strrchr(Device, '_' ) )
			{
				if( p != NULL )					/* Format compat. with Rel. 2.0 */
				{
					strcpy(devName, "A303");
					p = strtok((char *)Device, "_");
					if( p != NULL )
					{
						sscanf(p, "%x", &A303IO);
						p = strtok(NULL, "_");
						if( p != NULL )
						{
							sscanf(p, "%x", &cr);
							id  = SyIdent(devName, cr, A303IO, &devHandle);
							res = id - 1;
						}
					}
				}
			}
			else				/* Rel. 2.1 - Format here must be: CARD_aaa_cc  */
			{
				p = strtok((char *)Device, "_");
				if( p != NULL )
				{
					strcpy(devName, p);
					p = strtok(NULL, "_");
					if( p != NULL )
					{
						sscanf(p, "%x", &A303IO);
						p = strtok(NULL, "_");
						if( p != NULL )
						{
							sscanf(p, "%x", &cr);
							id  = SyIdent(devName, cr, A303IO, &devHandle);
							res = id - 1;
						}
					}
				}
			}
		}

		if( res != -1 )
		{
			sysList = realloc(sysList, (HVPSNum + 1)*sizeof(HVPS) );
			strcpy(sysList[i].name, sysName);
			if( res < 0 )                     /* Connected to old P.S. */
			{
	   			sysList[i].session   = -cr;
				sysList[i].devHandle = devHandle;		  /* Rel. 2.1 */
				sysList[i].devAddr   = A303IO;			  /* Rel. 2.1 */
			}
			else {
				char *idx;
				unsigned long MyIP;
				unsigned char a,b,c,d; /* Connected to new P.S. */
				char tmp[17];
				char *p;
				strcpy(tmp,Device);
				p = tmp;
				sysList[i].session   = res;

				idx = strchr(p,'.');
				*idx = 0;
				a = (unsigned char) atoi(p);
				p = idx + 1;

				idx = strchr(p,'.');
				*idx = 0;
				b = (unsigned char) atoi(p);
				p = idx + 1;

				idx = strchr(p,'.');
				*idx = 0;
				c = (unsigned char) atoi(p);
				p = idx + 1;

				d = (unsigned char) atoi(p);
				
				MyIP = d << 24 | c << 16 | b << 8 | a;
				sysList[i].Ip = MyIP;			  /* Rel. 3.00 */
			}
			sysList[i].Id = id;
			sysList[i].cr = cr;
			HVPSNum++;
		}
	}

/*	LeaveCriticalSection(&cs);	// Rel. 2.10 */
	unlocksem();	/* Rel. 2.11 */

	return res;
}

/*
//+----------------------------------------------------------------+
//|  RemHVPS                                                       |
//|  Rel. 2.00 - Added the control of sy127 and sy527.             |
//|  Rel. 2.01 - Modified to handle A1303 together with A303 via   |
//|              the HSCAENETLib library; we don't call A303...    |
//|              directly anymore                                  |
//|              We must handle the fact that we can have more     |
//|              P.S.s connected to the same card                  |
//|            - Added support for sy403                           |
//|  Rel. 2.02 - Added support for N470 and N570                   |
//|  Rel. 2.06 - Added support for N568                            |
//|  Rel. 2.10 - Added critical section to protect golobal vars.   |
//|  Rel. 2.12 - Added support for A290C                           |
//|                                                                |
//+----------------------------------------------------------------+
*/
static int RemHVPS(int sysIndex)
{
	int       i = sysIndex, res = -1, caenet = 0;
	int       devHandle;								/* Rel. 2.1 */
	unsigned  devAddr;								/* Rel. 2.1 */

/*	EnterCriticalSection(&cs);	// Rel. 2.10 */
	locksem();	/* Rel. 2.11 */

	if( sysIndex < HVPSNum )
	{
		if( sysList[sysIndex].session < 0 )				/* CAENET */
		{
			caenet = 1;
			devAddr   = sysList[sysIndex].devAddr;		/* Rel. 2.1 */
			devHandle = sysList[sysIndex].devHandle;	/* Rel. 2.1 */

			switch( sysList[sysIndex].Id )
			{
			  case A290C_ID:    						/* Rel. 2.12 */
				  A290CEnd(sysList[sysIndex].devHandle, sysList[sysIndex].cr);
				  break;

			  case N568_ID:	    						/* Rel. 2.6 */
				  N568End(sysList[sysIndex].devHandle, sysList[sysIndex].cr);
				  break;

			  case N470_ID:	    						/* Rel. 2.2 */
				  N470End(sysList[sysIndex].devHandle, sysList[sysIndex].cr);
				  break;

			  case SY403_ID:							/* Rel. 2.1 */
				  Sy403End(sysList[sysIndex].devHandle, sysList[sysIndex].cr);
				  break;

			  case SY127_ID:
				  Sy127End(sysList[sysIndex].devHandle, sysList[sysIndex].cr);
				  break;

			  case SY527_ID:
				  Sy527End(sysList[sysIndex].devHandle, sysList[sysIndex].cr);
				  break;

			  default:
				  break;
			}
			res = 0;
		}
		else
			res = Sy1527Logout(sysList[sysIndex].session);

		if( res >= 0 )
		{
			int j;

			for( j = i+1 ; j < HVPSNum ; j++ )
				sysList[j-1] = sysList[j];

			HVPSNum--;
			if( HVPSNum )
				sysList = realloc(sysList, (HVPSNum + 1)*sizeof(HVPS) );
			else
			{
				free(sysList);
				sysList = NULL;
			}
		}
    
		if( caenet )
		{
			int found = 0;

/* Rel. 2.1 - Modified the search criteria in order to cope with multiple
              boards
			for( i = 0 ; i < HVPSNum ; i++ )
				if( sysList[i].session < 0 )
					found = 1;

			if( !found ) 
			{
				firstTime = 1;
				A303End();
			}
*/
			for( i = 0 ; i < HVPSNum ; i++ )
				if( sysList[i].devAddr == devAddr )
					found = 1;

			if( !found ) 
				HSCAENETCardEnd(devHandle);
		}
	}
	else
		res = -1;

/*	LeaveCriticalSection(&cs);	// Rel. 2.10 */
	unlocksem();	/* Rel. 2.11 */

	return res;
}

/*
//+---------------------------------------------------------------+
//|  SysName2Index                                                |
//|  Rel. 2.0 - In Rel. 1.x this was SysName2Id, but now I need   |
//|             the index in order to get the various fields of   |
//|             sysList[]                                         |
//|                                                               |
//+---------------------------------------------------------------+
*/
static int SysName2Index(const char *sysName)
{
	int i;

	for( i = 0 ; i < HVPSNum ; i++ )
		if( !strcmp( sysList[i].name, sysName ) )
			break;

	return ( i == HVPSNum ? -1 : i );
}

/*
//+---------------------------------------------------------------+
//|  CAENHVLibSwRel                                               |
//|  Rel. 2.1 - Added the reporting of the HSCAENETLib Release    |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API char *CAENHVLibSwRel(void)
{
	static char buff[80];

	sprintf(buff,"3.00-%s",HSCAENETLibSwRel());
	return buff;
}

/*
//+----------------------------------------------------------------+
//|  CAENHVGetError                                                |
//|  Rel. 2.0  - Added the control of sy127 and sy527.             |
//|  Rel. 2.1  - Added the devHandle parameter.                    |
//|            - Added support for sy403                           |
//|  Rel. 2.2  - Added support for N470 and N570                   |
//|  Rel. 2.6  - Added support for N568                            |
//|  Rel. 2.10 - Must be made thread-safe (with TLS ?)             |
//|  Rel. 2.12 - Added support for A290C                           |
//|                                                                |
//+----------------------------------------------------------------+
*/
CAENHVLIB_API char *CAENHVGetError(const char *SystemName)
{
	int index = SysName2Index(SystemName);

	if( index == -1 )
		return " Connection failed ";
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetError(sysList[index].session);
				break;
			case A290C_ID:
				return A290CGetError(sysList[index].devHandle, sysList[index].cr);
				break;
			case N568_ID:
				return N568GetError(sysList[index].devHandle, sysList[index].cr);
				break;
			case N470_ID:
				return N470GetError(sysList[index].devHandle, sysList[index].cr);
				break;
			case SY403_ID:
				return Sy403GetError(sysList[index].devHandle, sysList[index].cr);
				break;
			case SY527_ID:
				return Sy527GetError(sysList[index].devHandle, sysList[index].cr);
				break;
			case SY127_ID:
				return Sy127GetError(sysList[index].devHandle, sysList[index].cr);
				break;
			default:
				return "";
				break;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVInitSystem                                             |
//|  Rel. 2.0 - Added the control of sy127 and sy527.             |
//|  Rel. 2.1 - In Rel. 2.0, in case of CAENET connection the Arg |
//|             parameter is: aaa_cc where cc is the caenet crate |
//|             number and aaa is the A303 IO Address.            |
//|             Now the Arg parameter is CARD_aaa_cc where CARD   |
//|             is "A303" or "A303A" or "A1303", cc is the caenet |
//|             crate number and aaa, if CARD == "A1303", is the  |
//|             index of the board starting from 0.               |
//|             For backward compatibility reasons, we still      |
//|             accept the aaa_cc format (for A303 or A303A only) |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVInitSystem
(const char *SystemName, int LinkType, void *Arg, const char *us, const char *pw)
{
	char         *CommPath;
	CAENHVRESULT res = CAENHV_OK;

	switch( LinkType )
	{
	case LINKTYPE_TCPIP:
		CommPath = "TCP/IP";
		break;
	case LINKTYPE_CAENET:
		CommPath = "CAENET";
		break;					
	case LINKTYPE_RS232:
		CommPath = "RS232";
		break;
	default:
		res = CAENHV_LINKNOTSUPPORTED;
		break;
	}
		
	if( res != CAENHV_OK )
		return res;
	else {
		locksemIDsNow();
		if( AddHVPS( SystemName, CommPath, Arg, us, pw ) != -1 ) {
		unlocksemIDsNow();
		return res;
		}
		else {
			unlocksemIDsNow();
			return CAENHV_LOGINFAILED;
		}
	}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVDeinitSystem                                           |
//|  Rel. 2.0 - We work now with index instead of Id as in 1.x    |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVDeinitSystem(const char *SystemName)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == -1 )
		return CAENHV_NOTCONNECTED;
	else
	{
		locksemIDsNow();
		if( RemHVPS(index) == -1 ) {
			unlocksemIDsNow();
			return CAENHV_LOGOUTFAILED;
		}
		else {
			unlocksemIDsNow();
			return CAENHV_OK;
		}
	}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetChName                                              |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|															      |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetChName
(const char *SystemName, ushort slot, ushort ChNum, const ushort *ChList, 
 char (*ChNameList)[MAX_CH_NAME])
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetChName(sysList[index].session, slot, ChNum, 
				                       ChList, ChNameList);
				break;
			case A290C_ID:
				return A290CGetChName(sysList[index].devHandle, sysList[index].cr, 
					                  slot, ChNum, ChList, ChNameList);
				break;
			case N568_ID:
				return N568GetChName(sysList[index].devHandle, sysList[index].cr, 
					                  slot, ChNum, ChList, ChNameList);
				break;
			case N470_ID:
				return N470GetChName(sysList[index].devHandle, sysList[index].cr, 
					                  slot, ChNum, ChList, ChNameList);
				break;
			case SY403_ID:
				return Sy403GetChName(sysList[index].devHandle, sysList[index].cr, 
					                  slot, ChNum, ChList, ChNameList);
				break;
			case SY527_ID:
				return Sy527GetChName(sysList[index].devHandle, sysList[index].cr, 
					                  slot, ChNum, ChList, ChNameList);
				break;
			case SY127_ID:
				return Sy127GetChName(sysList[index].devHandle, sysList[index].cr, 
					                  slot, ChNum, ChList, ChNameList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVSetChName                                              |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVSetChName
(const char *SystemName, ushort slot, ushort ChNum, const ushort *ChList, 
 const char *ChName)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527SetChName(sysList[index].session, slot, ChNum, 
				                       ChList, ChName);
				break;
			case A290C_ID:
				return A290CSetChName(sysList[index].devHandle, sysList[index].cr,
					                  slot, ChNum, ChList, ChName);
				break;
			case N568_ID:
				return N568SetChName(sysList[index].devHandle, sysList[index].cr,
					                  slot, ChNum, ChList, ChName);
				break;
			case N470_ID:
				return N470SetChName(sysList[index].devHandle, sysList[index].cr,
					                  slot, ChNum, ChList, ChName);
				break;
			case SY403_ID:
				return Sy403SetChName(sysList[index].devHandle, sysList[index].cr,
					                  slot, ChNum, ChList, ChName);
				break;
			case SY527_ID:
				return Sy527SetChName(sysList[index].devHandle, sysList[index].cr,
					                  slot, ChNum, ChList, ChName);
				break;
			case SY127_ID:
				return Sy127SetChName(sysList[index].devHandle, sysList[index].cr, 
									  slot, ChNum, ChList, ChName);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetChParamInfo                                         |
//|  The user must free the memory allocated for the ParNames     |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetChParamInfo
(const char *SystemName, ushort slot, ushort Ch, char **ParNameList)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetChParamInfo(sysList[index].session, slot, 
				                            Ch, ParNameList);
				break;
			case A290C_ID:
				return A290CGetChParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParNameList);
				break;
			case N568_ID:
				return N568GetChParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParNameList);
				break;
			case N470_ID:
				return N470GetChParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParNameList);
				break;
			case SY403_ID:
				return Sy403GetChParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParNameList);
				break;
			case SY527_ID:
				return Sy527GetChParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParNameList);
				break;
			case SY127_ID:
				return Sy127GetChParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParNameList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetChParamProp                                         |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetChParamProp
(const char *SystemName, ushort slot, ushort Ch, const char *ParName, 
 const char *PropName, void *retval)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetChParamProp(sysList[index].session, slot, 
				                            Ch, ParName, PropName, retval);
				break;
			case A290C_ID:
				return A290CGetChParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParName, PropName, retval);
				break;
			case N568_ID:
				return N568GetChParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParName, PropName, retval);
				break;
			case N470_ID:
				return N470GetChParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParName, PropName, retval);
				break;
			case SY403_ID:
				return Sy403GetChParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParName, PropName, retval);
				break;
			case SY527_ID:
				return Sy527GetChParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParName, PropName, retval);
				break;
			case SY127_ID:
				return Sy127GetChParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, Ch, 
					                       ParName, PropName, retval);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetChParam                                             |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetChParam
(const char *SystemName, ushort slot, const char *ParName, ushort ChNum, 
 const ushort *ChList, void *ParValList)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetChParam(sysList[index].session, slot, 
				                        ParName, ChNum, ChList, ParValList);
				break;
			case A290C_ID:
				return A290CGetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValList);
				break;
			case N568_ID:
				return N568GetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValList);
				break;
			case N470_ID:
				return N470GetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValList);
				break;
			case SY403_ID:
				return Sy403GetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValList);
				break;
			case SY527_ID:
				return Sy527GetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValList);
				break;
			case SY127_ID:
				return Sy127GetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVSetChParam                                             |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVSetChParam
(const char *SystemName, ushort slot, const char *ParName, ushort ChNum, 
 const ushort *ChList, void *ParValue)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527SetChParam(sysList[index].session, slot, 
				                        ParName, ChNum, ChList, ParValue);
				break;
			case A290C_ID:
				return A290CSetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValue);
				break;
			case N568_ID:
				return N568SetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValue);
				break;
			case N470_ID:
				return N470SetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValue);
				break;
			case SY403_ID:
				return Sy403SetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValue);
				break;
			case SY527_ID:
				return Sy527SetChParam(sysList[index].devHandle, sysList[index].cr,
					                   slot, ParName, ChNum, 
									   ChList, ParValue);
				break;
			case SY127_ID:
				return Sy127SetChParam(sysList[index].devHandle, sysList[index].cr, 
					                   slot, ParName, ChNum, 
									   ChList, ParValue);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVTestBdPresence                                         |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVTestBdPresence
(const char *SystemName, ushort slot, ushort *NrofCh, char *Model, 
 char *Description, ushort *SerNum, uchar *FmwRelMin, uchar *FmwRelMax)
{
	uchar		 Cr;
	ushort       *dummy;
	ulong        *slots;
	CAENHVRESULT res;

/* Bisognera' gestire meglio il numero di crate nel cluster */
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
	{
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				res = Sy1527TestBdPresence(sysList[index].session, slot, Model, 
				                           Description, SerNum, FmwRelMin, 
										   FmwRelMax);
				break;
			case A290C_ID:
				res = A290CTestBdPresence(sysList[index].devHandle, sysList[index].cr,
					                      slot, Model, 
					                      Description, SerNum, FmwRelMin, FmwRelMax);
				break;
			case N568_ID:
				res = N568TestBdPresence(sysList[index].devHandle, sysList[index].cr,
					                      slot, Model, 
					                      Description, SerNum, FmwRelMin, FmwRelMax);
				break;
			case N470_ID:
				res = N470TestBdPresence(sysList[index].devHandle, sysList[index].cr,
					                      slot, Model, 
					                      Description, SerNum, FmwRelMin, FmwRelMax);
				break;	
			case SY403_ID:
				res = Sy403TestBdPresence(sysList[index].devHandle, sysList[index].cr,
					                      slot, Model, 
					                      Description, SerNum, FmwRelMin, FmwRelMax);
				break;	
			case SY527_ID:
				res = Sy527TestBdPresence(sysList[index].devHandle, sysList[index].cr,
					                      slot, Model, 
					                      Description, SerNum, FmwRelMin, FmwRelMax);
				break;				/* Rel. 2.1 !!! */
			case SY127_ID:
				res = Sy127TestBdPresence(sysList[index].devHandle, sysList[index].cr, 
					                      slot, Model, 
					                      Description, SerNum, FmwRelMin, FmwRelMax);
				break;
		}
 
		if( res != CAENHV_OK )
			return res;
		else
		{
			switch( sysList[index].Id )
			{
				case SY1527_ID:
				res = Sy1527GetSysComp(sysList[index].session, &Cr, &dummy, &slots);
				break;
				case A290C_ID:
				res = A290CGetSysComp(sysList[index].devHandle, sysList[index].cr, 
					                  &Cr, &dummy, &slots);
				break;
				case N568_ID:
				res = N568GetSysComp(sysList[index].devHandle, sysList[index].cr, 
					                  &Cr, &dummy, &slots);
				break;
				case N470_ID:
				res = N470GetSysComp(sysList[index].devHandle, sysList[index].cr, 
					                  &Cr, &dummy, &slots);
				break;
				case SY403_ID:
				res = Sy403GetSysComp(sysList[index].devHandle, sysList[index].cr, 
					                  &Cr, &dummy, &slots);
				break;
				case SY527_ID:
				res = Sy527GetSysComp(sysList[index].devHandle, sysList[index].cr, 
					                  &Cr, &dummy, &slots);
				break;
				case SY127_ID:
				res = Sy127GetSysComp(sysList[index].devHandle, sysList[index].cr, 
					                  &Cr, &dummy, &slots);
				break;
			}

			if( res != SY1527_OK )
				return res;
			else
			{
				*NrofCh = (ushort)(slots[slot] & 0xffff);
				free(dummy), free(slots);
				return CAENHV_OK;
			}
		}
	}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetBdParamInfo                                         |
//|  The user must free the memory allocated for the ParNames     |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetBdParamInfo
(const char *SystemName, ushort slot, char **ParNameList)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetBdParamInfo(sysList[index].session, slot, 
				                            ParNameList);
				break;
			case A290C_ID:
				return A290CGetBdParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, 
					                       ParNameList);
				break;
			case N568_ID:
				return N568GetBdParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, 
					                       ParNameList);
				break;
			case N470_ID:
				return N470GetBdParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, 
					                       ParNameList);
				break;
			case SY403_ID:
				return Sy403GetBdParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, 
					                       ParNameList);
				break;
			case SY527_ID:
				return Sy527GetBdParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, 
					                       ParNameList);
				break;
			case SY127_ID:
				return Sy127GetBdParamInfo(sysList[index].devHandle, 
					                       sysList[index].cr, slot, 
					                       ParNameList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetBdParamProp                                         |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetBdParamProp
(const char *SystemName, ushort slot, const char *ParName, 
 const char *PropName, void *retval)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetBdParamProp(sysList[index].session, slot, 
				                            ParName, PropName, retval);
				break;
			case A290C_ID:
				return A290CGetBdParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, ParName, 
					                       PropName, retval);
				break;
			case N568_ID:
				return N568GetBdParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, ParName, 
					                       PropName, retval);
				break;
			case N470_ID:
				return N470GetBdParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, ParName, 
					                       PropName, retval);
				break;
			case SY403_ID:
				return Sy403GetBdParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, ParName, 
					                       PropName, retval);
				break;
			case SY527_ID:
				return Sy527GetBdParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, ParName, 
					                       PropName, retval);
				break;
			case SY127_ID:
				return Sy127GetBdParamProp(sysList[index].devHandle, 
					                       sysList[index].cr, slot, ParName, 
					                       PropName, retval);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetBdParam                                             |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetBdParam
(const char *SystemName, ushort slotNum, const ushort *slotList, 
 const char *ParName, void *ParValList)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetBdParam(sysList[index].session, slotNum, 
				                        slotList, ParName, ParValList);
				break;
			case A290C_ID:
				return A290CGetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
					                   ParName, ParValList);
				break;
			case N568_ID:
				return N568GetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
					                   ParName, ParValList);
				break;
			case N470_ID:
				return N470GetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
					                   ParName, ParValList);
				break;
			case SY403_ID:
				return Sy403GetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
					                   ParName, ParValList);
				break;
			case SY527_ID:
				return Sy527GetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
					                   ParName, ParValList);
				break;
			case SY127_ID:
				return Sy127GetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
					                   ParName, ParValList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVSetBdParam                                             |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.6  - This procedure is active since Rel. 2.6          |
//|  Rel. 2.8  - Added support for SY1527                         |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVSetBdParam
(const char *SystemName, ushort slotNum, const ushort *slotList, 
 const char *ParName, void *ParValue)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
/* Rel. 2.8 - Introduced */
				return Sy1527SetBdParam(sysList[index].session, slotNum, 
				                        slotList, ParName, ParValue);
				break;
			case A290C_ID:
				return A290CSetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
									   ParName, ParValue);
				break;
			case N568_ID:
				return N568SetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
									   ParName, ParValue);
				break;
			case N470_ID:
#if 0
				return N470SetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
									   ParName, ParValue);
#else
				return CAENHV_OK;
#endif
				break;
			case SY403_ID:
#if 0
				return Sy403SetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
									   ParName, ParValue);
#else
				return CAENHV_OK;
#endif
				break;
			case SY527_ID:
#if 0
				return Sy527SetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
									   ParName, ParValue);
#else
				return CAENHV_OK;
#endif
				break;
			case SY127_ID:
#if 0
				return Sy127SetBdParam(sysList[index].devHandle, sysList[index].cr, 
					                   slotNum, slotList, 
									   ParName, ParValue);
#else
				return CAENHV_OK;
#endif
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetCrateMap                                            |
//|  The user must free the memory allocated for the fields       |
//|  indicated by the keyword "List"                              |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetCrateMap
(const char *SystemName, ushort *NrOfSlot, ushort **NrofChList, char **ModelList, 
 char **DescriptionList, ushort **SerNumList, uchar **FmwRelMinList, 
 uchar **FmwRelMaxList)
{
	uchar		     Cr;
	ushort       *dummy;
	ulong        *slots;
	CAENHVRESULT res;

/* Bisognera' gestire meglio il numero di crate nel cluster */
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
	{
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				res = Sy1527GetCrateMap(sysList[index].session, NrOfSlot, 
										ModelList, DescriptionList, SerNumList, 
										FmwRelMinList, FmwRelMaxList);
				break;
			case A290C_ID:
				res = A290CGetCrateMap(sysList[index].devHandle, sysList[index].cr, 
					                   NrOfSlot, ModelList, 
					                   DescriptionList, SerNumList, FmwRelMinList, 
									   FmwRelMaxList);
				break;
			case N568_ID:
				res = N568GetCrateMap(sysList[index].devHandle, sysList[index].cr, 
					                   NrOfSlot, ModelList, 
					                   DescriptionList, SerNumList, FmwRelMinList, 
									   FmwRelMaxList);
				break;
			case N470_ID:
				res = N470GetCrateMap(sysList[index].devHandle, sysList[index].cr, 
					                   NrOfSlot, ModelList, 
					                   DescriptionList, SerNumList, FmwRelMinList, 
									   FmwRelMaxList);
				break;
			case SY403_ID:
				res = Sy403GetCrateMap(sysList[index].devHandle, sysList[index].cr, 
					                   NrOfSlot, ModelList, 
					                   DescriptionList, SerNumList, FmwRelMinList, 
									   FmwRelMaxList);
				break;
			case SY527_ID:
				res = Sy527GetCrateMap(sysList[index].devHandle, sysList[index].cr, 
					                   NrOfSlot, ModelList, 
					                   DescriptionList, SerNumList, FmwRelMinList, 
									   FmwRelMaxList);
				break;
			case SY127_ID:
				res = Sy127GetCrateMap(sysList[index].devHandle, sysList[index].cr, 
					                   NrOfSlot, ModelList, 
					                   DescriptionList, SerNumList, FmwRelMinList, 
									   FmwRelMaxList);
				break;
		}

		if( res != SY1527_OK )
			return res;
		else
		{
			switch( sysList[index].Id )
			{
				case SY1527_ID:
					res = Sy1527GetSysComp(sysList[index].session, &Cr, &dummy, 
					                       &slots);
					break;
				case A290C_ID:
					res = A290CGetSysComp(sysList[index].devHandle, sysList[index].cr, 
						                  &Cr, &dummy, &slots);
					break;
				case N568_ID:
					res = N568GetSysComp(sysList[index].devHandle, sysList[index].cr, 
						                  &Cr, &dummy, &slots);
					break;
				case N470_ID:
					res = N470GetSysComp(sysList[index].devHandle, sysList[index].cr, 
						                  &Cr, &dummy, &slots);
					break;
				case SY403_ID:
					res = Sy403GetSysComp(sysList[index].devHandle, sysList[index].cr, 
						                  &Cr, &dummy, &slots);
					break;
				case SY527_ID:
					res = Sy527GetSysComp(sysList[index].devHandle, sysList[index].cr, 
						                  &Cr, &dummy, &slots);
					break;
				case SY127_ID:
					res = Sy127GetSysComp(sysList[index].devHandle, sysList[index].cr, 
						                  &Cr, &dummy, &slots);
					break;
			}

			if( res != SY1527_OK )
				return res;
			else
			{
				int i;

				*NrofChList = malloc(*dummy*sizeof(short));
				for( i = 0 ; i < *dummy ; i++ )
					*(*NrofChList+i) = (ushort)(slots[i] & 0xffff);

/* Rel. 2.0 - Baco mangia memoria ? Non facevo la free di dummy e slots */
				free(dummy), free(slots);

				return CAENHV_OK;
			}
		}
	}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetExecCommList                                        |
//|  The user must free the memory allocated for the Command List |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetExecCommList
(const char *SystemName, ushort *NumComm, char **CommNameList)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetExecCommList(sysList[index].session, NumComm, 
				                             CommNameList);
				break;
			case A290C_ID:
				return A290CGetExecCommList(sysList[index].devHandle, sysList[index].cr, 
					                        NumComm, CommNameList);
				break;
			case N568_ID:
				return N568GetExecCommList(sysList[index].devHandle, sysList[index].cr, 
					                        NumComm, CommNameList);
				break;
			case N470_ID:
				return N470GetExecCommList(sysList[index].devHandle, sysList[index].cr, 
					                        NumComm, CommNameList);
				break;
			case SY403_ID:
				return Sy403GetExecCommList(sysList[index].devHandle, sysList[index].cr, 
					                        NumComm, CommNameList);
				break;
			case SY527_ID:
				return Sy527GetExecCommList(sysList[index].devHandle, sysList[index].cr, 
					                        NumComm, CommNameList);
				break;
			case SY127_ID:
				return Sy127GetExecCommList(sysList[index].devHandle, sysList[index].cr, 
										    NumComm, CommNameList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVExecComm                                               |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVExecComm
(const char *SystemName, const char *CommName)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527ExecComm(sysList[index].session, CommName);
				break;
			case A290C_ID:
				return A290CExecComm(sysList[index].devHandle, sysList[index].cr, 
					                 CommName);
				break;
			case N568_ID:
				return N568ExecComm(sysList[index].devHandle, sysList[index].cr, 
					                 CommName);
				break;
			case N470_ID:
				return N470ExecComm(sysList[index].devHandle, sysList[index].cr, 
					                 CommName);
				break;
			case SY403_ID:
				return Sy403ExecComm(sysList[index].devHandle, sysList[index].cr, 
					                 CommName);
				break;
			case SY527_ID:
				return Sy527ExecComm(sysList[index].devHandle, sysList[index].cr, 
					                 CommName);
				break;
			case SY127_ID:
				return Sy127ExecComm(sysList[index].devHandle, sysList[index].cr, 
					                 CommName);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetSysPropList                                         |
//|  The user must free the memory allocated for the System       |
//|  Properties List                                              |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetSysPropList
(const char *SystemName, ushort *NumProp, char **PropNameList)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetSysPropList(sysList[index].session, NumProp, 
				                            PropNameList);
				break;
			case A290C_ID:
				return A290CGetSysPropList(sysList[index].devHandle, sysList[index].cr,
										   NumProp, PropNameList);
				break;
			case N568_ID:
				return N568GetSysPropList(sysList[index].devHandle, sysList[index].cr,
										   NumProp, PropNameList);
				break;
			case N470_ID:
				return N470GetSysPropList(sysList[index].devHandle, sysList[index].cr,
										   NumProp, PropNameList);
				break;
			case SY403_ID:
				return Sy403GetSysPropList(sysList[index].devHandle, sysList[index].cr,
										   NumProp, PropNameList);
				break;
			case SY527_ID:
				return Sy527GetSysPropList(sysList[index].devHandle, sysList[index].cr,
										   NumProp, PropNameList);
				break;
			case SY127_ID:
				return Sy127GetSysPropList(sysList[index].devHandle, sysList[index].cr, 
										   NumProp, PropNameList);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetSysPropInfo                                         |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetSysPropInfo
(const char *SystemName, const char *PropName, 
 unsigned *PropMode, unsigned *PropType)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetSysPropInfo(sysList[index].session, PropName, 
											PropMode, PropType);
				break;
			case A290C_ID:
				return A290CGetSysPropInfo(sysList[index].devHandle, sysList[index].cr,
					                       PropName, PropMode, PropType);
				break;
			case N568_ID:
				return N568GetSysPropInfo(sysList[index].devHandle, sysList[index].cr,
					                       PropName, PropMode, PropType);
				break;
			case N470_ID:
				return N470GetSysPropInfo(sysList[index].devHandle, sysList[index].cr,
					                       PropName, PropMode, PropType);
				break;
			case SY403_ID:
				return Sy403GetSysPropInfo(sysList[index].devHandle, sysList[index].cr,
					                       PropName, PropMode, PropType);
				break;
			case SY527_ID:
				return Sy527GetSysPropInfo(sysList[index].devHandle, sysList[index].cr,
					                       PropName, PropMode, PropType);
				break;
			case SY127_ID:
				return Sy127GetSysPropInfo(sysList[index].devHandle, sysList[index].cr, 
										   PropName, PropMode, PropType);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetSysProp                                             |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetSysProp
(const char *SystemName, const char *PropName, void *Result)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527GetSysProp(sysList[index].session, PropName, Result);
				break;
			case A290C_ID:
				return A290CGetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Result);
				break;
			case N568_ID:
				return N568GetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Result);
				break;
			case N470_ID:
				return N470GetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Result);
				break;
			case SY403_ID:
				return Sy403GetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Result);
				break;
			case SY527_ID:
				return Sy527GetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Result);
				break;
			case SY127_ID:
				return Sy127GetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Result);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVSetSysProp                                             |
//|  Rel. 2.0  - We work now with index instead of Id as in 1.x   |
//|  Rel. 2.1  - Added the devHandle parameter.                   |
//|            - Added support for sy403                          |
//|  Rel. 2.2  - Added support for N470 and N570                  |
//|  Rel. 2.6  - Added support for N568                           |
//|  Rel. 2.12 - Added support for A290C                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVSetSysProp
(const char *SystemName, const char *PropName, void *Set)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527SetSysProp(sysList[index].session, PropName, Set);
				break;
			case A290C_ID:
				return A290CSetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Set);
				break;
			case N568_ID:
				return N568SetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Set);
				break;
			case N470_ID:
				return N470SetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Set);
				break;
			case SY403_ID:
				return Sy403SetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Set);
				break;
			case SY527_ID:
				return Sy527SetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Set);
				break;
			case SY127_ID:
				return Sy127SetSysProp(sysList[index].devHandle, sysList[index].cr, 
					                   PropName, Set);
				break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVCaenetComm                                             |
//|  Rel. 1.1 - Created                                           |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVCaenetComm
(const char *SystemName, ushort Crate, ushort Code, ushort NrWCode, 
 ushort *WCode, short *Result, ushort *NrOfData, ushort **Data)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].session)
		{
			case SY1527_ID:
				return Sy1527CaenetComm(sysList[index].session, Crate, Code,
                       NrWCode, WCode, Result, NrOfData, Data);
			break;
			default:
				return CAENHV_NOTCONNECTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVSubscribe                                              |
//|  Rel. 3.00 - Created                                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVSubscribe
(const char *SystemName, short UDPPort, ushort NrOfItems, const char *ListOfItems, 
 char *ListOfCodes)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527Subscribe(sysList[index].session, UDPPort, NrOfItems, 
					   ListOfItems, ListOfCodes);
				break;
			default:
				return CAENHV_LINKNOTSUPPORTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVUnSubscribe                                            |
//|  Rel. 3.00 - Created                                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVUnSubscribe(const char *SystemName, short UDPPort, 
 ushort NrOfIDs, const char *ListOfItems, char *ListOfErrorCodes)
{
	int index;

	if( ( index = SysName2Index(SystemName) ) == - 1)
		return CAENHV_NOTCONNECTED;
	else
		switch( sysList[index].Id )
		{
			case SY1527_ID:
				return Sy1527UnSubscribe(sysList[index].session, UDPPort, NrOfIDs, ListOfItems, 
					   ListOfErrorCodes);

				break;
			default:
				return CAENHV_LINKNOTSUPPORTED;
		}
}

/*
//+---------------------------------------------------------------+
//|  CAENHVGetEventData                                           |
//|  Rel. 3.00 - Created                                          |
//|                                                               |
//+---------------------------------------------------------------+
*/
CAENHVLIB_API CAENHVRESULT CAENHVGetEventData(SOCKET sck, CAENHVEVENT_TYPE **EventData, unsigned int *number)
{
	struct sockaddr_in addr;
	char bufferTrue[MAXLINE], *buffer;
	int len= sizeof(addr), i, result,j;
	int sysIdx,ret =0,size = 0;
	unsigned char major=0,minor=0;
	int status;
	unsigned short packetlength = 0;
	struct timeval tv;
	unsigned long lHandle;
	int type;
	IDINFO *p;
	buffer = bufferTrue;
	result = Sy1527GetEventData(sck, buffer,&addr, &len);

	if( result < 1 )
		return CAENHV_SYSERR;

	for( i = 0 ; i < HVPSNum ; i++ )
		if( sysList[i].Ip == addr.sin_addr.s_addr )
			break;
	if (i == HVPSNum) return CAENHV_NODATA;
	sysIdx = i;
	if (result < 8) return CAENHV_NODATA;
	if (memcmp(buffer,"CAEN",4) ) return CAENHV_NODATA;
	buffer += 4;
	major = *(unsigned char *) buffer;
	buffer += 1; 
	minor = *(unsigned char *) buffer;
	buffer += 1;
	if ( (major == 1) && (minor == 0) ) {
	
		/*protocol version 1.0 (single data from SYX527) */
		*EventData = (CAENHVEVENT_TYPE *) malloc(sizeof(CAENHVEVENT_TYPE));
		memset((*EventData)->Tvalue,0,256);
		type = buffer[0];
		buffer++;
		status = buffer[0];
		if (status != 1) {
			if (*EventData != NULL) free(*EventData);
			return CAENHV_NODATA;
		}
		buffer += 1;
		tv.tv_sec = ntohl(*(long *)buffer);
		buffer += 4;
		tv.tv_usec = ntohl(*(long *)buffer);
		buffer += 4;
		lHandle = ntohl(*(long *)buffer);
		buffer += 4;
		locksemIDsNow();
		p = FindID(sysIdx, lHandle);
		unlocksemIDsNow();
		if (p == NULL) {
			if (*EventData != NULL) free(*EventData);
			return CAENHV_NODATA;
		} 
		strcpy((*EventData)->ItemID,p->Item);
		
		switch(p->Type) { 
				case TYPE_CHANNEL_PAR:
				case TYPE_BOARD_PAR:
 					ret = FindBdChValPar(buffer, &(p->ParItem), (CAENHVEVENT_TYPE_ALIAS *) *EventData);
					if (ret == -1) {
						if (type) {
							len = ntohl(*(long *)buffer);
							buffer += 4;
							memcpy((*EventData)->Tvalue,buffer,len);
						}
						else {
							len = ntohl(*(long *)buffer);
							buffer += 4;
							*(long *) (*EventData)->Lvalue = ntohl(*(long *)buffer);
						}
					}
					break;
				case TYPE_SYSTEM_PROP:
 					FindSysPar(sysIdx, buffer, &(p->ParItem), (CAENHVEVENT_TYPE_ALIAS *) *EventData);
					break;
				case TYPE_ALARM:
 					FindAlarmPar(buffer, &(p->ParItem), (CAENHVEVENT_TYPE_ALIAS *) *EventData);
					break;
			}
		*number = 1;
	}
	else {
		/* Protocol version 1.1 (Multiple data from SYX527) */
		packetlength = ntohs(*(short *)buffer);
		if (packetlength != result) {
			return CAENHV_NODATA;
		}
		buffer += 2;
		*number = ntohs(*(unsigned short *)buffer);
		buffer += 2;
		*EventData = (CAENHVEVENT_TYPE *) malloc(sizeof(CAENHVEVENT_TYPE) * (*number));

		for (j=0;j<*number;j++) {
			memset((*EventData)[j].Tvalue,0,256);
			type = buffer[0];
			buffer++;
			status = buffer[0];
			if (status != 1) {
				if (*EventData != NULL) free(*EventData);
				return CAENHV_NODATA;
			}
			buffer += 1;
			tv.tv_sec = ntohl(*(long *)buffer);
			buffer += 4;
			tv.tv_usec = ntohl(*(long *)buffer);
			buffer += 4;
			lHandle = ntohl(*(long *)buffer);
			buffer += 4;
			locksemIDsNow();
			p = FindID(sysIdx, lHandle);
			unlocksemIDsNow();
			if (p == NULL) {
				if (*EventData != NULL) free(*EventData);
				return CAENHV_NODATA;
			}
			strcpy((*EventData)[j].ItemID,p->Item);
			
			switch(p->Type) { 
					case TYPE_CHANNEL_PAR:
					case TYPE_BOARD_PAR:
 						ret = FindBdChValPar(buffer, &(p->ParItem), (CAENHVEVENT_TYPE_ALIAS *) &((*EventData)[j]));
						if (ret == -1) {
							if (type) {
								len = ntohl(*(long *)buffer);
								buffer += 4;
								memcpy((*EventData)[j].Tvalue,buffer,len);
							}
							else {
								len = ntohl(*(long *)buffer);
								buffer += 4;
								*(long *) (*EventData)[j].Lvalue = ntohl(*(long *)buffer);
							}
						}
						else {
							buffer += 8;
						}
						break;
					case TYPE_SYSTEM_PROP:
 						FindSysPar(sysIdx, buffer, &(p->ParItem), (CAENHVEVENT_TYPE_ALIAS *) &((*EventData)[j]));
						size = ntohl(*(long *)buffer);
						buffer +=4;
						buffer += size;
						break;
					case TYPE_ALARM:
 						FindAlarmPar(buffer, &(p->ParItem), (CAENHVEVENT_TYPE_ALIAS *) &((*EventData)[j]));
						buffer +=4;
						buffer += size;
						break;
				}
		
		}

	}
	return 0;
}

CAENHVLIB_API CAENHVRESULT CAENHVFreeEventData(CAENHVEVENT_TYPE **EventData) {

	if (*EventData != NULL )free(*EventData);
	return 0;
}
