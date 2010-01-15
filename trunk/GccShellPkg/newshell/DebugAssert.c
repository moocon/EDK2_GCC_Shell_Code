/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugAssert.c

Abstract:

  Produce EfiDebugAssertProtocol to enable EfiUtilityLib to function.
  The EfiUtilityLib is used by the EFI shell!

--*/
#include "nshell.h"
#include EFI_PROTOCOL_DEFINITION (DebugAssert)


UINTN gRtErrorLevel = 0; 

EFI_STATUS
EFIAPI
ShellDebugAssert (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN CHAR8                              *FileName,
  IN INTN                               LineNumber,
  IN CHAR8                              *Description
  );

EFI_STATUS
EFIAPI
ShellDebugPrint (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN UINTN                              ErrorLevel,
  IN CHAR8                              *Format,
  IN VA_LIST                            Marker
  );

EFI_STATUS
EFIAPI
ShellPostCode (
  IN EFI_DEBUG_ASSERT_PROTOCOL          * This,
  IN  UINT16                            PostCode,
  IN  CHAR8                             *PostCodeString OPTIONAL
  );

EFI_STATUS
EFIAPI
ShellGetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             *ErrorLevel
  );

EFI_STATUS
EFIAPI
ShellSetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             ErrorLevel
  );

//
// Protocol instance, there can be only one.
//
EFI_HANDLE                mHandle = NULL;
EFI_DEBUG_ASSERT_PROTOCOL mDebugAssertProtocol = {
  ShellDebugAssert,
  ShellDebugPrint,
  ShellPostCode,
  ShellGetErrorLevel,
  ShellSetErrorLevel
};

//
// Function implementations
//
EFI_STATUS
EFIAPI
ShellDebugAssert (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN CHAR8                              *FileName,
  IN INTN                               LineNumber,
  IN CHAR8                              *Description
  )
/*++

Routine Description:

  Worker function for ASSERT (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded EFI_BREAKPOINT ().
  
Arguments:

  This        - Protocol instance.
  FileName    - File name of failing routine.
  LineNumber  - Line number of failing ASSERT().
  Description - Description, usually the assertion,

Returns:

  EFI_SUCCESS   The function always completes successfully.

--*/
{
  Print (L"\nASSERT (%a): %a:%d\n", Description, FileName, LineNumber);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ShellDebugPrint (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN UINTN                              ErrorLevel,
  IN CHAR8                              *Format,
  IN VA_LIST                            Marker
  )
/*++

Routine Description:

  Worker function for DEBUG (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.
  
Arguments:

  This       - Protocol Instance.
  ErrorLevel - If error level is set do the debug print.
  Format     - String to use for the print, followed by Print arguments.

Returns:

  EFI_SUCCESS   The function always completes successfully.

--*/
{
  CHAR16  Buffer[180];
  CHAR16  UnicodeFormat[180];
  UINTN   Index;
  
  if (!(gRtErrorLevel & ErrorLevel)) {
    return EFI_SUCCESS;
  }
  
  for (Index = 0; Format[Index] != '\0'; Index++) {
    UnicodeFormat[Index] = (CHAR16)Format[Index];
  }
  Format[Index] = '\0';
  
  VSPrint (Buffer, sizeof (Buffer), UnicodeFormat, Marker);
  Print (Buffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ShellPostCode (
  IN EFI_DEBUG_ASSERT_PROTOCOL          * This,
  IN  UINT16                            PostCode,
  IN  CHAR8                             *PostCodeString OPTIONAL
  )
/*++

Routine Description:

  Write the code to IO ports 80 and 81.

Arguments:

  This            - Protocol Instance.
  PostCode        - Code to write
  PostCodeString  - String, currently ignored.

Returns:

  EFI_SUCCESS   The function always completes successfully.

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ShellGetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             *ErrorLevel
  )
{
  *ErrorLevel = gRtErrorLevel;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ShellSetErrorLevel (
  IN EFI_DEBUG_ASSERT_PROTOCOL          *This,
  IN  UINTN                             ErrorLevel
  )
{
  gRtErrorLevel = ErrorLevel;
  return EFI_SUCCESS;
}

EFI_STATUS
InstallShellDebugAssert (
  VOID
  )
/*++

Routine Description:

  Install the status code debug assert protocol

Arguments:

  None

Returns:

  Results of call to InstallProtocolInterface.

--*/
{
  DEBUG_CODE (
    EFI_STATUS  Status;
    VOID        *Interface;
    
    Status = BS->LocateProtocol (&gEfiDebugAssertProtocolGuid, NULL, &Interface);
    if (EFI_ERROR (Status)) {
      BS->InstallProtocolInterface (
            &mHandle,
            &gEfiDebugAssertProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &mDebugAssertProtocol
            );
     }
  );

  return EFI_SUCCESS;
}
