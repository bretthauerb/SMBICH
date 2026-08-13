#ifdef __cplusplus
	extern "C" {
#endif

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4214 )
#pragma warning ( disable : 4115 )


#define DEBUGLEVEL_OFF						0x0		// 0 ==> No debugging messages
#define DEBUGLEVEL_DEBUG					0x1		// 0001b
#define DEBUGLEVEL_INFO						0x2		// 0010b
#define DEBUGLEVEL_WARNING					0x4		// 0100b
#define DEBUGLEVEL_ERROR					0x8		// 1000b
#define DEFAULT_DEBUGLEVEL					DEBUGLEVEL_OFF

extern int DebugLevel;

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
void DebugPrint(int	DebugPrintLevel,
				char *DebugMessage,
				...);
#ifdef __cplusplus
}
#endif
