#ifdef __cplusplus
	extern "C" {
#endif

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4514 )

#include <stdio.h>
#include <stdarg.h>			// for variable argument list in DebugPrint function
#include <ntddk.h>

#pragma warning ( default : 4201 )

#ifdef __cplusplus
	}
#endif


#include "FscCmos.h"
#include "stddcls.h"
#include "driver.h"
#include "FSCIoctl.h"

#ifndef min
#define min(x, y) (((x) > (y)) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#pragma PAGEDCODE

#ifdef _USED_
static void DbgPrintBin(PUCHAR p, int cnt)
{
    char szLine[256];
    int x, y, c, pos;
    
    for (c = 0, x = 0, y = 0, pos = 0; c < cnt; c++)
    {
        pos += sprintf(szLine+pos, "%2.2x ", p[c]);
        x++;
        if (x >= 16)
        {
            y++; x = 0;
            DBG_OUT(0, ("%s\n", szLine));
            pos = 0;
        }
        else if (x == 8)
        {
            pos += sprintf(szLine+pos, " ");
        }
    }
    if (x > 0)
    {
        DBG_OUT(0, ("%s\n", szLine));
    }
}
#endif // #ifdef _USED_

static NTSTATUS WriteCmos( IN UCHAR *ucBuffer, IN ULONG ulOffset, IN ULONG ulCount)
/************************************************************************************
    Author:     OPUS Trinkl + Trinkl GmbH / PT                             
    Created:    12.06.2006
******************************************+*****************************************/ 
/**
     Write data into the CMOS
*/
{  
    ULONG ulSucc;
    NTSTATUS ret;
    DBG_OUT(2, ("[FscCmos: WriteCmos] - entered\n"));

    if (ulOffset == 0)
    {
#if (AMD64 | IA64)
        ulSucc = HalSetBusDataByOffset(Cmos, 0 , 0, ucBuffer, 0, ulCount);
#else //#if (AMD64 | IA64)
        ulSucc = HalSetBusData(Cmos, 0 , 0, ucBuffer, ulCount);
#endif //#if (AMD64 | IA64)
		int c = (int)(ulSucc - ulCount);
		if (c < 2 && c > -2)
        {
            ret = STATUS_SUCCESS;
            goto Leave;
        }
        else
        {
            DBG_OUT(1, ("[FscCmos: WriteCmos] #2: HalSetBusData returned error, ulSucc=%d, ulCount=%d\n", ulSucc, ulCount));
            ret = STATUS_INTERNAL_ERROR;
            goto Leave;
        }
    }
    else
    {
        UCHAR ucTmpBuf[128];
#if (AMD64 | IA64)
        ulSucc = HalGetBusDataByOffset(Cmos, 0 , 0, ucTmpBuf, 0, 128);
#else //#if (AMD64 | IA64)
        ulSucc = HalGetBusData(Cmos, 0 , 0, ucTmpBuf, 128);
#endif //#if (AMD64 | IA64)
        if (ulSucc >= 127)
        {
            memcpy(ucTmpBuf + ulOffset, ucBuffer, ulCount);
#if (AMD64 | IA64)
            ulSucc = HalSetBusDataByOffset(Cmos, 0 , 0, ucTmpBuf, 0, 128);
#else //#if (AMD64 | IA64)
            ulSucc = HalSetBusData(Cmos, 0 , 0, ucTmpBuf, 128);
#endif //#if (AMD64 | IA64)
            if (ulSucc >= 127)
            {
                ret = STATUS_SUCCESS;
                goto Leave;
            }
            else
            {
                DBG_OUT(1, ("[FscCmos: WriteCmos] #2: HalSetBusData returned error\n"));
                ret = STATUS_INTERNAL_ERROR;
                goto Leave;
            }
        }
        else
        {
            DBG_OUT(1, ("[FscCmos: WriteCmos] #2: HalGetBusData returned error\n"));
            ret = STATUS_INTERNAL_ERROR;
            goto Leave;
        }
    }
Leave:
    DBG_OUT(2, ("[FscCmos: WriteCmos] - leaving, ret=0x%8.8lx\n", ret));
    return ret;
} // WriteCmos

