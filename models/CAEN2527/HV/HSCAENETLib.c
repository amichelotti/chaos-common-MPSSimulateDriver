/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    HSCAENETLib.c                                                        */
/*                                                                         */
/*    This is the source code for a library for Win32 (DLL) or for Linux.  */
/*    Its functions permit any kind of CAENET operation directly through   */
/*    A1303 model or, indirectly by calling A303lib functions, through     */
/*    A303/A model.                                                        */
/*                                                                         */
/*    Source code written in ANSI C with portions in Microsoft Visual C++  */
/*    for the DLL specific parts.                                          */ 
/*                                                                         */ 
/*    Created:  March 2002                                                 */
/*    April   2002: Rel. 1.1 : - Corrected a bug in HSCAENETComm which     */
/*		                 resulted in an error if communicating     */
/*		                 via A303 or A303A                         */						
/*    May     2002: Rel. 1.2 : - Porting for linux                         */
/*                  Rel. 1.3 : - Linux semaphores compatibilty with        */
/*         			 windows critical section.                 */
/*    October 2002: Rel. 1.4 : - Added the E_TX_TIMEOUT error code coming  */
/*             			 from the Windows driver.                  */
/*                             - Added a delay before the sendcommand in   */
/*                               order to avoid the caenet intrinsic       */
/*                               problem of the timeouts when interrogating*/
/*                               a module in a branch where more than one  */
/*                               module is located.                        */
/*                             - Modified HSCAENETComm in order to retry   */
/*                               the communication if a CAENET error occurs*/
/*                             - Delivered as testing release (only for    */
/*                               Win32) to NA60 (Nicolas Guettet)          */
/*   December 2002: Rel. 1.4 : - Ported on Linux and delivered officially  */
/*   March    2003: Rel. 1.5 : - Removed the delay introduced in 1.4.      */
/*                             - If HSCAENETComm fails, we retry 10 times  */
/*                               instead of 2                              */
/*                             - Delivered as testing release (only for    */
/*                               Win32) to NA60 (Nicolas Guettet)          */
/*   July     2003: Rel. 1.6 : - The delay between retries is now 10 msec, */
/*                               instead of 50                             */
/*      								   */
/*   February 2007: Rel. 1.7 : - No changes, add only 64bit version        */
/*									   */
/***************************************************************************/

#define HSCAENETLIB

#include <stdio.h>
#include <string.h>
#include "HSCAENETLib.h"

#define   MAX_NR_OF_CARDS     10

/* Rel. 1.6 - The following define was used in HSCAENETSendCommand and HSCAENETComm
            and it was 50 msec.; now we use it only in HSCAENETComm and it is 10 msec.*/
#define   DELAY_TIME          10			/* Rel. 1.4 - milliseconds */

#ifdef WIN32

/*
// Cla - Presa da wdm.h del ddk
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
*/

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define FILE_ANY_ACCESS                 0
#define METHOD_BUFFERED                 0

#define FILE_DEVICE_PLX                         0x00008300   // Legacy definition

#define IOCTL_MSG( code )                       CTL_CODE(             \
                                                    FILE_DEVICE_PLX,  \
                                                    code,             \
                                                    METHOD_BUFFERED,  \
                                                    FILE_ANY_ACCESS   \
                                                    )

#define A1303_IOCTL_WRITE     IOCTL_MSG( 0x900 )
#define A1303_IOCTL_READ      IOCTL_MSG( 0x901 )
#define A1303_IOCTL_RESET     IOCTL_MSG( 0x902 )
#define A1303_IOCTL_TIMEOUT   IOCTL_MSG( 0x903 )


typedef __declspec(dllimport) int (* A303SENDCOMMAND)(int, int, unsigned char *, int);
typedef __declspec(dllimport) int (* A303READRESPONSE)(unsigned char *, int *);
typedef __declspec(dllimport) int (* A303RESET)(void);
typedef __declspec(dllimport) int (* A303INIT)(unsigned long);
typedef __declspec(dllimport) int (* A303TIMEOUT)(unsigned long);

static HINSTANCE		    hA303DLL;                        // Handle to A303lib

