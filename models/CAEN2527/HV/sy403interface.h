/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    SY403INTERFACE.H					                   */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    April    2002    : Rel. 2.1 - Created                                */
/*                                                                         */
/***************************************************************************/

#ifndef __SY403INTERFACE_H
#define __SY403INTERFACE_H

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

int           Sy403Init(int devHandle, int crateNum);
void		  Sy403End(int devHandle, int crateNum);
char          *Sy403GetError(int devHandle, int crateNum);
CAENHVRESULT  Sy403GetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME]);

CAENHVRESULT  Sy403SetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName);

CAENHVRESULT  Sy403GetChParamInfo(int devHandle, int crateNum, ushort slot, 
								  ushort Ch, char **ParNameList);

CAENHVRESULT  Sy403GetChParamProp(int devHandle, int crateNum, ushort slot, ushort Ch,
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  Sy403GetChParam(int devHandle, int crateNum, ushort slot, 
							  const char *ParName, 
							  ushort ChNum, const ushort *ChList, void *ParValList);

CAENHVRESULT  Sy403SetChParam(int devHandle, int crateNum, ushort slot, 
							  const char *ParName, 
						      ushort ChNum, const ushort *ChList, void *ParValue);

CAENHVRESULT  Sy403TestBdPresence(int devHandle, int crateNum, ushort slot, 
								  char *Model, 
								  char *Description, ushort *SerNum, 
								  uchar *FmwRelMin, uchar *FmwRelMax);

CAENHVRESULT  Sy403GetBdParamInfo(int devHandle, int crateNum, ushort slot, 
								  char **ParNameList);

CAENHVRESULT  Sy403GetBdParamProp(int devHandle, int crateNum, ushort slot, 
								  const char *ParName, 
								  const char *PropName, void *retval);

CAENHVRESULT  Sy403GetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList,
							  const char *ParName, void *ParValList);

CAENHVRESULT  Sy403SetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, 
							  const char *ParName, void *ParValue);

CAENHVRESULT  Sy403GetSysComp(int devHandle, int crateNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList);

CAENHVRESULT  Sy403GetCrateMap(int devHandle, int crateNum, ushort *NrOfSlot, 
							   char **ModelList, 
							   char **DescriptionList, ushort **SerNumList, 
							   uchar **FmwRelMinList, uchar **FmwRelMaxList);

CAENHVRESULT  Sy403GetExecCommList(int devHandle, int crateNum, ushort *NumComm, 
								   char **CommNameList);

CAENHVRESULT  Sy403ExecComm(int devHandle, int crateNum, const char *CommName);

CAENHVRESULT  Sy403GetSysPropList(int devHandle, int crateNum, ushort *NumProp, 
								  char **PropNameList);

CAENHVRESULT  Sy403GetSysPropInfo(int devHandle, int crateNum, const char *PropName, 
								  unsigned *PropMode, unsigned *PropType);

CAENHVRESULT  Sy403GetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Result);

CAENHVRESULT  Sy403SetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SY403INTERFACE_H */