static NTSTATUS ReadCmos(OUT UCHAR *ucBuffer, IN ULONG ulOffset, IN ULONG ulRead)
/************************************************************************************
    Author:     OPUS Trinkl + Trinkl GmbH / PT                             
    Created:    13.10.1997
    21.11.03, RH  Replaced HalGetBusData by HalGetBusDataByOffset for 64bit
******************************************+*****************************************/ 
/**
     <<Read data from the CMOS>>
*/
{  
    ULONG ulSucc;
    NTSTATUS ret;

    DBG_OUT(2, ("[FscCmos: ReadCmos] - entered\n"));

    if (ulOffset == 0)
    {
#if (AMD64 | IA64)
        ulSucc = HalGetBusDataByOffset(Cmos, 0 , 0, ucBuffer, 0, ulRead);
#else //#if (AMD64 | IA64)
        ulSucc = HalGetBusData(Cmos, 0 , 0, ucBuffer, ulRead);
#endif //#if (AMD64 | IA64)
		int c = ulSucc - ulRead;
		if (c < 2 && c > -2)
        {
            ret = STATUS_SUCCESS;
            goto Leave;
        }
        else
        {
            ret = STATUS_INTERNAL_ERROR;
            goto Leave;
        }
    }
    else
    {
        UCHAR ucTmpBuf[128];
#if (AMD64 | IA64)
        ulSucc = HalGetBusDataByOffset(Cmos, 0 , 0, ucTmpBuf, 0, 128);
#else //#if (AMD64 | IA64)
        ulSucc = HalGetBusData(Cmos, 0 , 0, ucTmpBuf, 128);
#endif //#if (AMD64 | IA64)
        if (ulSucc >= 127)
        {
            memcpy(ucBuffer, ucTmpBuf + ulOffset, ulRead);
            ret = STATUS_SUCCESS;
            goto Leave;
        }
        else
        {
            ret = STATUS_INTERNAL_ERROR;
            goto Leave;
        }
    }
Leave:
    DBG_OUT(2, ("[FscCmos: ReadCmos] - leaving, ret=0x%8.8lx\n", ret));
    return ret;
} // ReadCmos

