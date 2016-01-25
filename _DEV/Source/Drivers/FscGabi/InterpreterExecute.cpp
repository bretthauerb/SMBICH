#include <ntddk.h>
#include "Interpreter.h"
#include "InterpreterInternal.h"
#include "AssemlerHelper.h"

eInterpreterReturn InitExecutionContext(psInterpreterContext pContext, PHYSICAL_ADDRESS pParam1, PHYSICAL_ADDRESS pParam2, PHYSICAL_ADDRESS pParam3)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	ULONGLONG value;
	UCHAR ucSize = (pContext->ucAddressingMode == INTERPRETER_64BIT) ? 8 : 4;

	if (pContext == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	memset(GET_STATE(pContext), 0, STACK_SIZE);
	GET_STATE(pContext)->SP.RVALUE = STACK_SIZE; //empty stack => offset is memory length

	//interrupts are enabled by default
	GET_STATE(pContext)->RFLAGS |= IF_MASK;

	//add parameter to stack 
	GET_STATE(pContext)->SP.RVALUE -= ucSize;
#if defined(_AMD64_) || defined(_IA64_)
	value = (ULONGLONG)pParam3.QuadPart;
#else
	value = (ULONG)pParam3.LowPart;
#endif
	memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);
	SET_REGISTER(pContext, R9, value, ucSize);

	GET_STATE(pContext)->SP.RVALUE -= ucSize;
#if defined(_AMD64_) || defined(_IA64_)
	value = (ULONGLONG)pParam2.QuadPart;
#else
	value = (ULONG)pParam2.LowPart;
#endif
	memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);
	SET_REGISTER(pContext, R8, value, ucSize);

	GET_STATE(pContext)->SP.RVALUE -= ucSize;
#if defined(_AMD64_) || defined(_IA64_)
	value = (ULONGLONG)pParam1.QuadPart;
#else
	value = (ULONG)pParam1.LowPart;
#endif
	memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);
	SET_REGISTER(pContext, D, value, ucSize);

	//set rcx value
#if defined(_AMD64_) || defined(_IA64_)
	value = (ULONGLONG)pContext->pEntryPoint;
#else
	value = (ULONG)pContext->pEntryPoint;
#endif
	SET_REGISTER(pContext, C, value, ucSize);

	//add current IP (simulate call)
	value = GET_STATE(pContext)->RIP;
	GET_STATE(pContext)->SP.RVALUE -= ucSize;
	memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);

	return ret;
}

