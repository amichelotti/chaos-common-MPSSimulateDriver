/******************************************************************************/
/*                                                                            */
/*       --- CAEN Engineering Srl - Computing Division ---                    */
/*                                                                            */
/*   SY1527 Software Project                                                  */
/*                                                                            */
/*   SY1527INTERFACE.H                                                        */
/*   This file contains the public interface to the functions of C library    */
/*                                                                            */
/*   Created: January 2000                                                    */
/*                                                                            */
/*   Auth: E. Zanetti, A. Morachioli                                          */
/*                                                                            */
/*   Release: 1.0L                                                            */
/*                                                                            */
/*  Modifiche                                                                 */
/*   Release 2.00L: it's available a new function for execute, by SY1527,     */
/*                 CAENET command: Sy1527CaenetComm                           */
/*   Release 2.01L:                                                           */
/*                                                                            */
/******************************************************************************/
#ifndef __SY1527INTERFACE
#define __SY1527INTERFACE
 
#include "sy1527oslib.h"
#include "CAENHVWrapper.h" /* Rel 2.01L */
#include "libclient.h"     /* Ver. 3.00 */


/* generic definitions */
/* #define SY1527_MAX_LIB_ERROR   0x1000 V. 2.01L move in CAENHVWrapper.h */

/* Rel 2.01L: messi in CAENHVWrapper.h

#define MAX_CH_NAME                12
 
#define MAX_PARAM_NAME             10

#define MAX_CRATES                             8
#define MAX_SLOTS                             32
#define MAX_BOARDS    ( MAX_SLOTS * MAX_CRATES )


#define MAX_CHANNEL_TYPES           1

#define MAX_BOARD_NAME             12
#define MAX_BOARD_DESC             28 
#define SET                         1
#define MON                         0
#define SIGNED                      1
#define UNSIGNED                    0

#define PARAM_TYPE_NUMERIC          0
#define PARAM_TYPE_ONOFF            1
#define PARAM_TYPE_CHSTATUS         2
#define PARAM_TYPE_BDSTATUS         3

#define PARAM_MODE_RDONLY           0
#define PARAM_MODE_WRONLY           1
#define PARAM_MODE_RDWR             2

#define PARAM_UN_NONE               0
#define PARAM_UN_AMPERE             1
#define PARAM_UN_VOLT               2
#define PARAM_UN_WATT               3
#define PARAM_UN_CELSIUS            4
#define PARAM_UN_HERTZ              5
#define PARAM_UN_BAR                6
#define PARAM_UN_VPS                7
#define PARAM_UN_SECOND             8

#define SYSPROP_TYPE_STR            0
#define SYSPROP_TYPE_REAL           1
#define SYSPROP_TYPE_UINT2          2
#define SYSPROP_TYPE_UINT4          3
#define SYSPROP_TYPE_INT2           4
#define SYSPROP_TYPE_INT4           5
#define SYSPROP_TYPE_BOOLEAN        6

#define SYSPROP_MODE_RDONLY         0
#define SYSPROP_MODE_WRONLY         1
#define SYSPROP_MODE_RDWR           2

*/

/*-----------------------------------------------------------------------------
                                                                             
                             ERROR    CODES                                 
                                                                             
  They are positive, since they define errors from the library not from      
  server, with the exception of Login/Logout command which always return -1  
  in case of error.                                                          
  Their meaning is the next:                                                 
   CODES                                                                     
    -1    Error give back by Login only                               
     0    Command library correctly executed                                 
     1    Error of operatived system                                         
     2    Write error in communication channel                               
     3    Read error in communication channel                                
     4    Time out in server communication                                   
     5    Command Front End application is down                              
     6    Comunication with system not yet connected by a Login command      
     7    Execute Command not yet implementated                              
     8    Get Property not yet implementated                                 
     9    Set Property not yet implementated                                 
     10   Communication with RS232 not yet implementated                     
     11   User memory not sufficient                                          
	 12   Value out of range
     13   Property not yet implementated
     14   Property not found
     15   Execute command not found
     16   No System property
     17   No get property
     18   No set property
     19   No execute command
     20   SY1527 configuration change
     21   Property of param not found
     22   Param not found
 -----------------------------------------------------------------------------*/

