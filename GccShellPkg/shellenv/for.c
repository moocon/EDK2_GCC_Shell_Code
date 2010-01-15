/*++

Copyright (c) 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution. The full text of the license may be found at         
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  for.c
  
Abstract:

  Internal Shell cmd "for" & "endfor"

Revision History

--*/

#include "shelle.h"
#include "shellenvguid.h"


EFI_STATUS
SEnvCmdForRun (
  IN EFI_HANDLE                         hImageHandle,
  IN EFI_SYSTEM_TABLE                   *pSystemTable
  )
{
  EFI_STATUS  Status;
  BOOLEAN     bSuccess;

  Status = EFI_SUCCESS;
  if (!SEnvBatchGetGotoActive ()) {
    //
    // Extra check. This checking is only done when Goto is not active, for
    // it's legal for encountering a loop use a same named variable as the
    // current loop when we're searching for the target label.
    //
    if ((StrLen (SI->Argv[1]) != 1) || !IsAlpha (SI->Argv[1][0])) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
      SEnvSetBatchAbort ();
      return EFI_INVALID_PARAMETER;
    }
    //
    //  If Goto is not active, then push it to the statement stack.
    //
    Status = SEnvBatchPushFor2Stack (StmtFor, FALSE);
    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
      SEnvSetBatchAbort ();
      return Status;
    }

  } else {
    //
    // If Goto is active, maintain the JumpStmt or the extra statement stack,
    // so that JumpStmt points to the current statement, or extra stack holds
    // the current statement.
    //
    if (SEnvBatchGetRewind ()) {
      Status = SEnvTryMoveUpJumpStmt (StmtFor, &bSuccess);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
        SEnvSetBatchAbort ();
        return Status;
      }

      if (bSuccess) {
        return Status;
      }
    }

    Status = SEnvBatchPushFor2Stack (StmtFor, TRUE);
    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
      SEnvSetBatchAbort ();
      return Status;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
SEnvCmdFor (
  IN  EFI_HANDLE               ImageHandle,
  IN  EFI_SYSTEM_TABLE         *SystemTable
  )