eInterpreterReturn ExecuteInstruction(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	switch (pInstruction->Type)
	{
		case INTERPRETER_MOV:
			ret = ExecuteInstruction_MOV(pContext, pInstruction);
			break;

		case INTERPRETER_SHIFT:
			ret = ExecuteInstruction_SHIFT(pContext, pInstruction);
			break;

		case INTERPRETER_ADD:
			ret = ExecuteInstruction_ADD(pContext, pInstruction);
			break;

		case INTERPRETER_SUB:
			ret = ExecuteInstruction_SUB(pContext, pInstruction);
			break;

		case INTERPRETER_CLI:
			ret = ExecuteInstruction_CLI(pContext, pInstruction);
			break;

		case INTERPRETER_STI:
			ret = ExecuteInstruction_STI(pContext, pInstruction);
			break;

		case INTERPRETER_OUT:
			ret = ExecuteInstruction_OUT(pContext, pInstruction);
			break;

		case INTERPRETER_CMP:
			ret = ExecuteInstruction_CMP(pContext, pInstruction);
			break;

		case INTERPRETER_JUMP:
			ret = ExecuteInstruction_JUMP(pContext, pInstruction);

			if (ret == INTERPRETER_SEGMENT_OK)
			{
				//this is the marker that RIP is absolute changed => return OK.
				return INTERPRETER_OK;
			}
			break;

		case INTERPRETER_PUSH:
			ret = ExecuteInstruction_PUSH(pContext, pInstruction);
			break;

		case INTERPRETER_POP:
			ret = ExecuteInstruction_POP(pContext, pInstruction);
			break;

		case INTERPRETER_RET:
			ret = ExecuteInstruction_RET(pContext, pInstruction);

			if (ret == INTERPRETER_SEGMENT_OK)
			{
				//this is the marker that RIP is absolute changed => return OK.
				return INTERPRETER_OK;
			}
			else
			{
				//last ret command => stop interpreter
				ret = INTERPRETER_SEGMENT_OK;
			}
			break;

		case INTERPRETER_CALL:
			ret = ExecuteInstruction_CALL(pContext, pInstruction);
			break;

		case INTERPRETER_NOP:
			//nop => nothing to do
			break;

		default:
			return INTERPRETER_E_UNKNOWN_INSTRUCTION;
	}

	if (ret == INTERPRETER_OK)
	{
		GET_STATE(pContext)->RIP += pInstruction->Len;
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_MOV(psInterpreterContext pContext, psInstruction pInstruction)
{
	UCHAR ucSize;
	eInterpreterReturn ret = INTERPRETER_OK;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	ucSize = pInstruction->u.MOV_ARGS.Size;

	if (pInstruction->u.MOV_ARGS.OI || (pInstruction->u.MOV_ARGS.MI && pInstruction->HasIndirectAdressing == 0))
	{
		ULONGLONG value = pInstruction->u.MOV_ARGS.Operand2;

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = SetRegisterValue(pContext, 1, pInstruction->u.MOV_ARGS.Operand1 & 0x7, value, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = SetRegisterValue(pContext, 0, pInstruction->u.MOV_ARGS.Operand1 & 0x7, value, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}
	else if (pInstruction->u.MOV_ARGS.MI && pInstruction->HasIndirectAdressing)
	{
		ULONGLONG value = pInstruction->u.MOV_ARGS.Operand2;
		ULONGLONG dest;

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.MOV_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.MOV_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.MOV_ARGS.Operand1, &dest);

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}

		memcpy((UCHAR*)dest, &value, ucSize);
	}
	else if (pInstruction->u.MOV_ARGS.FD)
	{
		ULONGLONG value;
		ULONGLONG source = pInstruction->u.MOV_ARGS.Operand2;

		memcpy(&value, (UCHAR*)source, ucSize);

		SET_REGISTER(pContext, A, value, ucSize);
	}
	else if (pInstruction->u.MOV_ARGS.TD)
	{
		ULONGLONG value = GET_STATE(pContext)->A.RVALUE;
		ULONGLONG dest = pInstruction->u.MOV_ARGS.Operand2;
		

		memcpy((UCHAR*)dest, &value, ucSize);
	}
	else if (pInstruction->u.MOV_ARGS.RM)
	{
		ULONGLONG value;

		ucSize = 4;

		if (pInstruction->OperandOverride == 1)
		{
			ucSize = 2;
		}
		else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
		{
			ucSize = 8;
		}

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.MOV_ARGS.Operand1 & 0x7, &value);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.MOV_ARGS.Operand1 & 0x7, &value);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = value;

			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.MOV_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&value, (UCHAR*)dest, ucSize);
		}

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = SetRegisterValue(pContext, 1, (pInstruction->u.MOV_ARGS.Operand1 >> 3) & 0x7, value, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = SetRegisterValue(pContext, 0, (pInstruction->u.MOV_ARGS.Operand1 >> 3) & 0x7, value, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}
	else if (pInstruction->u.MOV_ARGS.MR)
	{
		ULONGLONG value;

		ucSize = 4;

		if (pInstruction->OperandOverride == 1)
		{
			ucSize = 2;
		}
		else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
		{
			ucSize = 8;
		}

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.MOV_ARGS.Operand1 >> 3) & 0x7, &value);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.MOV_ARGS.Operand1 >> 3) & 0x7, &value);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest;

			if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
			{
				ret = GetRegisterValue(pContext, 1, pInstruction->u.MOV_ARGS.Operand1 & 0x7, &dest);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
			else
			{
				ret = GetRegisterValue(pContext, 0, pInstruction->u.MOV_ARGS.Operand1 & 0x7, &dest);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}

			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.MOV_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy((UCHAR*)dest, &value, ucSize);
		}
		else
		{
			if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
			{
				ret = SetRegisterValue(pContext, 1, pInstruction->u.MOV_ARGS.Operand1 & 0x7, value, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
			else
			{
				ret = SetRegisterValue(pContext, 0, pInstruction->u.MOV_ARGS.Operand1 & 0x7, value, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
		}
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_SHIFT(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	UCHAR ucSize = 4;
	UCHAR reg;
	ULONGLONG value = 0;
	UCHAR shiftCount = 1;
	ULONGLONG mask;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	reg = (pInstruction->u.SHIFT_ARGS.Operand1 >> 3) & 0x7;

	if (pInstruction->OperandOverride == 1)
	{
		ucSize = 2;
	}
	else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
	{
		ucSize = 8;
	}

	if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
	{
		ret = GetRegisterValue(pContext, 1, pInstruction->u.SHIFT_ARGS.Operand1 & 0x7, &value);

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}
	}
	else
	{
		ret = GetRegisterValue(pContext, 0, pInstruction->u.SHIFT_ARGS.Operand1 & 0x7, &value);

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}
	}

	if (pInstruction->HasIndirectAdressing)
	{
		ULONGLONG dest = value;
		ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.SHIFT_ARGS.Operand1, &dest);

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}

		memcpy(&value, (UCHAR*)dest, ucSize);
	}

	if (pInstruction->u.SHIFT_ARGS.MI)
	{
		shiftCount = pInstruction->u.SHIFT_ARGS.Operand2;
	}
	else if (pInstruction->u.SHIFT_ARGS.MC)
	{
		shiftCount = GET_STATE(pContext)->C.LVALUE;
	}

	mask = (ucSize == 2) ? 0x8000 : ((ucSize == 4) ? 0x80000000 : 0x8000000000000000);

	switch (reg)
	{
		case 4: //SAL, SHL		
			for (UCHAR i = 0; i < shiftCount; i++)
			{
				SET_CF(pContext, value & mask);
				value <<= 1;
			}

			if (shiftCount == 1)
			{
				if ((value & mask) != 0)
				{
					if (GET_CF(pContext) != 0)
					{
						SET_OF(pContext, 0);
					}
					else
					{
						SET_OF(pContext, 1);
					}
				}
				else
				{
					if (GET_CF(pContext) == 0)
					{
						SET_OF(pContext, 0);
					}
					else
					{
						SET_OF(pContext, 1);
					}
				}				
			}
			break;

		case 7: //SAR		
		case 5: //SHR
			{
				ULONGLONG orgValue = value;

				for (UCHAR i = 0; i < shiftCount; i++)
				{
					SET_CF(pContext, value & 1);
					value >>= 1;
					value &= ~mask;

					if (reg == 7) //SAR
					{
						if ((value & (mask >> 1)) != 0)
						{
							value |= mask;
						}
					}
				}

				if (shiftCount == 1)
				{
					if (reg == 7) //SAR
					{
						SET_OF(pContext, 0);
					}
					else
					{
						SET_OF(pContext, orgValue & mask);
					}
				}
			}
			break;
	}

	if (shiftCount > 0)
	{
		UpdateFlags(pContext, value, ucSize);
	}

	if (pInstruction->HasIndirectAdressing)
	{
		ULONGLONG dest;

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.SHIFT_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.SHIFT_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.SHIFT_ARGS.Operand1, &dest);

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}

		memcpy((UCHAR*)dest, &value, ucSize);
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = SetRegisterValue(pContext, 1, pInstruction->u.SHIFT_ARGS.Operand1 & 0x7, value, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = SetRegisterValue(pContext, 0, pInstruction->u.SHIFT_ARGS.Operand1 & 0x7, value, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_ADD(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	ULONGLONG valueLeft = 0;
	ULONGLONG valueRight = 0;
	UCHAR ucSize = 4;
	ULONGLONG bits;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (pInstruction->OperandOverride == 1)
	{
		ucSize = 2;
	}
	else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
	{
		ucSize = 8;
	}

	bits = (ucSize == 2) ? 0xFFFF : ((ucSize == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF);

	if (pInstruction->u.ADD_ARGS.I)
	{
		ret = GetRegisterValue(pContext, 0, 0, &valueLeft); //A Register

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}
	}
	else if (pInstruction->u.ADD_ARGS.MR || pInstruction->u.ADD_ARGS.MI)
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.ADD_ARGS.Operand1 & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.ADD_ARGS.Operand1 & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = valueLeft;
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.ADD_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&valueLeft, (UCHAR*)dest, ucSize);
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.ADD_ARGS.Operand1 >> 3) & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.ADD_ARGS.Operand1 >> 3) & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}

	if (pInstruction->u.ADD_ARGS.I || pInstruction->u.ADD_ARGS.MI)
	{
		valueRight = pInstruction->u.ADD_ARGS.Operand2;
	}
	else if (pInstruction->u.ADD_ARGS.MR)
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.ADD_ARGS.Operand1 >> 3) & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.ADD_ARGS.Operand1 >> 3) & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.ADD_ARGS.Operand1 & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.ADD_ARGS.Operand1 & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = valueRight;
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.ADD_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&valueRight, (UCHAR*)dest, ucSize);
		}
	}

	valueLeft &= bits;
	valueRight &= bits;

	valueLeft = AddValues(pContext, valueLeft, valueRight, ucSize);

	if (pInstruction->u.ADD_ARGS.I)
	{
		ret = SetRegisterValue(pContext, 0, 0, valueLeft, ucSize); //A Register

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}
	}
	else if (pInstruction->u.ADD_ARGS.MR || pInstruction->u.ADD_ARGS.MI)
	{
		ULONGLONG dest;

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.ADD_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.ADD_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{

			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.ADD_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy((UCHAR*)dest, &valueLeft, ucSize);
		}
		else
		{
			if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
			{
				ret = SetRegisterValue(pContext, 1, pInstruction->u.ADD_ARGS.Operand1 & 0x7, valueLeft, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
			else
			{
				ret = SetRegisterValue(pContext, 0, pInstruction->u.ADD_ARGS.Operand1 & 0x7, valueLeft, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = SetRegisterValue(pContext, 1, (pInstruction->u.ADD_ARGS.Operand1 >> 3) & 0x7, valueLeft, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = SetRegisterValue(pContext, 0, (pInstruction->u.ADD_ARGS.Operand1 >> 3) & 0x7, valueLeft, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_SUB(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	ULONGLONG valueLeft = 0;
	ULONGLONG valueRight = 0;
	UCHAR ucSize = 4;
	ULONGLONG bits;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (pInstruction->OperandOverride == 1)
	{
		ucSize = 2;
	}
	else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
	{
		ucSize = 8;
	}

	bits = (ucSize == 2) ? 0xFFFF : ((ucSize == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF);

	if (pInstruction->u.SUB_ARGS.I)
	{
		ret = GetRegisterValue(pContext, 0, 0, &valueLeft); //A Register

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}
	}
	else if (pInstruction->u.SUB_ARGS.MR || pInstruction->u.SUB_ARGS.MI)
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.SUB_ARGS.Operand1 & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.SUB_ARGS.Operand1 & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = valueLeft;
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.SUB_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&valueLeft, (UCHAR*)dest, ucSize);
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.SUB_ARGS.Operand1 >> 3) & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.SUB_ARGS.Operand1 >> 3) & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}

	if (pInstruction->u.SUB_ARGS.I || pInstruction->u.SUB_ARGS.MI)
	{
		valueRight = pInstruction->u.SUB_ARGS.Operand2;
	}
	else if (pInstruction->u.SUB_ARGS.MR)
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.SUB_ARGS.Operand1 >> 3) & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.SUB_ARGS.Operand1 >> 3) & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.SUB_ARGS.Operand1 & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.SUB_ARGS.Operand1 & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = valueRight;
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.SUB_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&valueRight, (UCHAR*)dest, ucSize);
		}
	}

	valueLeft &= bits;
	
	//create Komplement
	valueRight = (~valueRight) & bits;
	valueRight++;

	valueLeft = AddValues(pContext, valueLeft, valueRight, ucSize);

	if (pInstruction->u.SUB_ARGS.I)
	{
		ret = SetRegisterValue(pContext, 0, 0, valueLeft, ucSize); //A Register

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}
	}
	else if (pInstruction->u.SUB_ARGS.MR || pInstruction->u.SUB_ARGS.MI)
	{
		ULONGLONG dest;

		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.SUB_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.SUB_ARGS.Operand1 & 0x7, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.SUB_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy((UCHAR*)dest, &valueLeft, ucSize);
		}
		else
		{
			if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
			{
				ret = SetRegisterValue(pContext, 1, pInstruction->u.SUB_ARGS.Operand1 & 0x7, valueLeft, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
			else
			{
				ret = SetRegisterValue(pContext, 0, pInstruction->u.SUB_ARGS.Operand1 & 0x7, valueLeft, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = SetRegisterValue(pContext, 1, (pInstruction->u.SUB_ARGS.Operand1 >> 3) & 0x7, valueLeft, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = SetRegisterValue(pContext, 0, (pInstruction->u.SUB_ARGS.Operand1 >> 3) & 0x7, valueLeft, ucSize);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_CLI(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	GET_STATE(pContext)->RFLAGS &= ~(IF_MASK);

	return ret;
}

eInterpreterReturn ExecuteInstruction_STI(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	GET_STATE(pContext)->RFLAGS |= IF_MASK;

	return ret;
}

eInterpreterReturn ExecuteInstruction_OUT(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	PHYSICAL_ADDRESS stackPointer;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	stackPointer.QuadPart = (LONGLONG)STACK_TOP(pContext);

	//call out assembler function
	CallOUT(stackPointer, GET_STATE(pContext), (pInstruction->u.OUT_ARGS.WithOperand) ? pInstruction->u.OUT_ARGS.Operand : 0xFFFFFFFF);

	if (ret != INTERPRETER_OK)
	{
		return ret;
	}

	//  some PC's GABI entry point has a loop to call out opcode to wait SMI, 
	//  and it falls into an infinite loop.
	//  Now, interpreter out asm code already waits for SMI and 
	//  such loop is unnecessary.
	//  Interpreter will stop execution here.
	return INTERPRETER_SEGMENT_OK;
}

eInterpreterReturn ExecuteInstruction_CMP(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	ULONGLONG valueLeft = 0;
	ULONGLONG valueRight = 0;
	UCHAR ucSize = 4;
	ULONGLONG bits;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (pInstruction->OperandOverride == 1)
	{
		ucSize = 2;
	}
	else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
	{
		ucSize = 8;
	}

	bits = (ucSize == 2) ? 0xFFFF : ((ucSize == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF);

	if (pInstruction->u.CMP_ARGS.I)
	{
		ret = GetRegisterValue(pContext, 0, 0, &valueLeft); //A Register

		if (ret != INTERPRETER_OK)
		{
			return ret;
		}		
	}
	else if(pInstruction->u.CMP_ARGS.MR || pInstruction->u.CMP_ARGS.MI)
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.CMP_ARGS.Operand1 & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.CMP_ARGS.Operand1 & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = valueLeft;
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.CMP_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&valueLeft, (UCHAR*)dest, ucSize);
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.CMP_ARGS.Operand1 >> 3) & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.CMP_ARGS.Operand1 >> 3) & 0x7, &valueLeft);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}

	if (pInstruction->u.CMP_ARGS.I || pInstruction->u.CMP_ARGS.MI)
	{
		valueRight = pInstruction->u.CMP_ARGS.Operand2;
	}
	else if (pInstruction->u.CMP_ARGS.MR)
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXR_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, (pInstruction->u.CMP_ARGS.Operand1 >> 3) & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, (pInstruction->u.CMP_ARGS.Operand1 >> 3) & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
	}
	else
	{
		if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
		{
			ret = GetRegisterValue(pContext, 1, pInstruction->u.CMP_ARGS.Operand1 & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}
		else
		{
			ret = GetRegisterValue(pContext, 0, pInstruction->u.CMP_ARGS.Operand1 & 0x7, &valueRight);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}
		}

		if (pInstruction->HasIndirectAdressing)
		{
			ULONGLONG dest = valueRight;
			ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.CMP_ARGS.Operand1, &dest);

			if (ret != INTERPRETER_OK)
			{
				return ret;
			}

			memcpy(&valueRight, (UCHAR*)dest, ucSize);
		}
	}

	valueLeft &= bits;

	//create Komplement
	valueRight = (~valueRight) & bits;
	valueRight++;

	AddValues(pContext, valueLeft, valueRight, ucSize);

	return ret;
}