#define SY1527_LOG_ERR             -1
#define SY1527_OK                   0
#define SY1527_SYSERR               1
#define SY1527_WRITEERR             2
#define SY1527_READERR              3
#define SY1527_TIMEERR              4
#define SY1527_DOWN                 5
#define SY1527_NOTPRES              6
#define SY1527_SLOTNOTPRES          7
#define SY1527_NOSERIAL             8
#define SY1527_MEMORYFAULT          9
#define SY1527_OUTOFRANGE           10
#define SY1527_EXECCOMNOTIMPL       11
#define SY1527_GETPROPNOTIMPL       12
#define SY1527_SETPROPNOTIMPL       13
#define SY1527_PROPNOTFOUND         14
#define SY1527_EXECNOTFOUND         15
#define SY1527_NOTSYSPROP			16
#define SY1527_NOTGETPROP			17
#define SY1527_NOTSETPROP           18
#define SY1527_NOTEXECOMM           19
#define SY1527_SYSCONFCHANGE	    20
#define SY1527_PARAMPROPNOTFOUND    21
#define SY1527_PARAMNOTFOUND        22

#define UDPMINPACKETLENGTH			0x1C

typedef int SY1527RESULT;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



                       /*** FUNCTION  PROTOTYPES ***/


                              /* UTILITIES */


/* SY1527GETERROR -------------------------------------------------------------
    Go to last error occurred to the system of index SysId. Which can be used
    for all the function except for Login and Logout
    Parameter In
     SysId
 -----------------------------------------------------------------------------*/
 char *Sy1527GetError(int SysId);

/* SY1527LOGINOUTERR ----------------------------------------------------------
    Function which returns the error with the last Login/Logout call
 -----------------------------------------------------------------------------*/
 char *Sy1527LogInOutErr(void);

/* SY1527LIBSWREL -------------------------------------------------------------
    Return the software release of the library as parameter out
    Parameter Out
     SoftwareRel    stringa null terminata della release software
 *-----------------------------------------------------------------------------*/
 int Sy1527LibSwRel(char *SoftwareRel);



              /* Wrappers for the Command FrontEnd commands */


                         /* SESSION   COMMANDS */

/* SY1527LOGIN ----------------------------------------------------------------
    It is the first function to be used to communicate with the required system.
    It returns the ID of the system (>=0), -1 if the error occured
    Parametri In:
     CommPath  string which identifies the communication channel chosen to
               communicate with the SY1527: "TCP/IP" or "RS232"
     Device    string containing, according to the chosen communication channel,
               the address TCP/IP of the system or the gate (COMM1/COMM2 ) of
               the RS232
     User      name (among the allowed ones) with which the user chooses to log
     Passwd    password associated to the name
 -----------------------------------------------------------------------------*/
 int Sy1527Login(const char *CommPath, const char *Device, 
                                          const char *User, const char *Passwd);

/* SY1527LOGOUT ---------------------------------------------------------------
   Function which allow to close the communication , previously opened by a
   Login, with sistem of ID SysId
   It return the error code
 -----------------------------------------------------------------------------*/
 int Sy1527Logout(int SysId);



     /* NOTE: ALL THE FUNCTIONS SPECIFIED BELOW RETURNS THE ERROR CODE */


                         /* CHANNEL   COMMANDS */