/*++

Routine Description:

  Shell command "for" for loop in script files.

Arguments:
  ImageHandle      The image handle
  SystemTable      The system table

Returns:
  EFI_SUCCESS      The command finished sucessfully
  EFI_UNSUPPORTED  Unsupported
  EFI_INVALID_PARAMETER Invalid parameter
--*/
{
  EFI_STATUS  Status;
  BOOLEAN     Success;

  EFI_SHELL_APP_INIT (ImageHandle, SystemTable);

  //
  //  Output help
  //
  if (SI->Argc == 2) {
    if (StriCmp (SI->Argv[1], L"-?") == 0) {
      if (IS_OLD_SHELL) {
        PrintToken (STRING_TOKEN (STR_NO_HELP), HiiEnvHandle);
      } else {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_VERBOSE_HELP), HiiEnvHandle);
      }

      return EFI_SUCCESS;
    }
  } else if (SI->Argc == 3) {
    if ((StriCmp (SI->Argv[1], L"-?") == 0 && StriCmp (SI->Argv[2], L"-b") == 0) ||
        (StriCmp (SI->Argv[2], L"-?") == 0 && StriCmp (SI->Argv[1], L"-b") == 0)
        ) {
      EnablePageBreak (DEFAULT_INIT_ROW, DEFAULT_AUTO_LF);
      if (IS_OLD_SHELL) {
        PrintToken (STRING_TOKEN (STR_NO_HELP), HiiEnvHandle);
      } else {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_VERBOSE_HELP), HiiEnvHandle);
      }

      return EFI_SUCCESS;
    }
  }

  if (!SEnvBatchIsActive ()) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_ONLY_SCRIPT), HiiEnvHandle, L"for");
    return EFI_UNSUPPORTED;
  }
  //
  // Check the command line syntax. The syntax of statement for is:
  //   for %<var> in <string | file [[string | file]...]>
  //
  if (SI->Argc < 4) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
    SEnvSetBatchAbort ();
    return EFI_INVALID_PARAMETER;
  }

  if (0 == StriCmp (SI->Argv[2], L"run")) {
    return SEnvCmdForRun (ImageHandle, SystemTable);
  } else if (StriCmp (SI->Argv[2], L"in") != 0) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
    SEnvSetBatchAbort ();
    return EFI_INVALID_PARAMETER;
  }

  if (!SEnvBatchGetGotoActive ()) {
    //
    // Extra check. This checking is only done when Goto is not active, for
    // it's legal for encountering a loop use a same named variable as the
    // current loop when we're searching for the target label.
    //
    if ((StrLen (SI->Argv[1]) != 1) || !IsAlpha (SI->Argv[1][0])) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
      SEnvSetBatchAbort ();
      return EFI_INVALID_PARAMETER;
    }
    //
    //  If Goto is not active, then push it to the statement stack.
    //
    Status = SEnvBatchPushStmtStack (StmtFor, FALSE);
    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
      SEnvSetBatchAbort ();
      return Status;
    }

  } else {
    //
    // If Goto is active, maintain the JumpStmt or the extra statement stack,
    // so that JumpStmt points to the current statement, or extra stack holds
    // the current statement.
    //
    if (SEnvBatchGetRewind ()) {
      Status = SEnvTryMoveUpJumpStmt (StmtFor, &Success);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
        SEnvSetBatchAbort ();
        return Status;
      }

      if (Success) {
        return Status;
      }
    }

    Status = SEnvBatchPushStmtStack (StmtFor, TRUE);
    if (EFI_ERROR (Status)) {
      PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
      SEnvSetBatchAbort ();
      return Status;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
SEnvCmdEndfor (
  IN  EFI_HANDLE               ImageHandle,
  IN  EFI_SYSTEM_TABLE         *SystemTable
  )
/*++

Routine Description:

    Shell command "endfor".

Arguments:
  ImageHandle      The image handle
  SystemTable      The system table

Returns:
  EFI_SUCCESS      The command finished sucessfully
  EFI_UNSUPPORTED  Unsupported
  EFI_ABORTED      Aborted
--*/
{
  EFI_STATUS          Status;
  EFI_LIST_ENTRY      *VarLink;
  EFI_BATCH_STATEMENT *Stmt;
  EFI_BATCH_VAR_VALUE *VarValue;

  EFI_SHELL_APP_INIT (ImageHandle, SystemTable);

  if (!SEnvBatchIsActive ()) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_ENDFOR_ONLY_SUPPORTED_SCRIPT), HiiEnvHandle);
    return EFI_UNSUPPORTED;
  }

  if (SI->Argc > 1) {
    PrintToken (STRING_TOKEN (STR_SHELLENV_ENDFOR_TOO_MANY_ARGUMENTS), HiiEnvHandle, SEnvGetLineNumber ());
    SEnvSetBatchAbort ();
    return EFI_INVALID_PARAMETER;
  }

  if (!SEnvBatchGetGotoActive ()) {
    //
    // If Goto is not active, do the following steps:
    //   1. Check for corresponding "for"
    //   2. Delete one node from variable value list
    //   3. If the variable value list is empty, stop looping, otherwise
    //      continue loop
    //
    Stmt = SEnvBatchStmtStackTop ();
    if (Stmt == NULL || Stmt->StmtType != StmtFor) {
      PrintToken (
        STRING_TOKEN (STR_SHELLENV_ENDFOR_NO_CORRESPDING_FOR),
        HiiEnvHandle,
        SEnvGetLineNumber ()
        );
      SEnvSetBatchAbort ();
      return EFI_ABORTED;
    }
    //
    // It's possible for ValueList to be empty(for example, a "for" in a false
    // condition "if"). If so, we need not delete a value node from it.
    //
    if (!IsListEmpty (&Stmt->StmtInfo.ForInfo.ValueList)) {
      VarLink = Stmt->StmtInfo.ForInfo.ValueList.Flink;
      VarValue = CR (
                  VarLink,
                  EFI_BATCH_VAR_VALUE,
                  Link,
                  EFI_BATCH_VAR_SIGNATURE
                  );

      //
      //  Free the string contained in the first node of variable value list
      //
      if (VarValue->Value != NULL) {
        FreePool (VarValue->Value);
        VarValue->Value = NULL;
      }
      //
      //  Remove the first node from the variable value list
      //
      RemoveEntryList (&VarValue->Link);
      FreePool (VarValue);
      VarValue = NULL;
    }
    //
    //  If there is another value, then jump back to top of loop,
    //  otherwise, exit this FOR loop & pop out the statement.
    //
    if (!IsListEmpty (&Stmt->StmtInfo.ForInfo.ValueList)) {
      //
      //  Set script file position back to top of this loop
      //
      SEnvSetLineNumber (Stmt->StmtInfo.ForInfo.BeginLineNum);
      Status = SEnvBatchSetFilePosition (Stmt->BeginFilePos);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
        SEnvSetBatchAbort ();
        return Status;
      }

    } else {
      //
      // Pop the statement out of stack to exit loop
      //
      Status = SEnvBatchPopStmtStack (1, FALSE);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
        SEnvSetBatchAbort ();
        return Status;
      }
    }

    return EFI_SUCCESS;

  } else {
    //
    // if Goto is active, maintain the JumpStmt or the extra statement stack.
    //
    if (!SEnvBatchExtraStackEmpty ()) {
      Stmt = SEnvBatchExtraStackTop ();
      if (Stmt->StmtType != StmtFor) {
        PrintToken (STRING_TOKEN (STR_SHELLENV_ENDFOR_NO_FOR_STATEMENT), HiiEnvHandle, SEnvGetLineNumber ());
        SEnvSetBatchAbort ();
        return EFI_ABORTED;
      }

      Status = SEnvBatchPopStmtStack (1, TRUE);
      if (EFI_ERROR (Status)) {
        PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
        SEnvSetBatchAbort ();
        return Status;
      }

    } else {
      Status = SEnvMoveDownJumpStmt (StmtFor);
      if (EFI_ERROR (Status)) {
        if (Status == EFI_NOT_FOUND) {
          PrintToken (
            STRING_TOKEN (STR_SHELLENV_ENDFOR_NO_CORRESPDING_FOR),
            HiiEnvHandle,
            SEnvGetLineNumber ()
            );
        } else {
          PrintToken (STRING_TOKEN (STR_SHELLENV_FOR_INCORRECT_SYNTAX), HiiEnvHandle, L"for", SEnvGetLineNumber ());
        }

        SEnvSetBatchAbort ();
        return Status;
      }
    }

    return Status;
  }
}

EFI_STATUS
EFIAPI
SEnvCmdForGetLineHelp (
  OUT CHAR16              **Str
  )
/*++

Routine Description:

  Get this command's line help

Arguments:

  Str - The line help

Returns:

  EFI_SUCCESS   - Success

--*/
{
  return SEnvCmdGetStringByToken (STRING_TOKEN (STR_SHELLENV_FOR_LINE_HELP), Str);
}

EFI_STATUS
EFIAPI
SEnvCmdEndforGetLineHelp (
  OUT CHAR16              **Str
  )
/*++

Routine Description:

  Get this command's line help

Arguments:

  Str - The line help

Returns:

  EFI_SUCCESS   - Success

--*/
{
  return EFI_UNSUPPORTED;
}
