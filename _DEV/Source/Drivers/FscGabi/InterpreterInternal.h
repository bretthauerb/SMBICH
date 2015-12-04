#pragma once

typedef enum
{
	INTERPRETER_UNKOWN = 0,
	INTERPRETER_MOV,
	INTERPRETER_SHIFT,
	INTERPRETER_ADD,
	INTERPRETER_SUB,
	INTERPRETER_CLI,
	INTERPRETER_STI,
	INTERPRETER_NOP,
	INTERPRETER_CALL,
	INTERPRETER_OUT,
	INTERPRETER_CMP,
	INTERPRETER_JUMP,
	INTERPRETER_PUSH,
	INTERPRETER_POP,
	INTERPRETER_RET
}eInstructionType;

#define REXW_MASK 0x08
#define REXR_MASK 0x04
#define REXX_MASK 0x02
#define REXB_MASK 0x01

typedef struct
{
	UCHAR HasRexExtension : 1;
	UCHAR RexExtension : 4;
	UCHAR OperandOverride : 1;
	UCHAR AddressOverride : 1;
	UCHAR HasSibByte : 1;
	UCHAR SibByte;
	ULONGLONG Displacement;
	UCHAR HasIndirectAdressing : 1;
	UCHAR Mod : 2;
	UCHAR Len;
	eInstructionType Type;
	union
	{
		struct
		{
			USHORT BytesToRemove;
		} 
		RET_ARGS;
		struct
		{
			UCHAR MOperand : 1;
			UCHAR OOperand : 1;
			UCHAR IOperand : 1;
			UCHAR Register : 4;
			UCHAR CS : 1;
			UCHAR SS : 1;
			UCHAR DS : 1;
			UCHAR ES : 1;
			UCHAR FS : 1;
			UCHAR GS : 1;
			UCHAR A : 1;
			UCHAR F : 1;
			ULONGLONG Operand;
			UCHAR Size;
		}
		PUSH_ARGS;
		struct
		{
			UCHAR MOperand : 1;
			UCHAR OOperand : 1;
			UCHAR Register : 4;
			UCHAR SS : 1;
			UCHAR DS : 1;
			UCHAR ES : 1;
			UCHAR FS : 1;
			UCHAR GS : 1;
			UCHAR A : 1;
			UCHAR F : 1;
			ULONGLONG Operand;
			UCHAR Size;
		}
		POP_ARGS;
		struct
		{
			UCHAR WithOperand : 1;
			UCHAR AL : 1;
			UCHAR AX : 1;
			UCHAR Operand;
		}
		OUT_ARGS;
		struct
		{
			UCHAR M1 : 1;
			UCHAR MC : 1;
			UCHAR MI : 1;
			UCHAR Operand1;
			UCHAR Operand2;
		}
		SHIFT_ARGS;
		struct
		{
			UCHAR MR : 1;
			UCHAR RM : 1;
			UCHAR FD : 1;
			UCHAR TD : 1;
			UCHAR OI : 1;
			UCHAR MI : 1;
			UCHAR ControlReg : 1;			
			UCHAR Operand1;
			ULONGLONG Operand2;
			UCHAR Size;
		}
		MOV_ARGS;
		struct
		{
			UCHAR RM : 1;
			UCHAR MR : 1;
			UCHAR MI : 1;
			UCHAR I : 1;
			UCHAR Operand1;
			ULONG Operand2;
			UCHAR Size;
		}
		CMP_ARGS;
		struct
		{
			UCHAR JMP : 1;
			UCHAR JA : 1;
			UCHAR JBE : 1;
			UCHAR JC : 1;
			UCHAR JCXZ : 1;
			UCHAR JE : 1;
			UCHAR JG : 1;
			UCHAR JGE : 1;
			UCHAR JL : 1;
			UCHAR JNC : 1;
			UCHAR JNG : 1;
			UCHAR JNO : 1;
			UCHAR JNP : 1;
			UCHAR JNS : 1;
			UCHAR JNZ : 1;
			UCHAR JO : 1;
			UCHAR JP : 1;
			UCHAR JS : 1;
			UCHAR Absolute : 1;
			UCHAR D : 1;
			UCHAR M : 1;
			ULONG Operand;
			ULONGLONG Offset;
			UCHAR Size;
		}
		JUMP_ARGS;
		struct
		{
			UCHAR Operand;
		}
		NOP_ARGS;
		struct
		{
			UCHAR Absolute : 1;
			UCHAR M : 1;
			UCHAR Operand;
			ULONGLONG Offset;
			UCHAR Size;
		}
		CALL_ARGS;
		struct
		{
			UCHAR RM : 1;
			UCHAR MR : 1;
			UCHAR MI : 1;
			UCHAR I : 1;
			UCHAR Operand1;
			ULONG Operand2;
			UCHAR Size;
		}
		SUB_ARGS;
		struct
		{
			UCHAR RM : 1;
			UCHAR MR : 1;
			UCHAR MI : 1;
			UCHAR I : 1;
			UCHAR Operand1;
			ULONG Operand2;
			UCHAR Size;
		}
		ADD_ARGS;
	}u;
}sInstruction, *psInstruction;

#define STACK_SIZE 2048