static A303SENDCOMMAND	    		A303SendCommand  = NULL;
static A303READRESPONSE	    		A303ReadResponse = NULL;
static A303RESET		    	A303Reset        = NULL;
static A303INIT			    	A303Init         = NULL; 
static A303TIMEOUT		    	A303Timeout      = NULL;

static HANDLE               hndFile[MAX_NR_OF_CARDS];  // Handle got from CreateFile

static CRITICAL_SECTION     sem[MAX_NR_OF_CARDS];

//
//+---------------------------------------------------------------+
//|  DLLMain                                                      |
//|  Used to initialize static data                               |
//|                                                               |
//+---------------------------------------------------------------+
//
BOOL WINAPI DllMain( HANDLE hModule, DWORD ul_reason_for_call, 
                     LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				int i;

				for( i = 0 ; i < MAX_NR_OF_CARDS ; i++ )
					hndFile[i] = INVALID_HANDLE_VALUE;
			}
			break;

		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }
    return TRUE;
}

#else  /* WIN32 */

#include <sys/time.h>				/* Rel. 1.4 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "a303.h"

typedef unsigned long 	ULONG;
typedef char 		CHAR;
typedef unsigned char 	UCHAR;
typedef unsigned char * PUCHAR;
typedef unsigned int	BOOL;

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
#endif /* DARWIN */
#define A1303_MAGIC			'3'

#define A1303_IOCTL_TIMEOUT		_IOW(A1303_MAGIC, 0, unsigned long)
/*#define A1303_IOCTL_TIMEOUT             _IO(A1303_MAGIC, 0) */
#define A1303_IOCTL_RESET		_IO(A1303_MAGIC, 1)
#define A1303_IOCTL_LED			_IO(A1303_MAGIC, 2)

#define INVALID_HANDLE_VALUE 	-1

static int		hndFile[MAX_NR_OF_CARDS];  /* Handle got from CreateFile*/

static int 		sem[MAX_NR_OF_CARDS];


#ifdef UNIX
/*
//+---------------------------------------------------------------+
//|  Sleep                                                        |
//|  Rel. 1.4 				                          |
//|                                                               |
//+---------------------------------------------------------------+

static void Sleep(unsigned long x)
{
	struct timeval t;

	t.tv_sec = 0;
	t.tv_usec = (x)*1000;
	select(0, NULL, NULL, NULL, &t);
} */
#endif

/*
//+---------------------------------------------------------------+
//|  _init                                                        |
//|  Used to initialize static data                               |
//|                                                               |
//+---------------------------------------------------------------+

void _init( void )
{
	int i;
	
	for( i = 0 ; i < MAX_NR_OF_CARDS ; i++ )
		hndFile[i] = INVALID_HANDLE_VALUE;
}


//+---------------------------------------------------------------+
//|  _fini                                                        |
//|                                                               |
//+---------------------------------------------------------------+

void _fini( void )
{	
}
*/
#endif /* WIN32 */