/* SY1527GETCHNAME ------------------------------------------------------------
    Command to receive a list of strings containg the names of the channels
    belonging to the board in slot Slot
    Parameter In
     SysId           Sistem ID
     Slot            It identifies the board in terms of crate (high byte) and
                     slot (low byte)
     ChNum           number of channels of which you want to know the name
     ChList          array of ChNUm channels
    Parameter Out
     ChNameList      array of ChNUm strings containing the name of the channels
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetChName(int SysId, unsigned short Slot,
                             unsigned short ChNum, const unsigned short *ChList,
                                               char (*ChNameList)[MAX_CH_NAME]);

/* SY1527SETCHNAME ------------------------------------------------------------
    Command to assign the "ChName" to the channels belonging to the board in slot
    Slot
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ChNum        number of channels
     ChName       name to give to the specified channels in the list
     ChList       array of ChNUm channels
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527SetChName(int SysId, unsigned short Slot,
         unsigned short ChNum, const unsigned short *ChList, const char *ChName);

/* SY1527GETCHPARAMINFO -------------------------------------------------------
    Command to know the  names of the tupes of parameters associated to each
    channel
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     Ch           channel of which you want to know the parameter list
    Parameter Out
     ParNameList  array of strings containing the name of the parameters
                  associated to that channel. The last element of the array
                  is the null string
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetChParamInfo(int SysId, unsigned short Slot,
                                         unsigned short Ch, char **ParNameList);

/* SY1527GETCHPARAMPROP -------------------------------------------------------
    Function to receive the value associated to the particular property of the
    parameter of the channel identified by slot and ch. This can be a numeric
    value, a string or a bit field
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     Ch           channel
     ParName      parameter name
     PropName     property name
    Parameter Out
     retval       value of the property of the parameter
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetChParamProp(int SysId, unsigned short Slot,
     unsigned short Ch, const char *ParName, const char *PropName, void *retval);

/* SY1527GETCHPARAM -----------------------------------------------------------
    Command to receive a list of the values of parameter "ParName" of the
    channels belonging to the board in slot Slot
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ChNum        number of channels
     ChList       array of ChNUm channels
     ParName      parameter name
    Parameter Out
     ParValList   list of ChNum elements containig the values of the parameter
                  specified
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetChParam(int SysId, unsigned short Slot,
       const char *ParName, unsigned short ChNum, const unsigned short *ChList, 
                                                              void *ParValList);

/* SY1527SETCHPARAM -----------------------------------------------------------
    Command to assign a new value to parameter "ParName" to the channels
    belonging of the board in slot Slot
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ChNum        number of channels
     ChList       array of ChNUm channels
     ParName      parameter name
     ParValue     parameter value
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527SetChParam(int SysId, unsigned short Slot,
       const char *ParName, unsigned short ChNum, const unsigned short *ChList,
                                                                void *ParValue);


                         /* BOARDS   COMMANDS */

/* SY1527TESTBDPRESENCE -------------------------------------------------------
    Command which verifies if the boards in slot Slot is present and, if it is, it
    provides some basic info
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
    Parameter Out
     Model        string containig the name of the board
     Description  string containig the description of the board
     SerialNum    Serial Number of Boad
     FmwRelMin    minor revision
     FmwRelMax    major revision
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527TestBdPresence(int SysId, unsigned short Slot,
                     char *Model, char *Description, unsigned short *SerialNum,
                            unsigned char *FmwRelMin, unsigned char *FmwRelMax);

/* SY1527GETBDPARAMINFO -------------------------------------------------------
    Function with which it is possible to know the names of the parameters of the
    board
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
    Parameter Out
     ParNameList  array of strings containing the name of the parameters of the
                  board. The last element of the array is the null string
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetBdParamInfo(int SysId, unsigned short Slot,
                                                            char **ParNameList);

/* SY1527BDPARAMPROP ----------------------------------------------------------
    Function to receive the value associated to the particular property of the
    parameter of the board. This can be a numeric value, a string or a bit field
    Parameter In
     SysId        Sistem ID
     Slot         It identifies the board in terms of crate (high byte) and
                  slot (low byte)
     ParName      parameter name
     PropName     property name
    Parameter Out
     retval       value of the property of the parameter
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetBdParamProp(int SysId, unsigned short Slot,
                        const char *ParName, const char *PropName, void *retval);

/* SY1527GETBDPARAM -----------------------------------------------------------
   Command to receive the value of parameter "ParName" of the board in slot Slot
   Parameter In
    SysId        Sistem ID
    NrOfSlot     number of slot
    SlotList     list of NrOfSlot Slot identifies in terms of crate (high byte)
                 and slot (low byte)
    ParName      parameter name
   Parameter Out
    ParValList   list of NrOfSlot parameter value
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetBdParam(int SysId, unsigned short NrOfSlot,
          const unsigned short *SlotList, const char *ParName, void *ParValList);

/* SY1527SETBDPARAM -----------------------------------------------------------
  Command to assign a new value to parameter "ParName" of the board in slot Slot
   Parameter In
    SysId        Sistem ID
    NrOfSlot     number of slot
    SlotList     list of NrOfSlot Slot identifies in terms of crate (high byte)
                 and slot (low byte)
    ParName      parameter name
    ParVal       parameter value
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527SetBdParam(int SysId, unsigned short NrOfSlot,
            const unsigned short *SlotList, const char *ParName, void *ParValue);


                          /* GROUP   COMMANDS */

