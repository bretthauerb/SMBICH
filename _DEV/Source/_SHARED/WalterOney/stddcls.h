// stddcls.h -- Precompiled headers for WDM drivers
// Copyright (C) 1999 by Walter Oney
// All rights reserved
#ifndef STDDCLS_H_INCLUDED
#define STDDCLS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//#include <wdm.h>
#include <ntddk.h>
#include <stdio.h>

#ifdef __cplusplus
	}
#endif

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

// Override DDK definition of ASSERT so that debugger halts in the
// affected code and halts even in the unchecked OS

#if DBG && defined(_X86_)
	#undef ASSERT
	#define ASSERT(e) if(!(e)){DbgPrint("Assertion failure in "\
	__FILE__ ", line %d: " #e "\n", __LINE__);\
	_asm int 1\
  }
#endif

// Currently, the support routines for managing the device remove lock aren't
// defined in the WDM.H or implemented in Windows 98. The following declarations
// provide equivalent functionality on all WDM platforms by means of a private
// implementation in RemoveLock.cpp

typedef struct _REMOVE_LOCK {
	LONG usage;					// reference count
	BOOLEAN removing;			// true if removal is pending
	KEVENT evRemove;			// event to wait on
	} REMOVE_LOCK, *PREMOVE_LOCK;

VOID InitializeRemoveLock(PREMOVE_LOCK lock, ULONG tag, ULONG minutes, ULONG maxcount);
NTSTATUS AcquireRemoveLock(PREMOVE_LOCK lock, PVOID tag);
VOID ReleaseRemoveLock(PREMOVE_LOCK lock, PVOID tag);
VOID ReleaseRemoveLockAndWait(PREMOVE_LOCK lock, PVOID tag);

// Redefine macros in case the programmer changes the driver to include NTDDK.H
// instead of WDM.H

#undef IoInitializeRemoveLock
#define IoInitializeRemoveLock InitializeRemoveLock

#undef IoAcquireRemoveLock
#define IoAcquireRemoveLock AcquireRemoveLock

#undef IoReleaseRemoveLock
#define IoReleaseRemoveLock ReleaseRemoveLock

#undef IoReleaseRemoveLockAndWait
#define IoReleaseRemoveLockAndWait ReleaseRemoveLockAndWait

#define _IO_REMOVE_LOCK _REMOVE_LOCK
#define IO_REMOVE_LOCK REMOVE_LOCK
#define PIO_REMOVE_LOCK PREMOVE_LOCK

#endif STDDCLS_H_INCLUDED
