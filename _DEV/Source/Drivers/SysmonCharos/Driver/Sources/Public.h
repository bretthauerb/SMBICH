/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_SysMonCharos,
    0x822a707f,0xeecc,0x42ed,0xb2,0x92,0xff,0x89,0x54,0x83,0x5a,0xf5);
// {822a707f-eecc-42ed-b292-ff8954835af5}