typedef union
{
	ULONGLONG RVALUE;
	ULONG EVALUE;
	USHORT XVALUE;
	UCHAR LVALUE;
} uX86_X64Register;

typedef struct
{
	ULONGLONG RIP;  //no address. offset value
	ULONGLONG RFLAGS;
	uX86_X64Register A;
	uX86_X64Register B;
	uX86_X64Register C;
	uX86_X64Register D;
	uX86_X64Register SI;
	uX86_X64Register DI;
	uX86_X64Register BP;
	uX86_X64Register SP;  //no address. offset from struct start
	uX86_X64Register R8;
	uX86_X64Register R9;
	uX86_X64Register R10;
	uX86_X64Register R11;
	uX86_X64Register R12;
	uX86_X64Register R13;
	uX86_X64Register R14;
	uX86_X64Register R15;	
}sInterpreterCPUState, *psInterpreterCPUState;

#define GET_STATE(x) ((psInterpreterCPUState)(x->pInterpreterState))
#define GET_RFLAGS(x) GET_STATE(x)->RFLAGS
#define STACK_TOP(x) (((UCHAR*)GET_STATE((x))) + GET_STATE((x))->SP.RVALUE)
#define SET_REGISTER(x, regname, value, size) memcpy((UCHAR*)&(GET_STATE((x))->regname.RVALUE), (UCHAR*)&(value), size)

#define CF_MASK 0x1
#define PF_MASK 0x4
#define AF_MASK 0x10
#define ZF_MASK 0x40
#define SF_MASK 0x80
#define IF_MASK 0x200
#define OF_MASK 0x800

#define GET_CF(x) ((GET_RFLAGS(x)) & CF_MASK)
#define GET_PF(x) ((GET_RFLAGS(x)) & PF_MASK)
#define GET_AF(x) ((GET_RFLAGS(x)) & AF_MASK)
#define GET_ZF(x) ((GET_RFLAGS(x)) & ZF_MASK)
#define GET_SF(x) ((GET_RFLAGS(x)) & SF_MASK)
#define GET_OF(x) ((GET_RFLAGS(x)) & OF_MASK)

#define SET_CF(x, value) GET_RFLAGS(x) = (((value) == 0) ? ((GET_RFLAGS(x)) & ~CF_MASK) : ((GET_RFLAGS(x)) | CF_MASK))
#define SET_PF(x, value) GET_RFLAGS(x) = (((value) == 0) ? ((GET_RFLAGS(x)) & ~PF_MASK) : ((GET_RFLAGS(x)) | PF_MASK))
#define SET_AF(x, value) GET_RFLAGS(x) = (((value) == 0) ? ((GET_RFLAGS(x)) & ~AF_MASK) : ((GET_RFLAGS(x)) | AF_MASK))
#define SET_ZF(x, value) GET_RFLAGS(x) = (((value) == 0) ? ((GET_RFLAGS(x)) & ~ZF_MASK) : ((GET_RFLAGS(x)) | ZF_MASK))
#define SET_SF(x, value) GET_RFLAGS(x) = (((value) == 0) ? ((GET_RFLAGS(x)) & ~SF_MASK) : ((GET_RFLAGS(x)) | SF_MASK))
#define SET_OF(x, value) GET_RFLAGS(x) = (((value) == 0) ? ((GET_RFLAGS(x)) & ~OF_MASK) : ((GET_RFLAGS(x)) | OF_MASK))


eInterpreterReturn GetNextCommand(psInterpreterContext pContext, PUSHORT pwCurrentIndex, psInstruction pInstruction);
eInterpreterReturn ParseModRM(psInterpreterContext pContext, PUSHORT pwCurrentIndex, psInstruction pInstruction, UCHAR ucModRM);
eInterpreterReturn ReadImmediateValue(psInterpreterContext pContext, PUSHORT pwCurrentIndex, UCHAR* pDest, UCHAR ucSize);

eInterpreterReturn InitExecutionContext(psInterpreterContext pContext, PHYSICAL_ADDRESS pParam1, PHYSICAL_ADDRESS pParam2, PHYSICAL_ADDRESS pParam3);
eInterpreterReturn ExecuteInstruction(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_MOV(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_SHIFT(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_ADD(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_SUB(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_CLI(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_STI(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_OUT(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_CMP(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_JUMP(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_PUSH(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_POP(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_RET(psInterpreterContext pContext, psInstruction pInstruction);
eInterpreterReturn ExecuteInstruction_CALL(psInterpreterContext pContext, psInstruction pInstruction);

eInterpreterReturn GetIndirectAddressing(psInterpreterContext pContext, psInstruction pInstruction, UCHAR ucRM, ULONGLONG* pAddress);
eInterpreterReturn GetRegisterValue(psInterpreterContext pContext, UCHAR rex, UCHAR regIndex, ULONGLONG* pValue);
eInterpreterReturn SetRegisterValue(psInterpreterContext pContext, UCHAR rex, UCHAR regIndex, ULONGLONG value, UCHAR ucSize);
void UpdateFlags(psInterpreterContext pContext, ULONGLONG value, UCHAR ucSize);
ULONGLONG AddValues(psInterpreterContext pContext, ULONGLONG left, ULONGLONG right, UCHAR ucSize);