/* SY1527GETGRPCOMP -----------------------------------------------------------
    Command to get the group composition in term of pairs (Slot, ChInSlot)
    Parameter In
     SysId        Sistem ID
     Group        group number
    Parameter Out
     NrOfCh       number of channels
     ChList       list of NrOfCh channels identified by Slot (two MSB: crate (high byte),
                  slot(low byte)) e ChInSlot (two LSB: channel)
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetGrpComp(int SysId, unsigned short Group,
                                unsigned short *NrOfCh, unsigned long **ChList);

/* SY1527ADDCHTOGRP -----------------------------------------------------------
    Command to add to the group the channels indicated in ChLIst
    Parameter In
     SysId        Sistem ID
     Group        group number
     NrOfCh       number of channels
     ChList       list of NrOfCh channels identified by Slot (two MSB: crate (high byte),
                  slot(low byte)) e ChInSlot (two LSB: channel)
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527AddChToGrp(int SysId, unsigned short Group,
                             unsigned short NrOfCh, const unsigned long *ChList);

/* SY1527REMCHTOGRP -----------------------------------------------------------
    Command to remove from the group the channels indicated in ChLIst
    Parameter In
     SysId        Sistem ID
     Group        group number
     NrOfCh       number of channels
     ChList       list of NrOfCh channels identified by Slot (two MSB: crate (high byte),
                  slot(low byte)) e ChInSlot (two LSB: channel)
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527RemChToGrp(int SysId, unsigned short Group,
                             unsigned short NrOfCh, const unsigned long *ChList);

/* SY1527GETGRPPARAM ----------------------------------------------------------
    Command to receive a list of long words containg the values of the
    parameters in ParNameList of the channels belonging to the group
    Parameter In
     SysId        Sistem ID
     Group        group number
     NrOfPar      number of parameters
     ParNameList  array of NrOfPar containing the name of the parameter
    Parameter Out
     ParValList   list of NrOfPar*NrOfChGrp values of the corresponding parameters
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetGrpParam(int SysId, unsigned short Group,
      unsigned short NrOfPar, const unsigned char (*ParNameList)[MAX_PARAM_NAME],
                                                               void *ParValList);

/* SY1527SETGRPPARAM ----------------------------------------------------------
    Command to assigne a new value to parameters "ParName" to the channels
    belonging to the group
    Parameter In
     SysId        Sistem ID
     Group        group number
     ParName      parameter name
     ParVal       paramenter value
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527SetGrpParam(int SysId, unsigned short Group,
                                     const unsigned char *ParName, void *ParVal);


                          /* SYSTEM   COMMANDS */

/* SY1527GETCRATEMAP ----------------------------------------------------------
    Command which verifies the Sy1527's boards population
    Parameter In
     SysId            Sistem ID
    Parameter Out
     NrOfSl           number of slot of system
     ModelList        list of Model or ""
     DescriptionList  list of description. Null if board not present
     SerNumList       list of serial number. 0 if board not present
     FmwRelMinList    list minor revision. 0 if board not present
     FmwRelMaxList    list of major revision. 0 if board not present
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetCrateMap(int SysId, unsigned short *NrOfSl,
         char **ModelList, char **DescriptionList, unsigned short **SerNumList,
                  unsigned char **FmwRelMinList, unsigned char **FmwRelMaxList);
                                                 
/* SY1527GETSYSCOMP -----------------------------------------------------------
   Command which gets info about the system connected in a cluster via LocalNet.
   (number, list of crate(crate,number of slot), number of channel for each 
   slot)
    Parameter In
     SysId          Sistem ID
    Parameter Out
     NrOfCr         number of slot of system
     CrNrOfSlList   list of crate and number of slot(Crate (MSB), nr. of Slot(LSB))
     SlotChList     list of Nr. of Ch. for each Slot. They are Nr of total Slot.
	                  Slot (two bytes (MSB)) and Nr. of Ch (two bytes (LSB))
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetSysComp(int SysId, unsigned char *NrOfCr,
                        unsigned short **CrNrOfSlList, unsigned long **SlotChList);
                  
                                                 

                         /* EXECUTE    COMMANDS */

