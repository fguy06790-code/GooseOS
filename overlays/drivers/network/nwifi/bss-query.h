/*
 * PROJECT:     LiberNT Native WiFi (802.11) stack
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Retrieve variable-length miniport BSS results
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */

#ifndef _NWIFI_BSS_QUERY_H_
#define _NWIFI_BSS_QUERY_H_

static NDIS_STATUS
NwifiReadBssList(PNWIFI_ADAPTER Adapter,
                 PDOT11_BYTE_ARRAY *Results,
                 PULONG BytesWritten)
{
    ULONG BufferSize = 16 * 1024;
    ULONG Attempt;
    NDIS_STATUS Status = NDIS_STATUS_BUFFER_TOO_SHORT;

    *Results = NULL;
    *BytesWritten = 0;

    for (Attempt = 0; Attempt < 4; Attempt++)
    {
        PDOT11_BYTE_ARRAY Buffer;
        ULONG Written = 0, Needed = 0;

        Buffer = (PDOT11_BYTE_ARRAY)NwifiAllocate(BufferSize);
        if (Buffer == NULL)
            return NDIS_STATUS_RESOURCES;

        Status = NwifiProtocolDoRequestEx(Adapter, NdisRequestMethod,
                                          OID_DOT11_ENUM_BSS_LIST,
                                          Buffer, BufferSize, &Written, &Needed);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            if (Written < FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer) ||
                Written > BufferSize ||
                Buffer->uNumOfBytes > Written - FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer))
            {
                NwifiFree(Buffer);
                return NDIS_STATUS_INVALID_LENGTH;
            }
            *Results = Buffer;
            *BytesWritten = Written;
            return NDIS_STATUS_SUCCESS;
        }

        NwifiFree(Buffer);
        if ((Status != NDIS_STATUS_BUFFER_OVERFLOW &&
             Status != NDIS_STATUS_BUFFER_TOO_SHORT &&
             Status != NDIS_STATUS_INVALID_LENGTH) ||
            Needed <= BufferSize || Needed > 1024 * 1024)
        {
            return Status;
        }
        BufferSize = Needed;
    }

    return Status;
}

#endif
