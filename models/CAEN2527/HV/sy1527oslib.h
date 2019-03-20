/******************************************************************************/
/*                                                                            */
/*       --- CAEN Engineering Srl - Computing Division ---                    */
/*                                                                            */
/*   SY1527 Software Project                                                  */
/*                                                                            */
/*   SY1527OSLIB.H                                                            */
/* The following ifdef block is the standard way of creating macros which     */
/* make exporting from a DLL simpler. All files within this DLL are compiled  */
/* with the __SY1527OSLIB symbol defined on the command line. this symbol     */
/* should not be defined on any project that uses this DLL. This way any      */
/* other project whose source files include this file see SY1527LIB_API       */
/* functions as being imported from a DLL, whereas this DLL sees symbols      */
/* defined with this macro as being exported.                                 */
/*   Created: January 2000                                                    */
/*                                                                            */
/*   Auth: C.Raffo, A. Morachioli                                             */
/*                                                                            */
/*   Release: 1.0L                                                            */
/*                                                                            */
/*   May 2002: Modified for the multiplatform support in the same files.      */
/*                                                                            */
/******************************************************************************/
#ifndef __SY1527OSLIB
#define __SY1527OSLIB

#ifdef WIN32
// Rel. 2.2 - Eliminated the export of global functions in SY1527 Interface
//#define SY1527LIB_API __declspec(dllexport)

#include <windows.h>
//#include <winsock2.h>

#endif /* WIN32 */

#endif /* __SY1527OSLIB */