NTSTATUS
DriverIOCTL ( PDEVICE_OBJECT pDeviceObject, PIRP pIrp )
{
	/*PDEVICE_EXTENSION	pDevExt	  =*/ (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS			ntStatus;
  
    DBG_OUT(2, ("[FscCmos: DriverIOCTL] entered\n"));

	ULONG ulIoctlInputLength  = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ulIoctlOutputLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG *pulUlongSystemBuffer = (ULONG*)pIrp->AssociatedIrp.SystemBuffer;

	pIrp->IoStatus.Information = 0;
	ntStatus = STATUS_INVALID_PARAMETER;

	switch ( pIrpStack->Parameters.DeviceIoControl.IoControlCode )
	{
	case IOCTL_FSC_HARDWARE_PRESENT:
        DBG_OUT(2, ("[FscCmos: IOCTL_FSC_HARDWARE_PRESENT] entered\n"));
		if (ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = 1;
			pIrp->IoStatus.Information = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
        DBG_OUT(2, ("[FscCmos: IOCTL_FSC_HARDWARE_PRESENT] leaving\n"));
		break;

	case IOCTL_FSC_GET_DRIVER_VERSION:
        DBG_OUT(2, ("[FscCmos: IOCTL_FSC_GET_DRIVER_VERSION] entered\n"));
		if (ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = DRIVER_VERSION;
			pIrp->IoStatus.Information = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
        DBG_OUT(2, ("[FscCmos: IOCTL_FSC_GET_DRIVER_VERSION] leaving\n"));
		break;

	case IOCTL_CMOS_READ:
        DBG_OUT(2, ("[FscCmos: IOCTL_CMOS_READ] entered\n"));
		if (ulIoctlInputLength == sizeof(CMOS_READ_T))
		{
			//ULONG ulRead;
			CMOS_READ_T *p = (CMOS_READ_T*)(pIrp->AssociatedIrp.SystemBuffer);
            DBG_OUT(0, ("[FscCmos: CMOS data read] ulOffset=%d, size=%d\n", p->ulOffset, ulIoctlOutputLength));
			
            ntStatus = ReadCmos((PUCHAR)pIrp->AssociatedIrp.SystemBuffer, p->ulOffset, ulIoctlOutputLength);
            // ulRead = HalGetBusDataByOffset(Cmos, 0 , 0, pIrp->AssociatedIrp.SystemBuffer, p->ulOffset, ulIoctlOutputLength);
#ifdef _AMD64_
			// if (ulRead)
#else
			// if (ulRead == ulIoctlOutputLength)
#endif
			// {
#ifdef DBGLEVEL
                DbgPrintBin((PUCHAR)pIrp->AssociatedIrp.SystemBuffer, (int)ulIoctlOutputLength);
#endif
            pIrp->IoStatus.Information = ulIoctlOutputLength;
             //   ntStatus = STATUS_SUCCESS;
           // }
           // else
           // {
           //     DBG_OUT(0, ("[FscCmos: IOCTL_CMOS_READ] size mismatch ulRead=%d, ulIoctlOutputLength=%d\n", ulRead, ulIoctlOutputLength));
           // }
		}
        else
        {
            DBG_OUT(0, ("[FscCmos: IOCTL_CMOS_READ] ulIoctlInputLength size mismatch %d\n", ulIoctlInputLength));
        }
        DBG_OUT(2, ("[FscCmos: IOCTL_CMOS_READ] leaving\n"));
		break;

	case IOCTL_CMOS_WRITE:
        DBG_OUT(2, ("[FscCmos: IOCTL_CMOS_WRITE] entered\n"));
		if ((ulIoctlInputLength >= sizeof(CMOS_WRITE_T)) && (ulIoctlOutputLength == 0))
		{
			/*ULONG ulWrote;*/
			CMOS_WRITE_T *p  = (CMOS_WRITE_T*)(pIrp->AssociatedIrp.SystemBuffer);
			ULONG cnt = ulIoctlInputLength - FIELD_OFFSET(CMOS_WRITE_T, Data);

#ifdef DBG
            DBG_OUT(0, ("[FscCmos: CMOS data write] ulOffset=%d, cnt=%d\n", p->ulOffset, cnt));
            DbgPrintBin(p->Data, cnt);
#endif
            ntStatus = WriteCmos(p->Data, p->ulOffset, cnt);

            // ulWrote = HalSetBusDataByOffset(Cmos, 0 , 0, p->Data, p->ulOffset, cnt);
#ifdef _AMD64_
			// if (ulWrote)
#else
			// if (ulWrote == cnt)
#endif
			//	ntStatus = STATUS_SUCCESS;
		}
        else
        {
            DBG_OUT(0, ("[FscCmos: IOCTL_CMOS_WRITE] Invalid input buffer size %d\n", ulIoctlInputLength));
        }
        DBG_OUT(2, ("[FscCmos: IOCTL_CMOS_WRITE] leaving\n"));
		break;

	default:
        DBG_OUT(1, ("[FscCmos: invalid IOCTL] entered, code=0x%8.8lx\n", pIrpStack->Parameters.DeviceIoControl.IoControlCode));

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;

        DBG_OUT(1, ("[FscCmos: invalid IOCTL] leaving\n"));
		break;
	}

	pIrp->IoStatus.Status = ntStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    DBG_OUT(2, ("[FscCmos: DriverIOCTL] leaving, ntStatus=%d\n", ntStatus));

	return ntStatus;
}

#pragma LOCKEDCODE

// This driver does all IOCTL without queueing, so this is a dummy
VOID StartIo ( PDEVICE_OBJECT /*fdo*/, PIRP /*pIrp*/ )
{
}