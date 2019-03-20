/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    N470INTERFACE.H                                                      */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    May      2002    : Rel. 2.2 - Created                                */
/*                                                                         */
/***************************************************************************/

#ifndef __N470INTERFACE_H
#define __N470INTERFACE_H

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

int           N470Init(int devHandle, int crateNum);  
int           N470AInit(int devHandle, int crateNum);  
int           N570Init(int devHandle, int crateNum);  
void		  N470End(int devHandle, int crateNum);
char          *N470GetError(int devHandle, int crateNum);
CAENHVRESULT  N470GetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME]);

CAENHVRESULT  N470SetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName);

CAENHVRESULT  N470GetChParamInfo(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  char **ParNameList);

CAENHVRESULT  N470GetChParamProp(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  N470GetChParam(int devHandle, int crateNum, ushort slot, 
							  const char *ParName, ushort ChNum, const ushort *ChList, 
							  void *ParValList);

CAENHVRESULT  N470SetChParam(int devHandle, int crateNum, ushort slot, const char *ParName, 
						      ushort ChNum, const ushort *ChList, void *ParValue);

CAENHVRESULT  N470TestBdPresence(int devHandle, int crateNum, ushort slot, char *Model, 
								  char *Description, ushort *SerNum, 
								  uchar *FmwRelMin, uchar *FmwRelMax);

CAENHVRESULT  N470GetBdParamInfo(int devHandle, int crateNum, ushort slot, 
								  char **ParNameList);

CAENHVRESULT  N470GetBdParamProp(int devHandle, int crateNum, ushort slot, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  N470GetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, const char *ParName, 
							  void *ParValList);

CAENHVRESULT  N470SetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, 
							  const char *ParName, void *ParValue);

CAENHVRESULT  N470GetSysComp(int devHandle, int crateNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList);

CAENHVRESULT  N470GetCrateMap(int devHandle, int crateNum, ushort *NrOfSlot, 
							   char **ModelList, char **DescriptionList, 
							   ushort **SerNumList, uchar **FmwRelMinList, 
							   uchar **FmwRelMaxList);

CAENHVRESULT  N470GetExecCommList(int devHandle, int crateNum, ushort *NumComm, 
								   char **CommNameList);

CAENHVRESULT  N470ExecComm(int devHandle, int crateNum, const char *CommName);

CAENHVRESULT  N470GetSysPropList(int devHandle, int crateNum, ushort *NumProp, 
								  char **PropNameList);

CAENHVRESULT  N470GetSysPropInfo(int devHandle, int crateNum, const char *PropName, 
								  unsigned *PropMode, unsigned *PropType);

CAENHVRESULT  N470GetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Result);

CAENHVRESULT  N470SetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __N470INTERFACE_H */