eInterpreterReturn ExecuteInstruction_JUMP(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	UCHAR ucRIPChanged = 0;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (pInstruction->u.JUMP_ARGS.JMP)
	{
		if (pInstruction->u.JUMP_ARGS.Absolute != 0)
		{
			ret = INTERPRETER_E_UNKNOWN_INSTRUCTION;
		}
		else
		{
			const ULONGLONG mask = (pInstruction->u.JUMP_ARGS.Size == 1) ? 0x80 : ((pInstruction->u.JUMP_ARGS.Size == 2) ? 0x8000 : ((pInstruction->u.JUMP_ARGS.Size == 4) ? 0x80000000 : 0x8000000000000000));
			const ULONGLONG bits = (pInstruction->u.JUMP_ARGS.Size == 1) ? 0xFF : ((pInstruction->u.JUMP_ARGS.Size == 2) ? 0xFFFF : ((pInstruction->u.JUMP_ARGS.Size == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF));
			ULONGLONG offset = ((pInstruction->u.JUMP_ARGS.Operand & mask) != 0) ? ~bits : 0;
			offset |= (pInstruction->u.JUMP_ARGS.Operand & bits);

			//ucRIPChanged = 1 not neccessary because this is an relative jump
			GET_STATE(pContext)->RIP += (LONGLONG)offset;
		}
	}
	else
	{
		//jump with condition
		UCHAR ucDoJump = 0;

		if (pInstruction->u.JUMP_ARGS.JA)
		{
			ucDoJump = (GET_CF(pContext) == 0 && GET_ZF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JBE)
		{
			ucDoJump = (GET_CF(pContext) != 0 || GET_ZF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JC)
		{
			ucDoJump = (GET_CF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JCXZ)
		{
			UCHAR ucSize = 4;
			ULONGLONG bits;

			if (pInstruction->OperandOverride == 1)
			{
				ucSize = 2;
			}
			else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
			{
				ucSize = 8;
			}

			bits = (ucSize == 2) ? 0xFFFF : ((ucSize == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF);

			ucDoJump = ((GET_STATE(pContext)->C.RVALUE & bits) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JE)
		{
			ucDoJump = (GET_ZF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JG)
		{
			ucDoJump = (GET_ZF(pContext) == 0 && GET_SF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JGE)
		{
			ucDoJump = (GET_SF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JL)
		{
			ucDoJump = (GET_SF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JNC)
		{
			ucDoJump = (GET_CF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JNG)
		{
			ucDoJump = (GET_ZF(pContext) != 0 || GET_SF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JNO)
		{
			ucDoJump = (GET_OF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JNP)
		{
			ucDoJump = (GET_PF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JNS)
		{
			ucDoJump = (GET_SF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JNZ)
		{
			ucDoJump = (GET_ZF(pContext) == 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JO)
		{
			ucDoJump = (GET_OF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JP)
		{
			ucDoJump = (GET_PF(pContext) != 0) ? 1 : 0;
		}
		else if (pInstruction->u.JUMP_ARGS.JS)
		{
			ucDoJump = (GET_SF(pContext) != 0) ? 1 : 0;
		}

		if (ucDoJump)
		{
			const ULONGLONG mask = (pInstruction->u.JUMP_ARGS.Size == 1) ? 0x80 : ((pInstruction->u.JUMP_ARGS.Size == 2) ? 0x8000 : ((pInstruction->u.JUMP_ARGS.Size == 4) ? 0x80000000 : 0x8000000000000000));
			const ULONGLONG bits = (pInstruction->u.JUMP_ARGS.Size == 1) ? 0xFF : ((pInstruction->u.JUMP_ARGS.Size == 2) ? 0xFFFF : ((pInstruction->u.JUMP_ARGS.Size == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF));
			ULONGLONG offset = ((pInstruction->u.JUMP_ARGS.Operand & mask) != 0) ? ~bits : 0;
			offset |= (pInstruction->u.JUMP_ARGS.Operand & bits);

			//ucRIPChanged = 1 not neccessary because this is an relative jump
			GET_STATE(pContext)->RIP += (LONGLONG)offset;
		}
	}

	if (ret == INTERPRETER_OK && ucRIPChanged != 0)
	{
		return INTERPRETER_SEGMENT_OK;
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_PUSH(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	UCHAR ucSize;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (GET_STATE(pContext)->SP.RVALUE <= sizeof(sInterpreterCPUState) || GET_STATE(pContext)->SP.RVALUE > STACK_SIZE)
	{
		return INTERPRETER_E_STACK_ERROR;
	}

	if (pInstruction->u.PUSH_ARGS.A != 0)
	{
		ULONGLONG currentSP = GET_STATE(pContext)->SP.RVALUE;

		if (pContext->ucAddressingMode == INTERPRETER_64BIT)
		{
			return INTERPRETER_E_INVALID_INSTRUCTION;
		}
		else
		{
			if (pInstruction->OperandOverride)
			{
				ucSize = 2;
			}
			else
			{
				ucSize = 4;
			}
		}

		for (int i = 0; i < 8; i++)
		{
			ULONGLONG value;

			switch (i)
			{
				case 4:
					value = currentSP;
					break;

				default:
					ret = GetRegisterValue(pContext, 0, (UCHAR)i, &value);

					if (ret != INTERPRETER_OK)
					{
						return ret;
					}
					break;
			}

			GET_STATE(pContext)->SP.RVALUE -= ucSize;

			memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);
		}
	}
	else if (pInstruction->u.PUSH_ARGS.F != 0)
	{
		if (pContext->ucAddressingMode == INTERPRETER_64BIT)
		{
			if (pInstruction->OperandOverride)
			{
				ucSize = 2;
			}
			else
			{
				ucSize = 8;
			}
		}
		else
		{
			if (pInstruction->OperandOverride)
			{
				ucSize = 2;
			}
			else
			{
				ucSize = 4;
			}
		}

		GET_STATE(pContext)->SP.RVALUE -= ucSize;

		memcpy(STACK_TOP(pContext), (UCHAR*)&(GET_STATE(pContext)->RFLAGS), ucSize);
	}
	else
	{
		ULONGLONG value = 0;
		ucSize = 4;

		if (pInstruction->OperandOverride)
		{
			ucSize = 2;
		}
		else if (pContext->ucAddressingMode == INTERPRETER_64BIT)
		{
			ucSize = 8;
		}

		if (pInstruction->u.PUSH_ARGS.DS)
		{
			//value = GET_STATE(pContext)->DS;
		}
		else if (pInstruction->u.PUSH_ARGS.ES)
		{
			//value = GET_STATE(pContext)->ES;
		}
		else if (pInstruction->u.PUSH_ARGS.SS)
		{
			//value = GET_STATE(pContext)->SS;
		}
		else if (pInstruction->u.PUSH_ARGS.FS)
		{
			//value = GET_STATE(pContext)->FS;
		}
		else if (pInstruction->u.PUSH_ARGS.GS)
		{
			//value = GET_STATE(pContext)->GS;
		}
		else if (pInstruction->u.PUSH_ARGS.IOperand)
		{
			const ULONGLONG mask = (pInstruction->u.PUSH_ARGS.Size == 8) ? 0xFF : ((pInstruction->u.PUSH_ARGS.Size == 16) ? 0xFFFF : ((pInstruction->u.PUSH_ARGS.Size == 32) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF));
			value = pInstruction->u.PUSH_ARGS.Operand & mask;
		}
		else if (pInstruction->u.PUSH_ARGS.OOperand || pInstruction->u.PUSH_ARGS.MOperand)
		{
			if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
			{
				ret = GetRegisterValue(pContext, 1, pInstruction->u.PUSH_ARGS.Register, &value);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
			else
			{
				ret = GetRegisterValue(pContext, 0, pInstruction->u.PUSH_ARGS.Register, &value);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}

			if (pInstruction->u.PUSH_ARGS.MOperand && pInstruction->HasIndirectAdressing)
			{
				ULONGLONG dest = value;
				ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.PUSH_ARGS.Register, &dest);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}

				memcpy(&value, (UCHAR*)dest, ucSize);
			}
		}

		GET_STATE(pContext)->SP.RVALUE -= ucSize;

		memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);
	}

	if (GET_STATE(pContext)->SP.RVALUE <= sizeof(sInterpreterCPUState) || GET_STATE(pContext)->SP.RVALUE > STACK_SIZE)
	{
		return INTERPRETER_E_STACK_ERROR;
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_POP(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	UCHAR ucSize;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (GET_STATE(pContext)->SP.RVALUE >= STACK_SIZE)
	{
		return INTERPRETER_E_STACK_ERROR;
	}

	if (pInstruction->u.POP_ARGS.A != 0)
	{
		if (pContext->ucAddressingMode == INTERPRETER_64BIT)
		{
			return INTERPRETER_E_INVALID_INSTRUCTION;
		}
		else
		{
			if (pInstruction->OperandOverride)
			{
				ucSize = 2;
			}
			else
			{
				ucSize = 4;
			}
		}

		for (int i = 7; i >= 0; i--)
		{
			ULONGLONG value = 0;

			memcpy((UCHAR*)&value, STACK_TOP(pContext), ucSize);

			GET_STATE(pContext)->SP.RVALUE += ucSize;

			switch (i)
			{
				case 4:
					continue; //SP is ignored

				default:
					ret = SetRegisterValue(pContext, 0, (UCHAR)i, value, ucSize);

					if (ret != INTERPRETER_OK)
					{
						return ret;
					}
					break;
			}
		}
	}
	else if (pInstruction->u.POP_ARGS.F != 0)
	{
		if (pContext->ucAddressingMode == INTERPRETER_64BIT)
		{
			if (pInstruction->OperandOverride)
			{
				ucSize = 2;
			}
			else
			{
				ucSize = 8;
			}
		}
		else
		{
			if (pInstruction->OperandOverride)
			{
				ucSize = 2;
			}
			else
			{
				ucSize = 4;
			}
		}

		memcpy((UCHAR*)&(GET_STATE(pContext)->RFLAGS), STACK_TOP(pContext), ucSize);

		GET_STATE(pContext)->SP.RVALUE += ucSize;
	}
	else
	{
		ULONGLONG value = 0;
		ucSize = 4;

		if (pInstruction->OperandOverride)
		{
			ucSize = 2;
		}
		else if (pContext->ucAddressingMode == INTERPRETER_64BIT)
		{
			ucSize = 8;
		}

		memcpy((UCHAR*)&(value), STACK_TOP(pContext), ucSize);

		GET_STATE(pContext)->SP.RVALUE += ucSize;

		if (pInstruction->u.POP_ARGS.DS)
		{
			//SET_REGISTER(pContext, DS, value, ucSize);
		}
		else if (pInstruction->u.POP_ARGS.ES)
		{
			//SET_REGISTER(pContext, ES, value, ucSize);
		}
		else if (pInstruction->u.POP_ARGS.SS)
		{
			//SET_REGISTER(pContext, SS, value, ucSize);
		}
		else if (pInstruction->u.POP_ARGS.FS)
		{
			//SET_REGISTER(pContext, FS, value, ucSize);
		}
		else if (pInstruction->u.POP_ARGS.GS)
		{
			//SET_REGISTER(pContext, GS, value, ucSize);
		}
		else if (pInstruction->u.POP_ARGS.OOperand || pInstruction->u.POP_ARGS.MOperand)
		{
			if (pInstruction->u.POP_ARGS.MOperand && pInstruction->HasIndirectAdressing)
			{
				ULONGLONG dest;

				if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
				{
					ret = GetRegisterValue(pContext, 1, pInstruction->u.POP_ARGS.Register, &dest);

					if (ret != INTERPRETER_OK)
					{
						return ret;
					}
				}
				else
				{
					ret = GetRegisterValue(pContext, 0, pInstruction->u.POP_ARGS.Register, &dest);

					if (ret != INTERPRETER_OK)
					{
						return ret;
					}
				}

				ret = GetIndirectAddressing(pContext, pInstruction, pInstruction->u.POP_ARGS.Register, &dest);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}

				memcpy((UCHAR*)dest, &value, ucSize);
			}
			else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXB_MASK) != 0)
			{
				ret = SetRegisterValue(pContext, 1, pInstruction->u.POP_ARGS.Register, value, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
			else
			{
				ret = SetRegisterValue(pContext, 0, pInstruction->u.POP_ARGS.Register, value, ucSize);

				if (ret != INTERPRETER_OK)
				{
					return ret;
				}
			}
		}
	}

	if (GET_STATE(pContext)->SP.RVALUE > STACK_SIZE)
	{
		return INTERPRETER_E_STACK_ERROR;
	}

	return ret;
}

eInterpreterReturn ExecuteInstruction_RET(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
#if defined(_AMD64_) || defined(_IA64_)
	const UCHAR ucSize = 8;
#else
	const UCHAR ucSize = 4;
#endif
	ULONGLONG value;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	memcpy((UCHAR*)&value, STACK_TOP(pContext), ucSize);
	GET_STATE(pContext)->SP.RVALUE += ucSize;

	if (value != 0)
	{
		GET_STATE(pContext)->RIP = value;
		return INTERPRETER_SEGMENT_OK;
	}

	//program finished => no execution
	return ret;
}

eInterpreterReturn ExecuteInstruction_CALL(psInterpreterContext pContext, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	ULONGLONG value;
	UCHAR ucRIPChanged = 0;
	UCHAR ucSize = (pContext->ucAddressingMode == INTERPRETER_64BIT) ? 8 : 4;

	if (pContext == NULL || pInstruction == NULL || pContext->pInterpreterState == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	//add current IP
	value = GET_STATE(pContext)->RIP;
	value += pInstruction->Len; //next command after call
	GET_STATE(pContext)->SP.RVALUE -= ucSize;
	memcpy(STACK_TOP(pContext), (UCHAR*)&value, ucSize);

	if (pInstruction->u.CALL_ARGS.Absolute != 0)
	{
		return INTERPRETER_E_UNKNOWN_INSTRUCTION;
	}
	else
	{
		const ULONGLONG mask = (pInstruction->u.CALL_ARGS.Size == 1) ? 0x80 : ((pInstruction->u.CALL_ARGS.Size == 2) ? 0x8000 : ((pInstruction->u.CALL_ARGS.Size == 4) ? 0x80000000 : 0x8000000000000000));
		const ULONGLONG bits = (pInstruction->u.CALL_ARGS.Size == 1) ? 0xFF : ((pInstruction->u.CALL_ARGS.Size == 2) ? 0xFFFF : ((pInstruction->u.CALL_ARGS.Size == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF));
		ULONGLONG offset = ((pInstruction->u.CALL_ARGS.Offset & mask) != 0) ? ~bits : 0;
		offset |= (pInstruction->u.CALL_ARGS.Offset & bits);

		//ucRIPChanged = 1 not neccessary because this is an relative jump
		GET_STATE(pContext)->RIP += (LONGLONG)offset;
	}

	if (ret == INTERPRETER_OK && ucRIPChanged != 0)
	{
		return INTERPRETER_SEGMENT_OK;
	}

	return ret;
}

eInterpreterReturn GetIndirectAddressing(psInterpreterContext pContext, psInstruction pInstruction, UCHAR ucRM, ULONGLONG* pAddress)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	if (pInstruction->HasIndirectAdressing == 0 || pInstruction->Mod == 0x3)
	{
		return ret;
	}

	if (pContext->ucAddressingMode == INTERPRETER_64BIT || (pInstruction->AddressOverride == 1))
	{
		//32/64Bit addressing
		switch (ucRM)
		{
			case 4:
				if (pInstruction->HasSibByte)
				{
					UCHAR scale = (pInstruction->SibByte >> 6) & 0x3;
					UCHAR index = (pInstruction->SibByte >> 3) & 0x7;
					UCHAR base = (pInstruction->SibByte) & 0x7;

					switch (scale)
					{
						case 0:
							scale = 1;
							break;

						case 1:
							scale = 2;
							break;

						case 2:
							scale = 4;
							break;

						case 4:
							scale = 8;
							break;
					}

					if (pInstruction->HasRexExtension)
					{
						if((pInstruction->RexExtension & REXX_MASK) != 0)
						{
							index |= 0x8;
						}

						if ((pInstruction->RexExtension & REXB_MASK) != 0)
						{
							base |= 0x8;
						}
					}
					
					switch (base)
					{
						case 0x5:
						case 0xD:
							if (pInstruction->Mod == 0)
							{
								if (index != 0x4)
								{
									ret = GetRegisterValue(pContext, 0, index, pAddress);

									if (ret != INTERPRETER_OK)
									{
										return ret;
									}

									*pAddress = (*pAddress) * scale;
								}
								else
								{
									*pAddress = 0;
								}
								
								break;
							}
							//else path: continue with default

						default:
							{
								if(index != 0x4)
								{
									ULONGLONG tmp;

									ret = GetRegisterValue(pContext, 0, base, &tmp);

									if (ret != INTERPRETER_OK)
									{
										return ret;
									}

									ret = GetRegisterValue(pContext, 0, index, pAddress);

									if (ret != INTERPRETER_OK)
									{
										return ret;
									}

									*pAddress = (*pAddress) * scale + tmp;
								}
								else
								{
									ret = GetRegisterValue(pContext, 0, base, pAddress);

									if (ret != INTERPRETER_OK)
									{
										return ret;
									}
								}
							}
							break;
					}
				}
				break;

			case 5:
				if (pInstruction->Mod == 0)
				{
					*pAddress = (ULONGLONG)((UCHAR*)GET_STATE(pContext) + GET_STATE(pContext)->RIP + pInstruction->Displacement);
				}
				break;

			default:
				break;
		}		
	}
	else
	{
		//16Bit addressing
		switch (ucRM)
		{
			case 0:
				*pAddress = GET_STATE(pContext)->B.XVALUE + GET_STATE(pContext)->SI.XVALUE;
				break;

			case 1:
				*pAddress = GET_STATE(pContext)->B.XVALUE + GET_STATE(pContext)->DI.XVALUE;
				break;

			case 2:
				*pAddress = GET_STATE(pContext)->BP.XVALUE + GET_STATE(pContext)->SI.XVALUE;
				break;

			case 3:
				*pAddress = GET_STATE(pContext)->BP.XVALUE + GET_STATE(pContext)->DI.XVALUE;
				break;

			case 4:
				*pAddress = GET_STATE(pContext)->SI.XVALUE;
				break;

			case 5:
				*pAddress = GET_STATE(pContext)->DI.XVALUE;
				break;

			case 6:
				*pAddress = pInstruction->Displacement;
				break;

			case 7:
				*pAddress = GET_STATE(pContext)->B.XVALUE;
				break;
		}
	}

	if (pInstruction->Mod == 0x1 || pInstruction->Mod == 0x2)
	{
		*pAddress = (*pAddress) + pInstruction->Displacement;
	}

	return ret;
}

eInterpreterReturn GetRegisterValue(psInterpreterContext pContext, UCHAR rex, UCHAR regIndex, ULONGLONG* pValue)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	if (regIndex >= 8)
	{
		rex = 1;
		regIndex &= 0x7;
	}

	if (rex != 0)
	{
		switch (regIndex)
		{
			case 0:
				*pValue = GET_STATE(pContext)->R8.RVALUE;
				break;

			case 1:
				*pValue = GET_STATE(pContext)->R9.RVALUE;
				break;

			case 2:
				*pValue = GET_STATE(pContext)->R10.RVALUE;
				break;

			case 3:
				*pValue = GET_STATE(pContext)->R11.RVALUE;
				break;

			case 4:
				*pValue = GET_STATE(pContext)->R12.RVALUE;
				break;

			case 5:
				*pValue = GET_STATE(pContext)->R13.RVALUE;
				break;

			case 6:
				*pValue = GET_STATE(pContext)->R14.RVALUE;
				break;

			case 7:
				*pValue = GET_STATE(pContext)->R15.RVALUE;
				break;
		}
	}
	else
	{
		switch (regIndex)
		{
			case 0:
				*pValue = GET_STATE(pContext)->A.RVALUE;
				break;

			case 1:
				*pValue = GET_STATE(pContext)->C.RVALUE;
				break;

			case 2:
				*pValue = GET_STATE(pContext)->D.RVALUE;
				break;

			case 3:
				*pValue = GET_STATE(pContext)->B.RVALUE;
				break;

			case 4:
				*pValue = GET_STATE(pContext)->SP.RVALUE + (ULONGLONG)GET_STATE(pContext);
				break;

			case 5:
				*pValue = GET_STATE(pContext)->BP.RVALUE;
				break;

			case 6:
				*pValue = GET_STATE(pContext)->SI.RVALUE;
				break;

			case 7:
				*pValue = GET_STATE(pContext)->DI.RVALUE;
				break;
		}
	}

	return ret;
}

eInterpreterReturn SetRegisterValue(psInterpreterContext pContext, UCHAR rex, UCHAR regIndex, ULONGLONG value, UCHAR ucSize)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	if (regIndex >= 8)
	{
		rex = 1;
		regIndex &= 0x7;
	}

	if (rex != 0)
	{
		switch (regIndex)
		{
			case 0:
				SET_REGISTER(pContext, R8, value, ucSize);
				break;

			case 1:
				SET_REGISTER(pContext, R9, value, ucSize);
				break;

			case 2:
				SET_REGISTER(pContext, R10, value, ucSize);
				break;

			case 3:
				SET_REGISTER(pContext, R11, value, ucSize);
				break;

			case 4:
				SET_REGISTER(pContext, R12, value, ucSize);
				break;

			case 5:
				SET_REGISTER(pContext, R13, value, ucSize);
				break;

			case 6:
				SET_REGISTER(pContext, R14, value, ucSize);
				break;

			case 7:
				SET_REGISTER(pContext, R15, value, ucSize);
				break;
		}
	}
	else
	{
		switch (regIndex)
		{
			case 0:
				SET_REGISTER(pContext, A, value, ucSize);
				break;

			case 1:
				SET_REGISTER(pContext, C, value, ucSize);
				break;

			case 2:
				SET_REGISTER(pContext, D, value, ucSize);
				break;

			case 3:
				SET_REGISTER(pContext, B, value, ucSize);
				break;

			case 4:
				{
					ULONGLONG pStart = (ULONGLONG)GET_STATE(pContext);
					ULONGLONG pEnd = pStart + STACK_SIZE;

					if (pStart <= value && pEnd >= value)
					{
						value -= pStart;
						SET_REGISTER(pContext, SP, value, ucSize); 
					}
					else
					{
						return INTERPRETER_E_STACK_ERROR;
					}
				}
				break;

			case 5:
				SET_REGISTER(pContext, BP, value, ucSize);
				break;

			case 6:
				SET_REGISTER(pContext, SI, value, ucSize);
				break;

			case 7:
				SET_REGISTER(pContext, DI, value, ucSize);
				break;
		}
	}

	return ret;
}

void UpdateFlags(psInterpreterContext pContext, ULONGLONG value, UCHAR ucSize)
{
	UCHAR nOfSetBits = 0;
	const ULONGLONG bits = (ucSize == 2) ? 0xFFFF : ((ucSize == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF);
	const ULONGLONG mask = (ucSize == 2) ? 0x8000 : ((ucSize == 4) ? 0x80000000 : 0x8000000000000000);

	//update flags SF, ZF and PF
	SET_SF(pContext, value & mask);
	SET_ZF(pContext, value & bits);

	for (UCHAR i = 0; i < ucSize * 8; i++)
	{
		if ((value >> i) & 1)
		{
			nOfSetBits++;
		}
	}

	if ((nOfSetBits & 1) == 1)
	{
		SET_PF(pContext, 0);
	}
	else
	{
		SET_PF(pContext, 1);
	}
}

ULONGLONG AddValues(psInterpreterContext pContext, ULONGLONG left, ULONGLONG right, UCHAR ucSize)
{
	const ULONGLONG mask = (ucSize == 2) ? 0x8000 : ((ucSize == 4) ? 0x80000000 : 0x8000000000000000);
	ULONGLONG ret = left + right;
	UCHAR ucRetNibble = (left & 0xF) + (right & 0xF);

	if (ucSize == 8)
	{
		SET_CF(pContext, ret < left);

		if (ret < left)
		{
			SET_OF(pContext, 1);
		}
		else
		{
			if (ret & mask)
			{
				if ((left & (mask >> 1)) == 0 && (right & (mask >> 1)) == 0)
				{
					SET_OF(pContext, 1);
				}
				else
				{
					SET_OF(pContext, 0);
				}
			}
			else
			{
				SET_OF(pContext, 0);
			}
		}
	}
	else
	{
		SET_CF(pContext, ret & (mask << 1));

		if (ret & (mask << 1))
		{
			SET_OF(pContext, 1);
		}
		else
		{
			if (ret & mask)
			{
				if ((left & (mask >> 1)) == 0 && (right & (mask >> 1)) == 0)
				{
					SET_OF(pContext, 1);
				}
				else
				{
					SET_OF(pContext, 0);
				}
			}
			else
			{
				SET_OF(pContext, 0);
			}
		}
	}

	SET_AF(pContext, ucRetNibble & 0x10);
	UpdateFlags(pContext, ret, ucSize);

	return ret;
}