/*
//+---------------------------------------------------------------+
//|  createsem                                                    |
//|                                                               |
//+---------------------------------------------------------------+
*/
static void createsem(int dev)
{
#ifdef UNIX
	union semun semopts;
	key_t key;

	/* Create unique key via call to ftok() */
	key = ftok(".", '0'+dev);

	sem[dev] = semget(key, 1, IPC_CREAT|IPC_EXCL|0666);
	
	if( sem[dev] < 0 ) {
		fprintf(stderr, "Semaphore already present\r\n There is ");
		fprintf(stderr, "another process using the device.\r\n O");
		fprintf(stderr, "r a process using the device exited a");
		fprintf(stderr, "bnormally.\r\n In That case try to manu");
		fprintf(stderr, "ally release the semaphore with:\r\n   ");
		fprintf(stderr, "ipcrm sem XXX.\r\n");
		exit(1);
	}
	
	semopts.val = 1;
	
	semctl(sem[dev], 0, SETVAL, semopts);
#else
	InitializeCriticalSection(&sem[dev]);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  locksem                                                      |
//|                                                               |
//+---------------------------------------------------------------+
*/
static void locksem(int dev)
{
#ifdef UNIX
	struct sembuf sem_lock={ 0, -1, 0};
		
	semop(sem[dev], &sem_lock, 1);
#else
	EnterCriticalSection(&sem[dev]);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  unlocksem                                                    |
//|                                                               |
//+---------------------------------------------------------------+
*/
static void unlocksem(int dev)
{
#ifdef UNIX
	struct sembuf sem_unlock={ 0, 1, 0};

	semop(sem[dev], &sem_unlock, 1);
#else
	LeaveCriticalSection(&sem[dev]);	
#endif
}

/*
//+---------------------------------------------------------------+
//|  removesem                                                    |
//|                                                               |
//+---------------------------------------------------------------+
*/
static void removesem(int dev)
{
#ifdef UNIX
	semctl(sem[dev], 0, IPC_RMID, 0);
#else
	DeleteCriticalSection(&sem[dev]);
#endif
}

/*
//+---------------------------------------------------------------+
//|  HSCAENETLibSwRel                                             |
//|                                                               |
//|                                                               |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API char *HSCAENETLibSwRel(void)
{
	return "1.7";
}

#ifdef WIN32

/*
//+---------------------------------------------------------------+
//|  HSCAENETCardInit                                             |
//|  We support A303, A303A through A303lib and A1303 directly    |
//|  We return -1 if any problem occurs, 0 if connected to A303,  |
//|  >= 1 if connected to A1303                                   |
//|                                                               |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API int HSCAENETCardInit(const char *CardName, const void *param)
{
    SetLastError(0);
	
	if( !strcmp(CardName, "A303") || !strcmp(CardName, "A303A") )
	{
		hA303DLL = LoadLibrary("a303lib.dll");

		if (hA303DLL != NULL)
		{
			A303SendCommand  = (A303SENDCOMMAND)GetProcAddress(hA303DLL, "A303SendCommand");
			A303ReadResponse = (A303READRESPONSE)GetProcAddress(hA303DLL, "A303ReadResponse");
			A303Reset = (A303RESET)GetProcAddress(hA303DLL, "A303Reset");
			A303Init = (A303INIT)GetProcAddress(hA303DLL, "A303Init");
			A303Timeout = (A303TIMEOUT)GetProcAddress(hA303DLL, "A303Timeout");

			if( A303SendCommand  == NULL || 
				A303ReadResponse == NULL || 
				A303Reset        == NULL ||
				A303Init         == NULL || 
				A303Timeout      == NULL )
			{
				FreeLibrary(hA303DLL);       
				return -1;
			}
			else
			{
				int           err;
				unsigned long addr = *(unsigned long *)param;

				createsem(0);
				err = A303Init(addr);

				if( err != TUTTOK )
				{
					removesem(0);
					return -1;
				}
				else
					return 0;
			}
		}
		else
			return -1;
	}
	else if( !strcmp(CardName, "A1303") )
	{
		HANDLE hnd;
		char   devname[80];
		int    dev = *(int *)param;

		if( dev < 0 || dev >= MAX_NR_OF_CARDS )
			return -1;

		sprintf(devname,"\\\\.\\A1303-%d",dev);
	    hnd = CreateFile(devname, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
          
		if( hnd == INVALID_HANDLE_VALUE )      
			return -1;
		else
		{
			dev++;
			hndFile[dev] = hnd;
			createsem(dev);
			return dev;
		}
	}
	else
		return -1;
}

#else

/*
//+---------------------------------------------------------------+
//|  HSCAENETCardInit                                             |
//|  We support A303, A303A through A303lib and A1303 directly    |
//|  We return -1 if any problem occurs, 0 if connected to A303,  |
//|  >= 1 if connected to A1303                                   |
//|                                                               |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API int HSCAENETCardInit(const char *CardName, const void *param)
{
	
	if( !strcmp(CardName, "A303") || !strcmp(CardName, "A303A") )
	{
		int           err;
		unsigned long addr = *(unsigned long *)param;
		
		createsem(0);
		err = A303Init(addr);
		
		if( err != TUTTOK )
		{
			removesem(0);
			return -1;
		}
		else
			return 0;
	}
	else if( !strcmp(CardName, "A1303") )
	{
		int		hnd;
		char	devname[80];
		int		dev = *(int *)param;

		if( dev < 0 || dev >= MAX_NR_OF_CARDS )
			return -1;

		sprintf(devname, "/dev/a1303_%d", dev);          
		if( (hnd = open(devname, O_RDWR)) == -1 )      
			return -1;
		else
		{
			dev++;
			hndFile[dev] = hnd;
			createsem(dev);
			return dev;
		}
	}
	else
		return -1;
}

#endif

/*
//+---------------------------------------------------------------+
//|  HSCAENETSendCommand                                          |
//|                                                               |
//|  This routine takes the code of the operation, the caenet     |
//|  crate number of the slave, the data to send to the slave and |
//|  the number of bytes to send. It return 0 on success or an    |
//|  error code.                                                  |
//|  A further parameter is dev, i.e. 0 if A303, > 0 if A1303     |
//|                                                               |
//|  Rel. 1.3 - Linux semaphores compatibilty with windows        |
//|             critical section.                                 |
//|  Rel. 1.4 - Added the Sleep in order to avoid the timeout     |
//+---------------------------------------------------------------+
*/
static int __HSCAENETSendCommand(int dev, int code, int crnum, 
					  const void *buf, int count, int flag)
                                 
{
	int res;

/*  Rel. 1.5 - put the comment in the following; when used here, 
//	DELAY_TIME was 50 msec.
*/	Sleep(DELAY_TIME);	/* Rel. 1.4*/

	if( dev == 0 )				/* A303 */
	{
#ifdef WIN32
		if( A303SendCommand == NULL )
			res = E_NO_DEVICE;
		else
#endif
		{
			if( flag ) locksem(dev);
			res = A303SendCommand(code, crnum, (char *)buf, count);
			if( flag ) unlocksem(dev);
		}
	}
	else
	{
		if(    dev < 0 
			|| dev >= MAX_NR_OF_CARDS 
			|| hndFile[dev] == INVALID_HANDLE_VALUE  
		  )
			res = E_NO_DEVICE;
		else
		{
			int		  i;
			ULONG	  nb;
			PUCHAR	  locbuf, pbuf;
#ifdef WIN32
			BOOL      IoctlResult;
			UCHAR     resp[4];
#endif
			if( flag ) locksem(dev);

			locbuf = (UCHAR *)malloc(count + 6);

			locbuf[0] = 1;						          /* Master Id.			    Low Byte */
			locbuf[1] = 0;						          /* Master Id.			    High Byte */
			locbuf[2] = (char)(crnum & 0xff);	/* Slave Crate Number	    Low Byte */
			locbuf[3] = (char)((crnum & 0xff00) >> 8);  /* Slave Crate Number	    High Byte */
			locbuf[4] = (char)(code & 0xff);			  /* Caenet Code			Low Byte */
			locbuf[5] = (char)((code & 0xff00) >> 8);	  /* Caenet Code			High Byte */

			pbuf = (char *)buf;
			for( i = 0; i < count; i++ )				  /* Fill the buffer */
				locbuf[i + 6] = pbuf[i];

			/* Communicate to the device */
#ifdef WIN32
			SetLastError(0);
			IoctlResult = DeviceIoControl(
	                         hndFile[dev],            // Handle to device
	                         A1303_IOCTL_WRITE,       // WRITE command
	                         locbuf,			      // Buffer to driver.
	                         count + 6,			      // Length of buffer in bytes.
	                         resp,				
	                         4,					
	                         &nb,			          
	                         NULL				      // NULL means wait till op. completes.
	                         );

			free(locbuf);

			if( !IoctlResult) 
				res = E_OS;
			else 
				if( nb == 0 ) 
					res = TUTTOK;
				else 
					res = ( resp[2] + (resp[3]<<8) );
#else
			nb = write(hndFile[dev], locbuf, count + 6);

			res = ( nb<0 ? E_OS : TUTTOK );
			free(locbuf);
#endif

			if( flag ) unlocksem(dev);
		}
	}

  return res;
}

HSCAENETLIB_API int HSCAENETSendCommand(int dev, int code, int crnum, 
					const void *buf,   int count)
{
	return __HSCAENETSendCommand(dev, code, crnum, buf, count, -1);
}

/*
//+---------------------------------------------------------------+
//|  HSCAENETReadResponse                                         |
//|                                                               |
//|  This routine takes no input parameters, fill the buffer      |
//|  provided with the bytes read, nd set the value e slave and   |
//|  the number of bytes to send. It return 0 on success or an    |
//|  error code.                                                  |
//|  A further parameter is dev, i.e. 0 if A303, > 0 if A1303     |
//|                                                               |
//|  Rel. 1.3 - Linux semaphores compatibilty with windows        |
//|             critical section.                                 |
//+---------------------------------------------------------------+
*/
static int __HSCAENETReadResponse(int dev, void *buf, int *count, 
					   int flag)
{
	int		res;

	if( dev == 0 )						
	{
#ifdef WIN32
		if( A303ReadResponse == NULL )
			res = E_NO_DEVICE;
		else
#endif
		{
			if( flag ) locksem(dev);
			res = A303ReadResponse(buf, count);
			if( flag ) unlocksem(dev);
		}
	}
	else
	{
		if(    dev < 0 
			|| dev >= MAX_NR_OF_CARDS 
			|| hndFile[dev] == INVALID_HANDLE_VALUE  
		  )
			res = E_NO_DEVICE;
		else
		{
			ULONG	nb, i;
			BOOL    IoctlResult;
			CHAR	locbuf[MAX_LENGTH_FIFO];

			if( flag ) locksem(dev);

#ifdef WIN32
			SetLastError(0);
			IoctlResult = DeviceIoControl(
	                          hndFile[dev],           
	                            A1303_IOCTL_READ,
	                            NULL,				
	                            0,					
	                            locbuf,					// Buffer from driver.
	                            MAX_LENGTH_FIFO,
	                            &nb,					// Bytes placed in buf.
	                            NULL					// NULL means wait till op. completes.
	                            );
#else
			{
				int n;
				
				n = read(hndFile[dev], locbuf, MAX_LENGTH_FIFO);
				IoctlResult = (n >= 0);
				nb = n;
			}
#endif
			if( IoctlResult ) 
			{
				if( nb < 4 )
				{
					*count = 0;
					res = E_LESS_DATA;
					goto EndProc;
				}
	
			
	
				res = locbuf[2] + (locbuf[3]<<8);
				if( buf )		
				{
					PUCHAR  pbuf;
	
					pbuf = buf;
					for( i = 0; i < nb - 4; i++ )	
						pbuf[i] = locbuf[i + 4];
				}

				*count = nb - 4;	

				goto EndProc;
			}
			else
				res = E_OS;

EndProc:
			if( flag ) unlocksem(dev);
		}
	}

	return res;
}

HSCAENETLIB_API int HSCAENETReadResponse(int dev, void *buf, int *count)
{
	return __HSCAENETReadResponse(dev, buf, count, -1);
}

/*
//+---------------------------------------------------------------+
//|  HSCAENETCardReset                                            |
//|                                                               |
//|                                                               |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API int HSCAENETCardReset(int dev)
{
	int   res;

	if( dev == 0 )						
	{
#ifdef WIN32		
		if( A303Reset == NULL )
			res = E_NO_DEVICE;
		else
#endif
		{
			locksem(dev);
			res = A303Reset();
			unlocksem(dev);
		}
	}
	else
	{
		if(    dev < 0 
			|| dev >= MAX_NR_OF_CARDS 
			|| hndFile[dev] == INVALID_HANDLE_VALUE  
		  )
			res = E_NO_DEVICE;
		else
		{
#ifdef WIN32 
			ULONG	nb;
			BOOL  IoctlResult;
			UCHAR resp[4];
#endif
			locksem(dev);

#ifdef WIN32
			SetLastError(0);
			IoctlResult = DeviceIoControl(
	                            hndFile[dev],           // Handle to device
	                            A1303_IOCTL_RESET,		// Reset command
	                            NULL,				
	                            0,					
	                            resp,					// Buffer from driver.
	                            4,						// Length of buffer in bytes.
	                            &nb,					// Bytes placed in buf.
	                            NULL					// NULL means wait till op. completes.
	                            );
			if( !IoctlResult) 
				res = E_OS;
			else 
				if( nb == 0 ) 
					res = TUTTOK;
			else 
				res = ( resp[2] + (resp[3]<<8) );
#else
			if( ioctl(hndFile[dev], A1303_IOCTL_RESET, 0) == -1 )
			    res = E_OS;
			else 	
			    res = TUTTOK;
#endif

			unlocksem(dev);
		}
	}

	return res;
}

/*
//+---------------------------------------------------------------+
//|  HSCAENETTimeout                                              |
//|                                                               |
//|                                                               |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API int HSCAENETTimeout(int dev, ULONG Timeout)
{
	int   res;

	if( dev == 0 )					
	{
#ifdef WIN32
		if( A303Timeout == NULL )
			res = E_NO_DEVICE;
		else
#endif
		{
			locksem(dev);
			res = A303Timeout(Timeout);
			unlocksem(dev);
		}
	}
	else
	{
		if(    dev < 0 
			|| dev >= MAX_NR_OF_CARDS 
			|| hndFile[dev] == INVALID_HANDLE_VALUE  
		  )
			res = E_NO_DEVICE;
		else
		{
#ifdef WIN32
			ULONG nb;
#endif
			BOOL  IoctlResult;

			locksem(dev);

#ifdef WIN32
			SetLastError(0);
			IoctlResult = DeviceIoControl(
	                            hndFile[dev],           // Handle to device
	                            A1303_IOCTL_TIMEOUT,		// Timeout command
	                            &Timeout,				
	                            sizeof(ULONG),					
	                            NULL,
	                            0,
	                            &nb,					// Bytes placed in buf.
	                            NULL					// NULL means wait till op. completes.
	                            );
#else
		
			IoctlResult = !ioctl(hndFile[dev], A1303_IOCTL_TIMEOUT, Timeout);
#endif
			if( !IoctlResult) 
				res = E_OS;
			else
				res = TUTTOK;

			unlocksem(dev);
		}
	}

	return res;
}

/*
//+---------------------------------------------------------------+
//|  HSCAENETComm                                                 |
//|                                                               |
//|  Rel. 1.3 - Linux semaphores compatibilty with windows        |
//|             critical section.                                 |
//|  Rel. 1.4 - Modified in order to retry the communications if  |
//|             a CAENET error occurs                             |
//|  Rel. 1.5 - The number of retries passes from 2 to 10         |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API int HSCAENETComm(int dev, int Code, int CrateNum, 
								 const void *SourceBuff, int WriteByteCount, 
								 void *DestBuff)
{
	int i, res, dummy;

/* Rel. 1.1 - Added the test on dev != 0 */
	if(    dev < 0 
   		|| dev >= MAX_NR_OF_CARDS 
		|| (hndFile[dev] == INVALID_HANDLE_VALUE && dev != 0)
	  )
		return E_NO_DEVICE;


/*Rel. 1.4 - Added the loop
// Rel. 1.5 - from 2 to 10 loops
*/
	for( i = 0 ; i < 10 ; i++ )
	{
		locksem(dev);

		res = __HSCAENETSendCommand( dev, Code, CrateNum, SourceBuff, 
					     WriteByteCount, 0 );
		if( res == TUTTOK )
			res = __HSCAENETReadResponse( dev, DestBuff, &dummy, 0 );

		if( res == TUTTOK )
		{
			unlocksem(dev);
			break;
		}

		unlocksem(dev);

		Sleep(DELAY_TIME);		
	}

	return res;
}

/*
//+---------------------------------------------------------------+
//|  HSCAENETCardEnd                                              |
//|  Rel. 1.1 - Added the test on dev != 0.                       |
//|  Rel. 1.3 - removesem before everything else.                 |
//|             Added the A303End call.                           |
//+---------------------------------------------------------------+
*/
HSCAENETLIB_API int HSCAENETCardEnd(int dev)
{
	int res = TUTTOK;

	removesem(dev);

	if( dev == 0 ) {
#ifdef UNIX
		A303End();
#endif
		res = TUTTOK;
		goto End_CardEnd;
	}

	if(    dev < 0 
   		|| dev >= MAX_NR_OF_CARDS 
		|| hndFile[dev] == INVALID_HANDLE_VALUE 
	  )
		return E_NO_DEVICE;

#ifdef WIN32
	CloseHandle(hndFile[dev]);
#else
	close(hndFile[dev]);
#endif

End_CardEnd:
	hndFile[dev] = INVALID_HANDLE_VALUE;

	return res;
}