/* SY1527GETEXECCOMMANDLIST ---------------------------------------------------
    It returns the commands which can be executed by SY1527ExecComm
    Parameter In
     SysId        Sistem ID
    Parameter Out
     NrOfComm     number of commands
     CommNameList list of NrOfComm name of the commands
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetExecCommList(int SysId,
                                 unsigned short *NrOfComm, char **CommNameList);

/* SY1527EXECCOMM -------------------------------------------------------------
    It execute the command specified in CommName
    Parameter In
     SysId       Sistem ID
     CommName    command name
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527ExecComm(int SysId, const char *CommName);


                         /* PROPERTY   COMMANDS */

/* SY1527GETSYSPROPLIST -------------------------------------------------------
    It returns the properties which can be executed by Sy1527Get/SetSysProp
    Parameter In
     SysId        Sistem ID
    Parameter Out
     NrOfProp     number of properties
     PropNameList list of NrOfProp name of the properties
 *----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetSysPropList(int SysId,
                                 unsigned short *NrOfProp, char **PropNameList);

/* SY1527GETSYSPROPINFO -------------------------------------------------------
    It returns for each properties of system, the type of variable (to set or 
	 returned) and property mode (read,write or read/write)
	use
    Parameter In
     SysId        Sistem ID
     PropSysName  name of system property
    Parameter Out
	 Mode         type of property: property of read, write or read/write
	 Type         type of variable: string, ushort, short, real, int, uint, 
	              boolean
 *----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetSysPropInfo(int SysId,
                       const char *PropSysName, unsigned *Mode, unsigned *Type);

/* SY1527GETSYSPROP -----------------------------------------------------------
    It returns the value of the property PropName
    Parameter In
     SysId        Sistem ID
     PropName     property name
    Parameter Out
     result
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527GetSysProp(int SysId, const char *PropName,
                                                                  void *result);

/* SY1527SETSYSPROP -----------------------------------------------------------
    Function to set the Property PropName
    Parameter In
     SysId     Sistem ID
     PropName  property name
     set       value to set
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527SetSysProp(int SysId, const char *PropName,
                                                                     void *set);

/* SY1527CAENETCOMM  ----------------------------------------------------------
    Function to get CAENET code to module CAEN via SY1527
    Parameter In
     SysId        Sistem ID
     Crate        crate number of system to communicate
	 Code         code 
	 NrWCode      nr. of additional word code
	 WCode        additional word code
    Parameter Out
	 Result       caenet error code 
	 NrOfData     nr. of data
     Data         response to caenet code

Release 2.00L
 -----------------------------------------------------------------------------*/
 SY1527RESULT Sy1527CaenetComm(int SysId, unsigned short Crate,
         unsigned short Code, unsigned short NrWCode, unsigned short *WCode,
		     short *Result, unsigned short *NrOfData, unsigned short **Data);

/* SY1527SUBSCRIBE  ----------------------------------------------------------
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Parameter In
     SysId         Sistem ID
     UDPPort       UDP Port
	 NrOfItems     nr. of items to subscribe
	 ListOfItems   List of Items
    Parameter Out
	 ListOfCodes   List of codes which identify the items 

Release 3.00
 -----------------------------------------------------------------------------*/
SY1527RESULT Sy1527Subscribe(int SysId, short UDPPort, ushort NrOfItems, 
					   const char *ListOfItems, char *ListOfCodes);

/* SY1527UNSUBSCRIBE  ----------------------------------------------------------
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Parameter In
     SysId              Sistem ID
	 NrOfIDs            nr. of items to unsubscribe
    Parameter Out
	 ListOfErrorCodes   List of error which codes which identify the items 

Release 3.00
 -----------------------------------------------------------------------------*/
SY1527RESULT Sy1527UnSubscribe(int SysId, short UDPPort, short NrOfIDs, const char *ListOfItems,char *ListOfCodes);

/* Sy1527GetEventData  ----------------------------------------------------------
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Parameter In
     SysId              Sistem ID
	 UDPPort			UDP Port
	 NrOfIDs            nr. of items to unsubscribe
    Parameter Out
	 ListOfErrorCodes   List of error which codes which identify the items 

Release 3.00
 -----------------------------------------------------------------------------*/
int Sy1527GetEventData(SOCKET sck, char *buffer, struct sockaddr_in *addr, int *len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
