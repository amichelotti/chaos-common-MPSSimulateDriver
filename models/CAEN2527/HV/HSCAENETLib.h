/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    HSCAENETLib.h                                         			   */
/*                                                                         */
/*    This is the include file for a library for Win32 (DLL) or for Linux. */
/*    Its functions permit any kind of CAENET operation directly through   */
/*    A1303 model or, indirectly by calling A303lib functions, through     */
/*    A303/A model.										                   */
/*                                                                         */
/*    Created: March 2002                                                  */
/*                                                                         */
/***************************************************************************/

#ifndef __HSCAENETLIB_H
#define __HSCAENETLIB_H

#include <stdio.h>
#include <stdlib.h>
#if defined(DARWIN)
#include <sys/malloc.h>
#endif
#if !defined(DARWIN)
#include <malloc.h>
#endif
#include "hscaenetoslib.h"

#define TUTTOK              0
#define E_OS                1
#define E_LESS_DATA         2
#define E_TIMEOUT           3
#define E_NO_DEVICE         4
#define E_A303_BUSY         5

#define MAX_LENGTH_FIFO	    4096

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

HSCAENETLIB_API char *HSCAENETLibSwRel(void);
HSCAENETLIB_API int  HSCAENETCardInit(const char *CardName, const void *param);
HSCAENETLIB_API int  HSCAENETTimeout(int device, unsigned long Timeout);
HSCAENETLIB_API int  HSCAENETCardReset(int device);
HSCAENETLIB_API int  HSCAENETSendCommand (int device, int Code, int CrateNumber, 
					               const void *SourceBuff, int WriteByteCount);
HSCAENETLIB_API int  HSCAENETReadResponse (int device, void *DestBuff, int *ReadByteCount);
HSCAENETLIB_API int  HSCAENETComm (int device, int Code, int CrateNumber,
			    	 			   const void *SourceBuff, int WriteByteCount, void *DestBuff);

HSCAENETLIB_API int  HSCAENETCardEnd(int device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __HSCAENETLIB_H */
