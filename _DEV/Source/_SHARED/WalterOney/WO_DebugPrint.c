#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4214 )
#pragma warning ( disable : 4115 )

#ifdef __cplusplus
	extern "C" {
#endif

#include "stddcls.h"
#include <stdarg.h>			// for variable argument list in DebugPrint function
#include <ntstrsafe.h>
#include "DebugPrint.h"

//#pragma LOCKEDCODE
#pragma code_seg()
		
int DebugLevel = 0; //DEBUGLEVEL_ERROR;

//_______DebugPrint____________________________________________________________


//////////////////////////////////////////////////////////////////////
//  DebugPrint - Debugprint for all output messages
//  Input:  DebugPrintLevel - the priority of the message, can be
//				0 - Off (default)
//				1 - Lowest (message with highest detail)
//				2 - Low (typically INFO level)
//				4 - Normal (typically WARNING level)
//				8 - High (typeicall ERROR level)
//  Output: 
//  Notes:  
//////////////////////////////////////////////////////////////////////
void DebugPrint(int	 DebugPrintLevel,
				char *DebugMessage,
				...)
{
	va_list	ap;

	va_start(ap, DebugMessage);

	if ( DebugPrintLevel == DEBUGLEVEL_OFF || DebugLevel == DEBUGLEVEL_OFF )
	{
		va_end(ap);
		return;
	}

	if ( DebugPrintLevel & DebugLevel )
	{
		char	DebugBuffer[256];
		char	NewDebugMessage[256];
		NTSTATUS status = STATUS_SUCCESS;

		status = RtlStringCbCopyA(NewDebugMessage, sizeof(NewDebugMessage), "SMBus");
		switch ( DebugPrintLevel )
		{
		case DEBUGLEVEL_DEBUG:
			if (NT_SUCCESS(status))
				status = RtlStringCbCatA(NewDebugMessage, sizeof(NewDebugMessage), " (DEBUG)   ");
			break;
		case DEBUGLEVEL_INFO:
			if (NT_SUCCESS(status))
				status = RtlStringCbCatA(NewDebugMessage, sizeof(NewDebugMessage), " (INFO)    ");
			break;
		case DEBUGLEVEL_WARNING:
			if (NT_SUCCESS(status))
				status = RtlStringCbCatA(NewDebugMessage, sizeof(NewDebugMessage), " (WARNING) ");
			break;
		case DEBUGLEVEL_ERROR:
			if (NT_SUCCESS(status))
				status = RtlStringCbCatA(NewDebugMessage, sizeof(NewDebugMessage), " (ERROR)   ");
			break;
		}
		if (NT_SUCCESS(status))
			status = RtlStringCbCatA(NewDebugMessage, sizeof(NewDebugMessage), DebugMessage);
		if (NT_SUCCESS(status))
			status = RtlStringCbVPrintfA(DebugBuffer, sizeof(DebugBuffer), NewDebugMessage, ap);
		if (NT_SUCCESS(status))
			status = RtlStringCbCatA(DebugBuffer, sizeof(DebugBuffer), "\n");

		if (NT_SUCCESS(status))
			DbgPrint("%s", DebugBuffer);
	}

	va_end(ap);
}
#ifdef __cplusplus
}
#endif
