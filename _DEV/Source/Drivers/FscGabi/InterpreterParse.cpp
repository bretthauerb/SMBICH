#include <ntddk.h>
#include "Interpreter.h"
#include "InterpreterInternal.h"

eInterpreterReturn GetNextCommand(psInterpreterContext pContext, PUSHORT pwCurrentIndex, psInstruction pInstruction)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	UCHAR firstOpCode = 0;
	USHORT wStartIndex;

	if (pContext == NULL || pwCurrentIndex == NULL || pInstruction == NULL)
	{
		return INTERPRETER_E_ARGS;
	}

	if (pContext->wSize <= *pwCurrentIndex)
	{
		return INTERPRETER_E_EOF;
	}

	wStartIndex = *pwCurrentIndex;
	memset(pInstruction, 0, sizeof(sInstruction));

	while (pContext->wSize > *pwCurrentIndex)
	{
		UCHAR ucFinish = 0;
		USHORT opcode = firstOpCode;
		opcode = (opcode << 8) + pContext->pEntryPoint[*pwCurrentIndex];

		switch (opcode)
		{
			case 0x0F:
				//multi byte opcode
				firstOpCode = pContext->pEntryPoint[*pwCurrentIndex];
				(*pwCurrentIndex)++;
				break;

			case 0x66:
				//PREFIX
				(*pwCurrentIndex)++;
				pInstruction->OperandOverride = 1;
				break;

			case 0x67:
				//PREFIX
				(*pwCurrentIndex)++;
				pInstruction->AddressOverride = 1;
				break;

			case 0x9D:
				//POPF
			case 0x61:
				//POPA
				pInstruction->Type = INTERPRETER_POP;
				pInstruction->u.POP_ARGS.A = (pContext->pEntryPoint[*pwCurrentIndex] == 0x61);
				pInstruction->u.POP_ARGS.F = (pContext->pEntryPoint[*pwCurrentIndex] == 0x9D);
				ucFinish = 1;
				break;

			case 0x0FA1:
			case 0x0FA9:
			case 0x17:
			case 0x1F:
			case 0x07:
				//POP no parameters
				pInstruction->Type = INTERPRETER_POP;
				pInstruction->u.POP_ARGS.SS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x17);
				pInstruction->u.POP_ARGS.DS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x1F);
				pInstruction->u.POP_ARGS.ES = (pContext->pEntryPoint[*pwCurrentIndex] == 0x07);
				pInstruction->u.POP_ARGS.FS = (pContext->pEntryPoint[*pwCurrentIndex] == 0xA1);
				pInstruction->u.POP_ARGS.GS = (pContext->pEntryPoint[*pwCurrentIndex] == 0xA9);
				ucFinish = 1;
				break;

			case 0x58:
			case 0x59:
			case 0x5A:
			case 0x5B:
			case 0x5C:
			case 0x5D:
			case 0x5E:
			case 0x5F:
				//POP register with parameters
				pInstruction->u.POP_ARGS.OOperand = 1;
				pInstruction->u.POP_ARGS.Register = ((pContext->pEntryPoint[*pwCurrentIndex] & 0xF) - 8);
			case 0x8F:
				//POP with parameters
				pInstruction->u.POP_ARGS.MOperand = (pContext->pEntryPoint[*pwCurrentIndex] == 0x8F);
				pInstruction->Type = INTERPRETER_POP;

				if (pInstruction->u.PUSH_ARGS.OOperand == 0)
				{
					eInterpreterReturn tmp;

					(*pwCurrentIndex)++;

					if (pContext->wSize <= *pwCurrentIndex)
					{
						return INTERPRETER_E_EOF;
					}

					pInstruction->u.POP_ARGS.Operand = pContext->pEntryPoint[*pwCurrentIndex];

					if (pInstruction->u.POP_ARGS.MOperand == 1)
					{
						pInstruction->u.POP_ARGS.Register = pInstruction->u.POP_ARGS.Operand & 0x7;
					}

					tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, (UCHAR)pInstruction->u.POP_ARGS.Operand);

					if (tmp != INTERPRETER_OK)
					{
						return tmp;
					}
				}
				ucFinish = 1;
				break;

			case 0x9C:
				//PUSHF
			case 0x60:
				//PUSHA
				pInstruction->Type = INTERPRETER_PUSH;
				pInstruction->u.PUSH_ARGS.A = (pContext->pEntryPoint[*pwCurrentIndex] == 0x60);
				pInstruction->u.PUSH_ARGS.F = (pContext->pEntryPoint[*pwCurrentIndex] == 0x9C);
				ucFinish = 1;
				break;

			case 0x0FA0:
			case 0x0FA8:
			case 0x0E:
			case 0x16:
			case 0x1E:
			case 0x06:						
				//PUSH no parameters
				pInstruction->Type = INTERPRETER_PUSH;
				pInstruction->u.PUSH_ARGS.CS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x0E);
				pInstruction->u.PUSH_ARGS.SS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x16);
				pInstruction->u.PUSH_ARGS.DS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x1E);
				pInstruction->u.PUSH_ARGS.ES = (pContext->pEntryPoint[*pwCurrentIndex] == 0x06);
				pInstruction->u.PUSH_ARGS.FS = (pContext->pEntryPoint[*pwCurrentIndex] == 0xA0);
				pInstruction->u.PUSH_ARGS.GS = (pContext->pEntryPoint[*pwCurrentIndex] == 0xA8);
				ucFinish = 1;
				break;

			case 0x50:
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x56:
			case 0x57:
				//PUSH register with parameters
				pInstruction->u.PUSH_ARGS.OOperand = 1;
				pInstruction->u.PUSH_ARGS.Register = (pContext->pEntryPoint[*pwCurrentIndex] & 0xF);
			case 0x6A:
			case 0x68:
				//PUSH with parameters
				pInstruction->u.PUSH_ARGS.IOperand = (pContext->pEntryPoint[*pwCurrentIndex] == 0x6A || pContext->pEntryPoint[*pwCurrentIndex] == 0x68);
				pInstruction->Type = INTERPRETER_PUSH;

				if (pInstruction->u.PUSH_ARGS.OOperand == 0)
				{
					eInterpreterReturn tmp;

					if (pInstruction->u.PUSH_ARGS.IOperand == 0)
					{
						(*pwCurrentIndex)++;

						if (pContext->wSize <= *pwCurrentIndex)
						{
							return INTERPRETER_E_EOF;
						}

						pInstruction->u.PUSH_ARGS.Operand = pContext->pEntryPoint[*pwCurrentIndex];

						tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, (UCHAR)pInstruction->u.PUSH_ARGS.Operand);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}
					else
					{
						UCHAR ucSize = 4;

						if (opcode == 0x6A)
						{
							ucSize = 1;
						}
						else if(pInstruction->OperandOverride == 1)
						{
							ucSize = 2;
						}

						pInstruction->u.PUSH_ARGS.Size = ucSize;
						tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.PUSH_ARGS.Operand), ucSize);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}					
				}
				ucFinish = 1;
				break;
		
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
			case 0x48:
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4e:
			case 0x4f:
				//REX prefix				
				pInstruction->RexExtension = (pContext->pEntryPoint[*pwCurrentIndex] & 0xF);
				pInstruction->HasRexExtension = 1;
				(*pwCurrentIndex)++;
				break;

			case 0xC2:
			case 0xCA:
				//RET imm16
				(*pwCurrentIndex)++;
				
				if (pContext->wSize <= *pwCurrentIndex)
				{
					return INTERPRETER_E_EOF;
				}

				pInstruction->u.RET_ARGS.BytesToRemove = pContext->pEntryPoint[*pwCurrentIndex];
			case 0xC3:
			case 0xCB:
				//RET
				pInstruction->Type = INTERPRETER_RET;
				ucFinish = 1;
				break;

			case 0xFA:
				//CLI
				pInstruction->Type = INTERPRETER_CLI;
				ucFinish = 1;
				break;

			case 0x0F1F:
				//NOP with args
				{
					eInterpreterReturn tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, pInstruction->u.NOP_ARGS.Operand);

					if (tmp != INTERPRETER_OK)
					{
						return tmp;
					}
				}
			case 0x90:
				//NOP
				pInstruction->Type = INTERPRETER_NOP;
				ucFinish = 1;
				break;

			case 0xFB:
				//STI
				pInstruction->Type = INTERPRETER_STI;
				ucFinish = 1;
				break;

			case 0xE6:
			case 0xE7:
				//OUT with operand
				pInstruction->u.OUT_ARGS.WithOperand = 1;
			case 0xEE:
			case 0xEF:
				//OUT
				pInstruction->u.OUT_ARGS.AL = (pContext->pEntryPoint[*pwCurrentIndex] == 0xEE || pContext->pEntryPoint[*pwCurrentIndex] == 0xE6);
				pInstruction->u.OUT_ARGS.AX = (pContext->pEntryPoint[*pwCurrentIndex] == 0xEF || pContext->pEntryPoint[*pwCurrentIndex] == 0xE7);
				pInstruction->Type = INTERPRETER_OUT;

				if (pInstruction->u.OUT_ARGS.WithOperand != 0)
				{
					(*pwCurrentIndex)++;

					if (pContext->wSize <= *pwCurrentIndex)
					{
						return INTERPRETER_E_EOF;
					}

					pInstruction->u.OUT_ARGS.Operand = pContext->pEntryPoint[*pwCurrentIndex];
				}

				ucFinish = 1;
				break;

			case 0xD0:
			case 0xD1:
			case 0xD2:
			case 0xD3:
			case 0xC0:
			case 0xC1:
				//SAL,SAR,SHL,SHR
				{
					eInterpreterReturn tmp;

					pInstruction->Type = INTERPRETER_SHIFT;
					pInstruction->u.SHIFT_ARGS.M1 = (pContext->pEntryPoint[*pwCurrentIndex] == 0xD0 || pContext->pEntryPoint[*pwCurrentIndex] == 0xD1);
					pInstruction->u.SHIFT_ARGS.MC = (pContext->pEntryPoint[*pwCurrentIndex] == 0xD2 || pContext->pEntryPoint[*pwCurrentIndex] == 0xD3);
					pInstruction->u.SHIFT_ARGS.MI = (pContext->pEntryPoint[*pwCurrentIndex] == 0xC0 || pContext->pEntryPoint[*pwCurrentIndex] == 0xC1);

					(*pwCurrentIndex)++;

					if (pContext->wSize <= *pwCurrentIndex)
					{
						return INTERPRETER_E_EOF;
					}

					pInstruction->u.SHIFT_ARGS.Operand1 = pContext->pEntryPoint[*pwCurrentIndex];

					tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, pInstruction->u.SHIFT_ARGS.Operand1);

					if (tmp != INTERPRETER_OK)
					{
						return tmp;
					}

					if (pInstruction->u.SHIFT_ARGS.MI == 1)
					{
						(*pwCurrentIndex)++;

						if (pContext->wSize <= *pwCurrentIndex)
						{
							return INTERPRETER_E_EOF;
						}

						pInstruction->u.SHIFT_ARGS.Operand2 = pContext->pEntryPoint[*pwCurrentIndex];
					}

					ucFinish = 1;
				}
				break;

			case 0x0F20:
			case 0x0F22:
				//MOV two bytes opcode
				pInstruction->u.MOV_ARGS.ControlReg = 1;
			case 0x88:
			case 0x89:
			case 0x8A:
			case 0x8B:
			case 0x8C:
			case 0x8D:
			case 0xA0:
			case 0xA1:
			case 0xA3:
			case 0xB0:
			case 0xB1:
			case 0xB2:
			case 0xB3:
			case 0xB4:
			case 0xB5:
			case 0xB6:
			case 0xB7:
			case 0xB8:
			case 0xB9:
			case 0xBA:
			case 0xBB:
			case 0xBC:
			case 0xBD:
			case 0xBE:
			case 0xBF:
			case 0xC6:
			case 0xC7:
				//MOV
				{
					eInterpreterReturn tmp;

					pInstruction->Type = INTERPRETER_MOV;
					pInstruction->u.MOV_ARGS.MR = (pContext->pEntryPoint[*pwCurrentIndex] == 0x88 || pContext->pEntryPoint[*pwCurrentIndex] == 0x89 || pContext->pEntryPoint[*pwCurrentIndex] == 0x8C || opcode == 0x0F20);
					pInstruction->u.MOV_ARGS.RM = (pContext->pEntryPoint[*pwCurrentIndex] == 0x8A || pContext->pEntryPoint[*pwCurrentIndex] == 0x8B || pContext->pEntryPoint[*pwCurrentIndex] == 0x8E || opcode == 0x0F22);
					pInstruction->u.MOV_ARGS.FD = (pContext->pEntryPoint[*pwCurrentIndex] == 0xA0 || pContext->pEntryPoint[*pwCurrentIndex] == 0xA1);
					pInstruction->u.MOV_ARGS.TD = (pContext->pEntryPoint[*pwCurrentIndex] == 0xA2 || pContext->pEntryPoint[*pwCurrentIndex] == 0xA3);
					pInstruction->u.MOV_ARGS.OI = (pContext->pEntryPoint[*pwCurrentIndex] >= 0xB0 && pContext->pEntryPoint[*pwCurrentIndex] <= 0xBF);
					pInstruction->u.MOV_ARGS.MI = (pContext->pEntryPoint[*pwCurrentIndex] == 0xC6 || pContext->pEntryPoint[*pwCurrentIndex] == 0xC7);

					if (pInstruction->u.MOV_ARGS.OI == 1)
					{
						pInstruction->u.MOV_ARGS.Operand1 = (pContext->pEntryPoint[*pwCurrentIndex] & 0xF);

						if (pContext->pEntryPoint[*pwCurrentIndex] >= 0xB8 && pContext->pEntryPoint[*pwCurrentIndex] <= 0xBF)
						{
							pInstruction->u.MOV_ARGS.Operand1 -= 8;
						}
					}
					else if (pInstruction->u.MOV_ARGS.TD == 0 && pInstruction->u.MOV_ARGS.FD == 0)
					{
						(*pwCurrentIndex)++;

						if (pContext->wSize <= *pwCurrentIndex)
						{
							return INTERPRETER_E_EOF;
						}

						pInstruction->u.MOV_ARGS.Operand1 = pContext->pEntryPoint[*pwCurrentIndex];

						tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, (UCHAR)pInstruction->u.MOV_ARGS.Operand1);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					if (pInstruction->u.MOV_ARGS.MR == 0 && pInstruction->u.MOV_ARGS.RM == 0)
					{
						UCHAR ucSize = 4;

						if ((opcode >= 0xB0 && opcode <= 0xB7) || opcode == 0xC6 || opcode == 0xA2)
						{
							ucSize = 1;
						}
						else if (pInstruction->OperandOverride == 1)
						{
							ucSize = 2;
						}
						else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
						{
							ucSize = 8;
						}

						pInstruction->u.MOV_ARGS.Size = ucSize;
						tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.MOV_ARGS.Operand2), ucSize);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					ucFinish = 1;
				}
				break;

			case 0x80:
			case 0x81:
			case 0x83:
				//mapping not unique => analyze second byte
				{
					eInterpreterReturn tmp;
					UCHAR modByte, subOpcode;

					(*pwCurrentIndex)++;

					if (pContext->wSize <= *pwCurrentIndex)
					{
						return INTERPRETER_E_EOF;
					}

					modByte = pContext->pEntryPoint[*pwCurrentIndex];
					subOpcode = (modByte >> 3) & 0x7;

					tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, modByte);

					if (tmp != INTERPRETER_OK)
					{
						return tmp;
					}

					switch (subOpcode)
					{
						case 0x0:
							//ADD
							{
								UCHAR ucSize = 4;

								pInstruction->Type = INTERPRETER_ADD;
								pInstruction->u.ADD_ARGS.MI = 1;
								pInstruction->u.ADD_ARGS.Operand1 = modByte;

								if (opcode == 0x80 || opcode == 0x83)
								{
									ucSize = 1;
								}
								else if (pInstruction->OperandOverride == 1)
								{
									ucSize = 2;
								}

								pInstruction->u.ADD_ARGS.Size = ucSize;
								tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.ADD_ARGS.Operand2), ucSize);

								if (tmp != INTERPRETER_OK)
								{
									return tmp;
								}

								ucFinish = 1;
							}
							break;

						case 0x5:
							//SUB
							{
								UCHAR ucSize = 4;

								pInstruction->Type = INTERPRETER_SUB;
								pInstruction->u.SUB_ARGS.MI = 1;
								pInstruction->u.SUB_ARGS.Operand1 = modByte;

								if (opcode == 0x80 || opcode == 0x83)
								{
									ucSize = 1;
								}
								else if (pInstruction->OperandOverride == 1)
								{
									ucSize = 2;
								}

								pInstruction->u.SUB_ARGS.Size = ucSize;
								tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.SUB_ARGS.Operand2), ucSize);

								if (tmp != INTERPRETER_OK)
								{
									return tmp;
								}

								ucFinish = 1;
							}
							break;

						case 0x7:
							//CMP
							{
								UCHAR ucSize = 4;

								pInstruction->Type = INTERPRETER_CMP;
								pInstruction->u.CMP_ARGS.MI = 1;
								pInstruction->u.CMP_ARGS.Operand1 = modByte;								

								if (opcode == 0x80 || opcode == 0x83)
								{
									ucSize = 1;
								}
								else if (pInstruction->OperandOverride == 1)
								{
									ucSize = 2;
								}

								pInstruction->u.CMP_ARGS.Size = ucSize;
								tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.CMP_ARGS.Operand2), ucSize);

								if (tmp != INTERPRETER_OK)
								{
									return tmp;
								}

								ucFinish = 1;
							}
							break;

						default:
							return INTERPRETER_E_UNKNOWN_INSTRUCTION;
					}
				}				
				break;

			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
				//ADD
				{
					eInterpreterReturn tmp;
					pInstruction->Type = INTERPRETER_ADD;
					pInstruction->u.ADD_ARGS.I = (pContext->pEntryPoint[*pwCurrentIndex] == 0x04 || pContext->pEntryPoint[*pwCurrentIndex] == 0x05);
					pInstruction->u.ADD_ARGS.MR = (pContext->pEntryPoint[*pwCurrentIndex] == 0x00 || pContext->pEntryPoint[*pwCurrentIndex] == 0x01);
					pInstruction->u.ADD_ARGS.RM = (pContext->pEntryPoint[*pwCurrentIndex] == 0x04 || pContext->pEntryPoint[*pwCurrentIndex] == 0x05);

					if (pInstruction->u.ADD_ARGS.I == 0)
					{
						(*pwCurrentIndex)++;

						if (pContext->wSize <= *pwCurrentIndex)
						{
							return INTERPRETER_E_EOF;
						}

						pInstruction->u.ADD_ARGS.Operand1 = pContext->pEntryPoint[*pwCurrentIndex];

						tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, (UCHAR)pInstruction->u.ADD_ARGS.Operand1);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					if (pInstruction->u.ADD_ARGS.I == 1)
					{
						UCHAR ucSize = 4;

						if (opcode == 0x3C)
						{
							ucSize = 1;
						}
						else if (pInstruction->OperandOverride == 1)
						{
							ucSize = 2;
						}

						pInstruction->u.ADD_ARGS.Size = ucSize;
						tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.ADD_ARGS.Operand2), ucSize);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					ucFinish = 1;
				}
				break;

			case 0x2C:
			case 0x2D:
			case 0x28:
			case 0x29:
			case 0x2A:
			case 0x2B:
				//SUB
				{
					eInterpreterReturn tmp;
					pInstruction->Type = INTERPRETER_SUB;
					pInstruction->u.SUB_ARGS.I = (pContext->pEntryPoint[*pwCurrentIndex] == 0x2C || pContext->pEntryPoint[*pwCurrentIndex] == 0x2D);
					pInstruction->u.SUB_ARGS.MR = (pContext->pEntryPoint[*pwCurrentIndex] == 0x28 || pContext->pEntryPoint[*pwCurrentIndex] == 0x29);
					pInstruction->u.SUB_ARGS.RM = (pContext->pEntryPoint[*pwCurrentIndex] == 0x2A || pContext->pEntryPoint[*pwCurrentIndex] == 0x2B);

					if (pInstruction->u.SUB_ARGS.I == 0)
					{
						(*pwCurrentIndex)++;

						if (pContext->wSize <= *pwCurrentIndex)
						{
							return INTERPRETER_E_EOF;
						}

						pInstruction->u.SUB_ARGS.Operand1 = pContext->pEntryPoint[*pwCurrentIndex];

						tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, (UCHAR)pInstruction->u.SUB_ARGS.Operand1);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					if (pInstruction->u.SUB_ARGS.I == 1)
					{
						UCHAR ucSize = 4;

						if (opcode == 0x3C)
						{
							ucSize = 1;
						}
						else if (pInstruction->OperandOverride == 1)
						{
							ucSize = 2;
						}

						pInstruction->u.SUB_ARGS.Size = ucSize;
						tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.SUB_ARGS.Operand2), ucSize);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					ucFinish = 1;
				}
				break;

			case 0x38:
			case 0x39:
			case 0x3A:
			case 0x3B:
			case 0x3C:
			case 0x3D:
				//CMP
				{
					eInterpreterReturn tmp;
					pInstruction->Type = INTERPRETER_CMP;
					pInstruction->u.CMP_ARGS.I = (pContext->pEntryPoint[*pwCurrentIndex] == 0x3C || pContext->pEntryPoint[*pwCurrentIndex] == 0x3D);
					pInstruction->u.CMP_ARGS.MR = (pContext->pEntryPoint[*pwCurrentIndex] == 0x38 || pContext->pEntryPoint[*pwCurrentIndex] == 0x39);
					pInstruction->u.CMP_ARGS.RM = (pContext->pEntryPoint[*pwCurrentIndex] == 0x3A || pContext->pEntryPoint[*pwCurrentIndex] == 0x3B);

					if (pInstruction->u.CMP_ARGS.I == 0)
					{
						(*pwCurrentIndex)++;

						if (pContext->wSize <= *pwCurrentIndex)
						{
							return INTERPRETER_E_EOF;
						}

						pInstruction->u.CMP_ARGS.Operand1 = pContext->pEntryPoint[*pwCurrentIndex];

						tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, (UCHAR)pInstruction->u.CMP_ARGS.Operand1);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					if (pInstruction->u.CMP_ARGS.I == 1)
					{
						UCHAR ucSize = 4;

						if (opcode == 0x3C)
						{
							ucSize = 1;
						}
						else if (pInstruction->OperandOverride == 1)
						{
							ucSize = 2;
						}

						pInstruction->u.CMP_ARGS.Size = ucSize;
						tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.CMP_ARGS.Operand2), ucSize);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					ucFinish = 1;
				}
				break;

			case 0xFF:
				//mapping not unique => analyze second byte
				{
					eInterpreterReturn tmp;
					UCHAR modByte, subOpcode;

					(*pwCurrentIndex)++;

					if (pContext->wSize <= *pwCurrentIndex)
					{
						return INTERPRETER_E_EOF;
					}

					modByte = pContext->pEntryPoint[*pwCurrentIndex];
					subOpcode = (modByte >> 3) & 0x7;

					tmp = ParseModRM(pContext, pwCurrentIndex, pInstruction, modByte);

					if (tmp != INTERPRETER_OK)
					{
						return tmp;
					}

					switch (subOpcode)
					{
						case 0x2:
							//CALL
							pInstruction->u.CALL_ARGS.M = 1;
						case 0x3:
							//CALL
							{
								pInstruction->u.CALL_ARGS.Absolute = 1;
								pInstruction->u.CALL_ARGS.Operand = modByte;

								UCHAR ucSize = 4;

								if (pInstruction->OperandOverride == 1)
								{
									ucSize = 2;
								}
								else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
								{
									ucSize = 8;
								}

								pInstruction->u.CALL_ARGS.Size = ucSize;
								tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.CALL_ARGS.Offset), ucSize);

								if (tmp != INTERPRETER_OK)
								{
									return tmp;
								}

								pInstruction->Type = INTERPRETER_CALL;
								ucFinish = 1;
							}
							break;

						case 0x5:
							//JMP
							pInstruction->u.JUMP_ARGS.D = 1;
						case 0x4:
							//JMP
							pInstruction->Type = INTERPRETER_JUMP;							
							pInstruction->u.JUMP_ARGS.M = (subOpcode == 0x4);
							pInstruction->u.JUMP_ARGS.JMP = 1;
							pInstruction->u.JUMP_ARGS.Absolute = 1;

							pInstruction->u.JUMP_ARGS.Operand = modByte;

							if(pInstruction->u.JUMP_ARGS.D == 1)
							{
								UCHAR ucSize = 4;

								if (pInstruction->OperandOverride == 1)
								{
									ucSize = 2;
								}
								else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
								{
									ucSize = 8;
								}

								pInstruction->u.JUMP_ARGS.Size = ucSize;
								tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.JUMP_ARGS.Offset), ucSize);

								if (tmp != INTERPRETER_OK)
								{
									return tmp;
								}
							}

							ucFinish = 1;
							break;

						case 0x6:
							//PUSH with parameters
							pInstruction->u.PUSH_ARGS.MOperand = 1;
							pInstruction->Type = INTERPRETER_PUSH;

							pInstruction->u.PUSH_ARGS.Operand = modByte;
							pInstruction->u.PUSH_ARGS.Register = modByte & 0x7;
							ucFinish = 1;
							break;

						default:
							return INTERPRETER_E_UNKNOWN_INSTRUCTION;
					}
				}
				break;

			case 0x9A:
				//CALL
				pInstruction->u.CALL_ARGS.Absolute = 1;
			case 0xE8:
				//CALL
				{
					UCHAR ucSize = 4;

					if (pInstruction->OperandOverride == 1)
					{
						ucSize = 2;
					}
					else if (pInstruction->HasRexExtension && (pInstruction->RexExtension & REXW_MASK) != 0)
					{
						ucSize = 8;
					}

					pInstruction->u.CALL_ARGS.Size = ucSize;
					eInterpreterReturn tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.CALL_ARGS.Offset), ucSize);

					if (tmp != INTERPRETER_OK)
					{
						return tmp;
					}

					pInstruction->Type = INTERPRETER_CALL;
					ucFinish = 1;
				}
				break;

			case 0xEA:
				//JMP absolute
				pInstruction->u.JUMP_ARGS.Absolute = 1;
			case 0xEB:
			case 0xE9:			
				pInstruction->u.JUMP_ARGS.JMP = 1;				
			case 0x70:
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x76:
			case 0x77:
			case 0x78:
			case 0x79:
			case 0x7A:
			case 0x7B:
			case 0x7C:
			case 0x7D:
			case 0x7E:
			case 0x7F:
			case 0xE3:
			case 0x0F80:
			case 0x0F81:
			case 0x0F82:
			case 0x0F83:
			case 0x0F84:
			case 0x0F85:
			case 0x0F86:
			case 0x0F87:
			case 0x0F88:
			case 0x0F89:
			case 0x0F8A:
			case 0x0F8B:
			case 0x0F8C:
			case 0x0F8D:
			case 0x0F8E:
			case 0x0F8F:
				//JMP, Jcc
				{
					eInterpreterReturn tmp;

					pInstruction->Type = INTERPRETER_JUMP;					
					pInstruction->u.JUMP_ARGS.D = 1;
					pInstruction->u.JUMP_ARGS.JA = (pContext->pEntryPoint[*pwCurrentIndex] == 0x77 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F87);
					pInstruction->u.JUMP_ARGS.JBE = (pContext->pEntryPoint[*pwCurrentIndex] == 0x76 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F86);
					pInstruction->u.JUMP_ARGS.JC = (pContext->pEntryPoint[*pwCurrentIndex] == 0x72 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F82);
					pInstruction->u.JUMP_ARGS.JCXZ = (pContext->pEntryPoint[*pwCurrentIndex] == 0xE3);
					pInstruction->u.JUMP_ARGS.JE = (pContext->pEntryPoint[*pwCurrentIndex] == 0x74 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F84);
					pInstruction->u.JUMP_ARGS.JG = (pContext->pEntryPoint[*pwCurrentIndex] == 0x7F || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F8F);
					pInstruction->u.JUMP_ARGS.JGE = (pContext->pEntryPoint[*pwCurrentIndex] == 0x7D || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F8D);
					pInstruction->u.JUMP_ARGS.JL = (pContext->pEntryPoint[*pwCurrentIndex] == 0x7C || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F8C);
					pInstruction->u.JUMP_ARGS.JNC = (pContext->pEntryPoint[*pwCurrentIndex] == 0x73 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F83);
					pInstruction->u.JUMP_ARGS.JNG = (pContext->pEntryPoint[*pwCurrentIndex] == 0x7E || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F8E);
					pInstruction->u.JUMP_ARGS.JNO = (pContext->pEntryPoint[*pwCurrentIndex] == 0x71 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F81);
					pInstruction->u.JUMP_ARGS.JNP = (pContext->pEntryPoint[*pwCurrentIndex] == 0x7B || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F8B);
					pInstruction->u.JUMP_ARGS.JNS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x79 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F89);
					pInstruction->u.JUMP_ARGS.JNZ = (pContext->pEntryPoint[*pwCurrentIndex] == 0x75 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F85);
					pInstruction->u.JUMP_ARGS.JO = (pContext->pEntryPoint[*pwCurrentIndex] == 0x70 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F80);
					pInstruction->u.JUMP_ARGS.JP = (pContext->pEntryPoint[*pwCurrentIndex] == 0x7A || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F8A);
					pInstruction->u.JUMP_ARGS.JS = (pContext->pEntryPoint[*pwCurrentIndex] == 0x78 || pContext->pEntryPoint[*pwCurrentIndex] == 0x0F88);

					if (pInstruction->u.JUMP_ARGS.D == 1)
					{
						UCHAR ucSize = 4;

						if (opcode == 0xEB || (opcode >= 0x70 && opcode <= 0x7F) || opcode == 0xE3)
						{
							ucSize = 1;
						}
						else if (pInstruction->OperandOverride == 1)
						{
							ucSize = 2;
						}

						pInstruction->u.JUMP_ARGS.Size = ucSize;
						tmp = ReadImmediateValue(pContext, pwCurrentIndex, (UCHAR*)&(pInstruction->u.JUMP_ARGS.Operand), ucSize);

						if (tmp != INTERPRETER_OK)
						{
							return tmp;
						}
					}

					ucFinish = 1;
				}
				break;

			default:
				return INTERPRETER_E_UNKNOWN_INSTRUCTION;
		}

		if (ucFinish != 0)
		{
			pInstruction->Len = (UCHAR)((*pwCurrentIndex) - wStartIndex + 1); //+1 Index is on last byte
			break;
		}
	}

	return ret;
}

