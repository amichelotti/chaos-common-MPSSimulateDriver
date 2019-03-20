/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    N568INTERFACE.H                                                      */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    September 2002    : Rel. 2.6 - Created                               */
/*                                                                         */
/***************************************************************************/

#ifndef __N568INTERFACE_H
#define __N568INTERFACE_H

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

int           N568Init(int devHandle, int crateNum);  
int           N568BInit(int devHandle, int crateNum);  
void		  N568End(int devHandle, int crateNum);
char          *N568GetError(int devHandle, int crateNum);
CAENHVRESULT  N568GetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME]);

CAENHVRESULT  N568SetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName);

CAENHVRESULT  N568GetChParamInfo(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  char **ParNameList);

CAENHVRESULT  N568GetChParamProp(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  N568GetChParam(int devHandle, int crateNum, ushort slot, 
							  const char *ParName, ushort ChNum, const ushort *ChList, 
							  void *ParValList);

CAENHVRESULT  N568SetChParam(int devHandle, int crateNum, ushort slot, const char *ParName, 
						      ushort ChNum, const ushort *ChList, void *ParValue);

CAENHVRESULT  N568TestBdPresence(int devHandle, int crateNum, ushort slot, char *Model, 
								  char *Description, ushort *SerNum, 
								  uchar *FmwRelMin, uchar *FmwRelMax);

CAENHVRESULT  N568GetBdParamInfo(int devHandle, int crateNum, ushort slot, 
								  char **ParNameList);

CAENHVRESULT  N568GetBdParamProp(int devHandle, int crateNum, ushort slot, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  N568GetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, const char *ParName, 
							  void *ParValList);

CAENHVRESULT  N568SetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, 
							  const char *ParName, void *ParValue);

CAENHVRESULT  N568GetSysComp(int devHandle, int crateNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList);

CAENHVRESULT  N568GetCrateMap(int devHandle, int crateNum, ushort *NrOfSlot, 
							   char **ModelList, char **DescriptionList, 
							   ushort **SerNumList, uchar **FmwRelMinList, 
							   uchar **FmwRelMaxList);

CAENHVRESULT  N568GetExecCommList(int devHandle, int crateNum, ushort *NumComm, 
								   char **CommNameList);

CAENHVRESULT  N568ExecComm(int devHandle, int crateNum, const char *CommName);

CAENHVRESULT  N568GetSysPropList(int devHandle, int crateNum, ushort *NumProp, 
								  char **PropNameList);

CAENHVRESULT  N568GetSysPropInfo(int devHandle, int crateNum, const char *PropName, 
								  unsigned *PropMode, unsigned *PropType);

CAENHVRESULT  N568GetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Result);

CAENHVRESULT  N568SetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __N568INTERFACE_H */
