/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY127INTERFACE.H								               	       */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    December 2001    : Created                                           */
/*    April    2002    : Rel. 2.1 - Added the devHandle param              */
/*                                                                         */
/***************************************************************************/

#ifndef __SY127INTERFACE_H
#define __SY127INTERFACE_H

#ifndef uchar 
#define uchar unsigned char
#endif
#ifndef ushort 
#define ushort unsigned short
#endif
#ifndef ulong
#define ulong unsigned long
#endif

#ifndef __CAENHVRESULT__                         /* Rel. 2.0 - Linux */
/* The Error Code type */
typedef int CAENHVRESULT;
#define __CAENHVRESULT__
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int           Sy127Init(int devHandle, int crateNum);  
void		  Sy127End(int devHandle, int crateNum);
char          *Sy127GetError(int devHandle, int crateNum);
CAENHVRESULT  Sy127GetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME]);

CAENHVRESULT  Sy127SetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName);

CAENHVRESULT  Sy127GetChParamInfo(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  char **ParNameList);

CAENHVRESULT  Sy127GetChParamProp(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  Sy127GetChParam(int devHandle, int crateNum, ushort slot, 
							  const char *ParName, ushort ChNum, const ushort *ChList, 
							  void *ParValList);

CAENHVRESULT  Sy127SetChParam(int devHandle, int crateNum, ushort slot, const char *ParName, 
						      ushort ChNum, const ushort *ChList, void *ParValue);

CAENHVRESULT  Sy127TestBdPresence(int devHandle, int crateNum, ushort slot, char *Model, 
								  char *Description, ushort *SerNum, 
								  uchar *FmwRelMin, uchar *FmwRelMax);

CAENHVRESULT  Sy127GetBdParamInfo(int devHandle, int crateNum, ushort slot, 
								  char **ParNameList);

CAENHVRESULT  Sy127GetBdParamProp(int devHandle, int crateNum, ushort slot, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  Sy127GetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, const char *ParName, 
							  void *ParValList);

CAENHVRESULT  Sy127SetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, 
							  const char *ParName, void *ParValue);

CAENHVRESULT  Sy127GetSysComp(int devHandle, int crateNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList);

CAENHVRESULT  Sy127GetCrateMap(int devHandle, int crateNum, ushort *NrOfSlot, 
							   char **ModelList, char **DescriptionList, 
							   ushort **SerNumList, uchar **FmwRelMinList, 
							   uchar **FmwRelMaxList);

CAENHVRESULT  Sy127GetExecCommList(int devHandle, int crateNum, ushort *NumComm, 
								   char **CommNameList);

CAENHVRESULT  Sy127ExecComm(int devHandle, int crateNum, const char *CommName);

CAENHVRESULT  Sy127GetSysPropList(int devHandle, int crateNum, ushort *NumProp, 
								  char **PropNameList);

CAENHVRESULT  Sy127GetSysPropInfo(int devHandle, int crateNum, const char *PropName, 
								  unsigned *PropMode, unsigned *PropType);

CAENHVRESULT  Sy127GetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Result);

CAENHVRESULT  Sy127SetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SY127INTERFACE_H */
