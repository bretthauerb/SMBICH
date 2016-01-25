#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	UCHAR* pEntryPoint;
	USHORT wSize;
	UCHAR ucAddressingMode;
	void* pInterpreterState;
}sInterpreterContext, *psInterpreterContext;

typedef enum
{
	INTERPRETER_OK = 0,
	INTERPRETER_SEGMENT_OK,
	INTERPRETER_E_ARGS,
	INTERPRETER_E_EOF,
	INTERPRETER_E_STACK_ERROR,
	INTERPRETER_E_INVALID_INSTRUCTION,
	INTERPRETER_E_UNKNOWN_INSTRUCTION
}eInterpreterReturn;

#define INTERPRETER_64BIT 1
#define INTERPRETER_32BIT 0

psInterpreterContext InitInterpreter(UCHAR* pEntryPoint, USHORT wSize, UCHAR ucAddressingMode);
void CleanupInterpreter(psInterpreterContext pContext);

eInterpreterReturn AnalyzeInterpreter(psInterpreterContext pContext);
eInterpreterReturn ExecuteInterpreter(psInterpreterContext pContext, PHYSICAL_ADDRESS pParam1, PHYSICAL_ADDRESS pParam2, PHYSICAL_ADDRESS pParam3);

#ifdef __cplusplus
}
#endif