eInterpreterReturn ParseModRM(psInterpreterContext pContext, PUSHORT pwCurrentIndex, psInstruction pInstruction, UCHAR ucModRM)
{
	eInterpreterReturn ret = INTERPRETER_OK;
	UCHAR mod = (ucModRM >> 6) & 0x3;
	UCHAR rm = (ucModRM & 0x7);

	pInstruction->Mod = mod;

	if (mod != 0x3)
	{
		UCHAR sibRequired = 0;
		UCHAR displacementBytes = 0;

		if (pContext->ucAddressingMode == INTERPRETER_64BIT || (pInstruction->AddressOverride == 1))
		{
			if (rm == 0x4)
			{
				sibRequired = 1;
			}

			if (rm == 0x7 || mod == 0x2)
			{
				displacementBytes = 4;
			}

			if (mod == 0x1)
			{
				displacementBytes = 1;
			}
		}
		else
		{
			if (mod == 0 && rm == 0x6)
			{
				displacementBytes = 2;
			}

			if (mod == 0x2)
			{
				displacementBytes = 2;
			}
			else if (mod == 0x1)
			{
				displacementBytes = 1;
			}
		}

		if (sibRequired)
		{
			UCHAR base;

			(*pwCurrentIndex)++;

			if (pContext->wSize <= *pwCurrentIndex)
			{
				return INTERPRETER_E_EOF;
			}

			pInstruction->SibByte = pContext->pEntryPoint[*pwCurrentIndex];
			pInstruction->HasSibByte = 1;

			base = (pInstruction->SibByte) & 0x7;

			if (mod == 0 && base == 0x5)
			{
				displacementBytes = 4;
			}
		}

		for (UCHAR i = 0; i < displacementBytes; i++)
		{
			(*pwCurrentIndex)++;

			if (pContext->wSize <= *pwCurrentIndex)
			{
				return INTERPRETER_E_EOF;
			}

			pInstruction->Displacement += (pContext->pEntryPoint[*pwCurrentIndex] << (i * 8));
		}

		if (sibRequired || displacementBytes > 0)
		{
			pInstruction->HasIndirectAdressing = 1;
		}
	}

	return ret;
}

eInterpreterReturn ReadImmediateValue(psInterpreterContext pContext, PUSHORT pwCurrentIndex, UCHAR* pDest, UCHAR ucSize)
{
	eInterpreterReturn ret = INTERPRETER_OK;

	for (UCHAR i = 0; i < ucSize; i++, pDest++)
	{
		(*pwCurrentIndex)++;

		if (pContext->wSize <= *pwCurrentIndex)
		{
			return INTERPRETER_E_EOF;
		}

		*pDest = pContext->pEntryPoint[*pwCurrentIndex];
	}

	return ret;
}
