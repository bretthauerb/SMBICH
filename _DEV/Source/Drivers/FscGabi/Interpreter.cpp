#include <ntddk.h>
#include "Interpreter.h"
#include "InterpreterInternal.h"
#include "AssemlerHelper.h"
#ifdef USERMODE
#include <stdlib.h>

#define ALLOCATE_BUFFER(x) malloc(x)
#define FREE_BUFFER(x) free(x)
#else
#define ALLOCATE_BUFFER(x) ExAllocatePoolWithTag(NonPagedPool, x, 'InBu');
#define FREE_BUFFER(x) ExFreePoolWithTag(x, 'InBu')
#endif

psInterpreterContext InitInterpreter(UCHAR* pEntryPoint, USHORT wSize, UCHAR ucAddressingMode)
{
	psInterpreterContext pContext;

	if (pEntryPoint == NULL || wSize == 0 || (ucAddressingMode != INTERPRETER_32BIT && ucAddressingMode != INTERPRETER_64BIT))
	{
		return NULL;
	}

	pContext = (psInterpreterContext)ALLOCATE_BUFFER(sizeof(sInterpreterContext));	

	if (pContext != NULL)
	{
		memset(pContext, 0, sizeof(sInterpreterContext));

		pContext->pEntryPoint = pEntryPoint;
		pContext->wSize = wSize;
		pContext->ucAddressingMode = ucAddressingMode;
		pContext->pInterpreterState = ALLOCATE_BUFFER(STACK_SIZE);
	}

	return pContext;
}

void CleanupInterpreter(psInterpreterContext pContext)
{
	if (pContext == NULL)
	{
		return;
	}

	if (pContext->pInterpreterState != NULL)
	{
		FREE_BUFFER(pContext->pInterpreterState);
		pContext->pInterpreterState = NULL;
	}

	if (pContext != NULL)
	{
		FREE_BUFFER(pContext);
	}
}

eInterpreterReturn ExecuteInterpreter(psInterpreterContext pContext, PHYSICAL_ADDRESS pParam1, PHYSICAL_ADDRESS pParam2, PHYSICAL_ADDRESS pParam3)
{
	eInterpreterReturn ret;
	sInstruction sCurrentInstruction;
	USHORT wCurrentPC;

	if (pContext == NULL || pContext->pEntryPoint == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	ret = InitExecutionContext(pContext, pParam1, pParam2, pParam3);

	if (ret != INTERPRETER_OK)
	{
		return ret;
	}

	wCurrentPC = (USHORT)GET_STATE(pContext)->RIP;

	while ((ret = GetNextCommand(pContext, &wCurrentPC, &sCurrentInstruction)) == INTERPRETER_OK)
	{
		ret = ExecuteInstruction(pContext, &sCurrentInstruction);

		if (ret != INTERPRETER_OK && ret != INTERPRETER_SEGMENT_OK)
		{
			return ret;
		}

		wCurrentPC = (USHORT)GET_STATE(pContext)->RIP;

		if (sCurrentInstruction.Type == INTERPRETER_RET)
		{
			if (ret == INTERPRETER_SEGMENT_OK)
			{
				break;
			}
		}
	}

	if (ret != INTERPRETER_SEGMENT_OK)
	{
		return ret;
	}

	return INTERPRETER_OK;
}

eInterpreterReturn AnalyzeInterpreter(psInterpreterContext pContext)
{
	eInterpreterReturn ret;
	USHORT wCurrentIndex = 0;
	sInstruction sCurrentInstruction;

	if (pContext == NULL || pContext->pEntryPoint == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	while ((ret = GetNextCommand(pContext, &wCurrentIndex, &sCurrentInstruction)) == INTERPRETER_OK)
	{
		if (sCurrentInstruction.Type == INTERPRETER_RET)
		{
			ret = INTERPRETER_SEGMENT_OK;
			break;
		}

		wCurrentIndex++;
	}

	if (ret != INTERPRETER_SEGMENT_OK)
	{
		return ret;
	}

	return INTERPRETER_OK;
}
