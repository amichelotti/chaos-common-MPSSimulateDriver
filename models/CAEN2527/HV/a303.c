/*****************************************************************************/
/*                                                                           */
/*       --- CAEN Engineering Srl - Computing Systems Division ---           */
/*                                                                           */
/*   A303.c                                                                  */
/*                                                                           */
/*   January  2000 :   Created.                                              */
/*   February 2000 :   Corrected bug in the writing of the high byte of      */   
/*                     code/crnum.                                           */
/*   May      2002 :   Used in HSCAENETLib only for the Linux systems.       */
/*                                                                           */
/*****************************************************************************/

#include  <stdio.h>
#include  <string.h>
#include  <errno.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <sys/ioctl.h>
#include  <stdlib.h>
#include  <fcntl.h>
#include  <unistd.h>
#include  "a303drv.h"
#include  "a303.h"

#define   TUTTOK                 0
#define   E_WRONG_ARG            1
#define   E_NO_RESET             2
#define   E_ON_DRIVER            3
#define   E_TX_TIMEOUT           5
#define   E_OS                   6


static int dev_handle;

/************************************************************************/
/*                                                                      */
/* A303INIT                                                             */
/*                                                                      */
/************************************************************************/
int A303Init(unsigned long Address)
{
int val = TUTTOK;

if( (dev_handle = open("/dev/A303",O_RDWR)) == -1 )  
  {
   val = E_ON_DRIVER;
   return val;
  }
   
if( ioctl(dev_handle,ADDRESS_A303,Address) == -1 ) 
  {
   val = E_WRONG_ARG;
   return val;
  }

return val;
}

/************************************************************************/
/*                                                                      */
/* A303RESET                                                            */
/* Executes an hardware reset of A303 module                            */
/*                                                                      */
/************************************************************************/
int A303Reset(void)
{
int val = TUTTOK;

if( ioctl(dev_handle,RESET_A303,0) == -1)
   val = E_NO_RESET;
return val;
}

/************************************************************************/
/*                                                                      */
/* A303TIMEOUT                                                          */
/* It is the time which the read function waits before than signalling  */
/* that the addressed slave module is not present.                      */
/* The following relation holds: timeout (in sec.) = time_out / 100     */
/*                                                                      */
/************************************************************************/
int A303Timeout(unsigned long Timeout)
{
int val = TUTTOK;

if( ioctl(dev_handle,TIMEOUT_A303,Timeout) == -1 )
   val = E_WRONG_ARG;
return val;
}

/************************************************************************/
/*                                                                      */
/* A303SENDCOMMAND                                                      */
/* Used to send a generic Caenet Command to a generic Caenet Slave      */
/*                                                                      */
/* February 2000 - Corrected bug in the writing of the high byte of     */ 
/*                 code/crnum                                           */
/*                                                                      */
/************************************************************************/
int A303SendCommand(int code, int crnum, void *source_buff, int count)
{
int            i, numb_byte;
unsigned char  *local_buff;

/* 
  CAENET Protocol requires: Master Identifier, Slave Crate Number and code
*/
local_buff = (unsigned char *)malloc(count + 6);

local_buff[0] = 1;                             /* Master Id.      Low  Byte */
local_buff[1] = 0;                             /* Master Id.      High Byte */
local_buff[2] = (char)(crnum & 0xff);          /* Slave Cr. Num.  Low  Byte */
local_buff[3] = (char)((crnum & 0xff00) >> 8); /* Slave Cr. Num.  High Byte */
local_buff[4] = (char)(code & 0xff);           /* Caenet code     Low  Byte */
local_buff[5] = (char)((code & 0xff00) >> 8);  /* Caenet code     High Byte */

for( i = 0; i < count; i++ )
    local_buff[i + 6] = *((unsigned char *)source_buff+i);

numb_byte = write(dev_handle,local_buff,count+6);
free(local_buff);
if( numb_byte != count + 6 )                         /* Problems in writing */
   return E_TX_TIMEOUT;
else
   return TUTTOK;
}


/************************************************************************/
/*                                                                      */
/* A303READRESPONSE                                                     */
/* Used to read a response following a command sent by A303SendCommand  */
/*                                                                      */ 
/************************************************************************/
int A303ReadResponse(void *buff, int *count)
{
int   i, resp, numb_byte;
char  tmpbuff[MAX_LENGTH_FIFO];

if( (numb_byte = read(dev_handle,tmpbuff,MAX_LENGTH_FIFO)) < 0 )
    return E_OS;
  
/* Discard first two bytes: echo of Master Ident. sent by A303SendCommand */
/* Isolate following two bytes which contain Slave Response               */
resp = tmpbuff[2] + (tmpbuff[3]<<8);

for( i = 0; i < numb_byte - 4 ; i++ )       
    *((char *)buff+i) = tmpbuff[i+4];      

*count = numb_byte - 4;

return resp;
}

/************************************************************************/
/*                                                                      */
/* A303END                                                              */
/* Dummy function in Linux                                              */
/*                                                                      */ 
/************************************************************************/
int A303End(void)
{
	close(dev_handle);
	return 0;
}

/************************************************************************/
/*                                                                      */
/* A303DECODERESP                                                       */
/*                                                                      */ 
/************************************************************************/
char  *A303DecodeResp(int resp)
{
static char buf[80];

if( resp == E_OS )
   sprintf(buf,"Operating System Error: %s", strerror(errno));
else
   switch( resp )
    {
      case E_WRONG_ARG:
        sprintf(buf,"Driver Error: Operation not permitted");
        break;
      case E_NO_RESET:
        sprintf(buf,"CaeNet Error: Timeout on reset");
        break;
      case E_ON_DRIVER:
        sprintf(buf,"Driver Error: %s", strerror(errno));
        break;
      case E_TX_TIMEOUT:
        sprintf(buf,"CaeNet Error: transmit timeout");
        break;
      default:
        sprintf(buf," Error %d received",resp);
        break;
    }
  
return buf;     
}
