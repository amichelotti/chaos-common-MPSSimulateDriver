/***************************************************************************/
/*                                                                         */
/*        --- CAEN Engineering Srl - Computing Systems Division ---        */
/*                                                                         */
/*    A303.H                                                			         */
/*                                                                         */
/*    Declarations of routines which permit the control of A303/A module   */
/*    to a generic ANSI C program.                                         */
/*                                                                         */
/*    Created: January 2000                                                */
/*                                                                         */
/***************************************************************************/

#define MAX_LENGTH_FIFO 4096
#define TUTTOK             0


/***------------------------------------------------------------------------

  A303Init
  Must be called before any other call to routines declared here.
  It initializes operating system dependent structures to correctly drive
  the A303/A module mapped at the I/O port specified.
  Arguments:
    Port - the I/O port
  Return value:
    error code, see A303DecodeResp for explaination.

    --------------------------------------------------------------------***/
int   A303Init(unsigned long Port);

/***------------------------------------------------------------------------

  A303SendCommand
  Sends a command to a device in the Caenet chain.
  Arguments:
    code  - the code of the command
    crnum - the crate number of the device in the chain
    buf   - a byte-array containing arguments eventually needed by the
            command specified
    count - length of the previous byte array
  Return value:
    error code, see A303DecodeResp for explaination.

    --------------------------------------------------------------------***/
int   A303SendCommand(int code, int crnum, void *buf, int count);

/***------------------------------------------------------------------------

  A303ReadResponse
  Waits for a response from the device.
  Arguments:
    buf   - the response from the device
    count - the length, in bytes, of the response
  Return value:
    error code, see A303DecodeResp for explaination.

    --------------------------------------------------------------------***/
int   A303ReadResponse(void *buf, int *count);

/***------------------------------------------------------------------------

  A303Reset
  Resets the A303/A module.
  Arguments:
    none
  Return value:
    error code, see A303DecodeResp for explaination.

    --------------------------------------------------------------------***/
int   A303Reset(void);

/***------------------------------------------------------------------------

  A303Timeout
  Sets the time to wait for a response. 
  Arguments:
    Timeout - Time to wait in tenth of seconds.
  Return value:
    error code, see A303DecodeResp for explaination.

    --------------------------------------------------------------------***/
int   A303Timeout(unsigned long Timeout);

/***------------------------------------------------------------------------

  A303End
  Must be called after any other call to routines declared here.
  It resets operating system dependent structures.
  Arguments:
    none
  Return value:
    error code, see A303DecodeResp for explaination.

    --------------------------------------------------------------------***/
int   A303End(void);

/***------------------------------------------------------------------------

  A303DecodeResp
  Given the value returned from any routine declared here it returns an
  array of char that explains the error. Errors can be of two types: Caenet 
  communication errors or operating system dependent.
  Arguments:
    resp - a code returned by any of the routines declared here.
  Return value:
    an explaination string for the error. 

    --------------------------------------------------------------------***/
char *A303DecodeResp(int resp);
