#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4214 )
#pragma warning ( disable : 4115 )

#ifdef __cplusplus
	extern "C" {
#endif

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>			// for variable argument list in DebugPrint function
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
		return;

	if ( DebugPrintLevel & DebugLevel )
	{
		char	DebugBuffer[256];
		char	NewDebugMessage[256];

		strcpy((char *)NewDebugMessage, "SMBus");
		switch ( DebugPrintLevel )
		{
		case DEBUGLEVEL_DEBUG:
			strcat((char *)NewDebugMessage, " (DEBUG)   ");
			break;
		case DEBUGLEVEL_INFO:
			strcat((char *)NewDebugMessage, " (INFO)    ");
			break;
		case DEBUGLEVEL_WARNING:
			strcat((char *)NewDebugMessage, " (WARNING) ");
			break;
		case DEBUGLEVEL_ERROR:
			strcat((char *)NewDebugMessage, " (ERROR)   ");
			break;
		}
		strcat((char *)NewDebugMessage, (char *)DebugMessage);

		_vsnprintf((char *)DebugBuffer, 256, (char *)NewDebugMessage, ap);

			// append the newline
		strcat((char *)DebugBuffer, "\n");

		DbgPrint((char *)DebugBuffer);
	}

	va_end(ap);
}
#ifdef __cplusplus
}
#endif
