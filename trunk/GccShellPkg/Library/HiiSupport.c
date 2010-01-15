/*++

Copyright (c) 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  HiiSupport.c

Abstract:

  Implements some helper functions for hii operations.

Revision History

--*/

#include "EfiShellLib.h"

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)

//
// Hii relative protocols
//
BOOLEAN  mHiiProtocolsInitialized = FALSE;

EFI_HII_DATABASE_PROTOCOL *gLibHiiDatabase = NULL;
EFI_HII_STRING_PROTOCOL   *gLibHiiString = NULL;

EFI_STATUS
LocateHiiProtocols (
  VOID
  )
/*++

Routine Description:
  This function locate Hii relative protocols for later usage.

Arguments:
  None.

Returns:
  Status code.

--*/
{
  EFI_STATUS  Status;

  if (mHiiProtocolsInitialized) {
    return EFI_SUCCESS;
  }

  Status = LibLocateProtocol (&gEfiHiiDatabaseProtocolGuid, (VOID **)&gLibHiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = LibLocateProtocol (&gEfiHiiStringProtocolGuid, (VOID **)&gLibHiiString);
  if (EFI_ERROR (Status)) {
    return Status;
  }  
  
  mHiiProtocolsInitialized = TRUE;
  return EFI_SUCCESS;
}

EFI_HII_PACKAGE_LIST_HEADER *
PreparePackageList (
  IN UINTN                    PkgNumber,
  IN EFI_GUID                 *GuidId,
  ...
  )
/*++

Routine Description:

  Assemble EFI_HII_PACKAGE_LIST according to the passed in packages.

Arguments:

  NumberOfPackages  -  Number of packages.
  GuidId            -  Package GUID.

Returns:

  Pointer of EFI_HII_PACKAGE_LIST_HEADER.

--*/
{
  VA_LIST                     Args;
  EFI_HII_PACKAGE_LIST_HEADER *PkgListHdr;
  CHAR8                       *PkgListData;
  UINT32                      PkgListLen;
  UINT32                      PkgLen;
  EFI_HII_PACKAGE_HEADER      PkgHdr = {0, 0};
  UINT8                       *PkgAddr;
  UINTN                       Index;

  PkgListLen = sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  VA_START (Args, GuidId);
  for (Index = 0; Index < PkgNumber; Index++) {
    CopyMem (&PkgLen, VA_ARG (Args, VOID *), sizeof (UINT32));
    PkgListLen += (PkgLen - sizeof (UINT32));
  }

  //
  // Include the lenght of EFI_HII_PACKAGE_END
  //
  PkgListLen += sizeof (EFI_HII_PACKAGE_HEADER);

  VA_END (Args);

  PkgListHdr = AllocateZeroPool (PkgListLen);
  ASSERT (PkgListHdr != NULL);
  CopyMem (&PkgListHdr->PackageListGuid, GuidId, sizeof (EFI_GUID));
  PkgListHdr->PackageLength = PkgListLen;

  PkgListData = (CHAR8 *)PkgListHdr + sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  VA_START (Args, GuidId);
  for (Index = 0; Index < PkgNumber; Index++) {
    PkgAddr = (UINT8 *)VA_ARG (Args, VOID *);
    CopyMem (&PkgLen, PkgAddr, sizeof (UINT32));
    PkgLen  -= sizeof (UINT32);
    PkgAddr += sizeof (UINT32);
    CopyMem (PkgListData, PkgAddr, PkgLen);
    PkgListData += PkgLen;
  }
  VA_END (Args);

  //
  // Append EFI_HII_PACKAGE_END
  //
  PkgHdr.Type = EFI_HII_PACKAGE_END;
  PkgHdr.Length = sizeof (EFI_HII_PACKAGE_HEADER);
  CopyMem (PkgListData, &PkgHdr, PkgHdr.Length);

  return PkgListHdr;
}

EFI_STATUS
GetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
/*++

Routine Description:
  Determine what is the current language setting

Arguments:
  Lang      - Pointer of system language

Returns:
  Status code

--*/
{
  EFI_STATUS  Status;
  UINTN       Size;

  //
  // Get current language setting
  //
  Size = RFC_3066_ENTRY_SIZE;
  Status = RT->GetVariable (
                 L"PlatformLang",
                 &gEfiGlobalVariableGuid,
                 NULL,
                 &Size,
                 Lang
                 );

  if (EFI_ERROR (Status)) {
    strcpya (Lang, "en-US");
  }

  return Status;
}

BOOLEAN
CompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
/*++

Routine Description:

  Compare whether two names of languages are identical.

Arguments:

  Language1 - Name of language 1
  Language2 - Name of language 2

Returns:

  TRUE      - same
  FALSE     - not same

--*/
{
  UINTN Index;
  
  for (Index = 0; (Language1[Index] != 0) && (Language2[Index] != 0); Index++) {
    if (Language1[Index] != Language2[Index]) {
      return FALSE;
    }
  }

  if (((Language1[Index] == 0) && (Language2[Index] == 0))   || 
      ((Language1[Index] == 0) && (Language2[Index] != ';')) ||
      ((Language1[Index] == ';') && (Language2[Index] != 0)) ||
      ((Language1[Index] == ';') && (Language2[Index] != ';'))) {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
LibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  )
/*++

  Routine Description:
    This function try to retrieve string from String package of current language.
    If fail, it try to retrieve string from String package of first language it support.

  Arguments:
    PackageList       - The package list in the HII database to search for the specified string.
    StringId          - The string's id, which is unique within PackageList.
    String            - Points to the new null-terminated string.
    StringSize        - On entry, points to the size of the buffer pointed to by String, in bytes. On return,
                        points to the length of the string, in bytes.

  Returns:
    EFI_SUCCESS            - The string was returned successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not available.
    EFI_BUFFER_TOO_SMALL   - The buffer specified by StringLength is too small to hold the string.
    EFI_INVALID_PARAMETER  - The String or StringSize was NULL.
    EFI_UNSUPPORTED        - This operation is not supported since the protocol
                             interface is unavailable.

--*/
{
  EFI_STATUS Status;
  CHAR8      CurrentLang[RFC_3066_ENTRY_SIZE];

  Status = LocateHiiProtocols ();
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  GetCurrentLanguage (CurrentLang);

  Status = gLibHiiString->GetString (
                            gLibHiiString,
                            CurrentLang,
                            PackageList,
                            StringId,
                            String,
                            StringSize,
                            NULL
                            );

  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    //
    // Since en-US should be supported by all shell strings, if we cannot get 
    // the string in current language, use the en-US instead.
    //
    strcpya (CurrentLang, "en-US");

    Status = gLibHiiString->GetString (
                              gLibHiiString,
                              CurrentLang,
                              PackageList,
                              StringId,
                              String,
                              StringSize,
                              NULL
                              );
  }

  return Status;
}

#endif

