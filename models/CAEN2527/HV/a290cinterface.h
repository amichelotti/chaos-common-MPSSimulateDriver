/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    A290CINTERFACE.H								               	       */
/*                                                                         */
/*    This file is part of the CAEN HV Wrapper Library                     */
/*                                                                         */
/*    Source code written in ANSI C                                        */
/*                                                                         */
/*    May 2004         : Created Rel. 2.12                                 */
/*                                                                         */
/***************************************************************************/

#ifndef __A290CINTERFACE_H
#define __A290CINTERFACE_H

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

int           A290CInit(int devHandle, int crateNum);  
void		  A290CEnd(int devHandle, int crateNum);
char          *A290CGetError(int devHandle, int crateNum);
CAENHVRESULT  A290CGetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, 
							 char (*ChNameList)[MAX_CH_NAME]);

CAENHVRESULT  A290CSetChName(int devHandle, int crateNum, ushort slot, ushort ChNum, 
							 const ushort *ChList, const char *ChName);

CAENHVRESULT  A290CGetChParamInfo(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  char **ParNameList);

CAENHVRESULT  A290CGetChParamProp(int devHandle, int crateNum, ushort slot, ushort Ch, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  A290CGetChParam(int devHandle, int crateNum, ushort slot, 
							  const char *ParName, ushort ChNum, const ushort *ChList, 
							  void *ParValList);

CAENHVRESULT  A290CSetChParam(int devHandle, int crateNum, ushort slot, const char *ParName, 
						      ushort ChNum, const ushort *ChList, void *ParValue);

CAENHVRESULT  A290CTestBdPresence(int devHandle, int crateNum, ushort slot, char *Model, 
								  char *Description, ushort *SerNum, 
								  uchar *FmwRelMin, uchar *FmwRelMax);

CAENHVRESULT  A290CGetBdParamInfo(int devHandle, int crateNum, ushort slot, 
								  char **ParNameList);

CAENHVRESULT  A290CGetBdParamProp(int devHandle, int crateNum, ushort slot, 
								  const char *ParName, const char *PropName, 
								  void *retval);

CAENHVRESULT  A290CGetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, const char *ParName, 
							  void *ParValList);

CAENHVRESULT  A290CSetBdParam(int devHandle, int crateNum, ushort slotNum, 
							  const ushort *slotList, 
							  const char *ParName, void *ParValue);

CAENHVRESULT  A290CGetSysComp(int devHandle, int crateNum, uchar *NrOfCr,
							  ushort **CrNrOfSlList, ulong **SlotChList);

CAENHVRESULT  A290CGetCrateMap(int devHandle, int crateNum, ushort *NrOfSlot, 
							   char **ModelList, char **DescriptionList, 
							   ushort **SerNumList, uchar **FmwRelMinList, 
							   uchar **FmwRelMaxList);

CAENHVRESULT  A290CGetExecCommList(int devHandle, int crateNum, ushort *NumComm, 
								   char **CommNameList);

CAENHVRESULT  A290CExecComm(int devHandle, int crateNum, const char *CommName);

CAENHVRESULT  A290CGetSysPropList(int devHandle, int crateNum, ushort *NumProp, 
								  char **PropNameList);

CAENHVRESULT  A290CGetSysPropInfo(int devHandle, int crateNum, const char *PropName, 
								  unsigned *PropMode, unsigned *PropType);

CAENHVRESULT  A290CGetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Result);

CAENHVRESULT  A290CSetSysProp(int devHandle, int crateNum, const char *PropName, 
							  void *Set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __A290CINTERFACE_H */
