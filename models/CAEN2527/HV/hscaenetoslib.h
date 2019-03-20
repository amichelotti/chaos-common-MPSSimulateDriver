/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    HSCAENETOSLIB.H                                         	     	   */
/*                                                                         */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */ 
/*                                                                         */ 
/*    Created: March 2002                                                  */
/*                                                                         */
/*    May 2002 - Support for the Unix systems.                             */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#ifndef __HSCAENETOSLIB_H
#define __HSCAENETOSLIB_H

#ifdef WIN32

#include <windows.h>
#include <winioctl.h>

#ifdef HSCAENETLIB
#define HSCAENETLIB_API __declspec(dllexport) 

#else

#define HSCAENETLIB_API 

#endif

#else  /* WIN32 */ 

#define HSCAENETLIB_API

#endif  /* WIN32 */

#endif  /* HSCAENETOSLIB_H */
