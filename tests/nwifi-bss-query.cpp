/*
 * PROJECT:     THYFOS Native WiFi tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Exercise BSS query resizing with a simulated miniport
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using ULONG = std::uint32_t;
using PULONG = ULONG *;
using PNWIFI_ADAPTER = void *;
enum NDIS_STATUS
{
    NDIS_STATUS_SUCCESS,
    NDIS_STATUS_BUFFER_OVERFLOW,
    NDIS_STATUS_BUFFER_TOO_SHORT,
    NDIS_STATUS_INVALID_LENGTH,
    NDIS_STATUS_RESOURCES,
    NDIS_STATUS_NOT_SUPPORTED
};
enum { NdisRequestMethod, OID_DOT11_ENUM_BSS_LIST };
struct DOT11_BYTE_ARRAY
{
    std::uint32_t Header;
    ULONG uNumOfBytes;
    ULONG uTotalNumOfBytes;
    unsigned char ucBuffer[1];
};
using PDOT11_BYTE_ARRAY = DOT11_BYTE_ARRAY *;
#define FIELD_OFFSET(Type, Field) offsetof(Type, Field)

struct Reply
{
    NDIS_STATUS status;
    ULONG written;
    ULONG needed;
    ULONG payload;
};

static std::vector<Reply> Replies;
static std::vector<ULONG> Lengths;
static std::size_t Allocations, Freed, FailAllocation;

static void *NwifiAllocate(std::size_t size)
{
    if (++Allocations == FailAllocation)
        return nullptr;
    return std::calloc(1, size);
}

static void NwifiFree(void *buffer)
{
    if (buffer)
    {
        ++Freed;
        std::free(buffer);
    }
}

static NDIS_STATUS NwifiProtocolDoRequestEx(PNWIFI_ADAPTER, int type, int oid,
                                           void *buffer, ULONG length,
                                           PULONG written, PULONG needed)
{
    if (type != NdisRequestMethod || oid != OID_DOT11_ENUM_BSS_LIST ||
        Lengths.size() >= Replies.size())
        std::abort();
    const Reply &reply = Replies[Lengths.size()];
    Lengths.push_back(length);
    *written = reply.written;
    *needed = reply.needed;
    static_cast<PDOT11_BYTE_ARRAY>(buffer)->uNumOfBytes = reply.payload;
    return reply.status;
}

#include "../overlays/drivers/network/nwifi/bss-query.h"

static void Reset(std::initializer_list<Reply> replies)
{
    Replies = replies;
    Lengths.clear();
    Allocations = Freed = FailAllocation = 0;
}

static bool Failed(NDIS_STATUS expected, std::size_t calls)
{
    PDOT11_BYTE_ARRAY result = reinterpret_cast<PDOT11_BYTE_ARRAY>(1);
    ULONG written = 1;
    return NwifiReadBssList(nullptr, &result, &written) == expected &&
           result == nullptr && written == 0 && Lengths.size() == calls &&
           Freed == Allocations - (FailAllocation ? 1 : 0);
}

int main()
{
    PDOT11_BYTE_ARRAY result;
    ULONG written;

    Reset({ {NDIS_STATUS_BUFFER_OVERFLOW, 0, 40000, 0},
            {NDIS_STATUS_SUCCESS, 40000, 0, 40000 - FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer)} });
    if (NwifiReadBssList(nullptr, &result, &written) != NDIS_STATUS_SUCCESS ||
        !result || written != 40000 || Lengths != std::vector<ULONG>{16384, 40000} ||
        Freed != 1)
        return 1;
    NwifiFree(result);
    if (Freed != Allocations)
        return 2;

    Reset({ {NDIS_STATUS_BUFFER_TOO_SHORT, 0, 32768, 0},
            {NDIS_STATUS_INVALID_LENGTH, 0, 65536, 0},
            {NDIS_STATUS_SUCCESS, FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer), 0, 0} });
    if (NwifiReadBssList(nullptr, &result, &written) != NDIS_STATUS_SUCCESS ||
        !result || Lengths != std::vector<ULONG>{16384, 32768, 65536})
        return 3;
    NwifiFree(result);
    if (Freed != Allocations)
        return 4;

    Reset({ {NDIS_STATUS_NOT_SUPPORTED, 0, 65536, 0} });
    if (!Failed(NDIS_STATUS_NOT_SUPPORTED, 1)) return 5;
    Reset({ {NDIS_STATUS_BUFFER_OVERFLOW, 0, 16384, 0} });
    if (!Failed(NDIS_STATUS_BUFFER_OVERFLOW, 1)) return 6;
    Reset({ {NDIS_STATUS_BUFFER_OVERFLOW, 0, 1024 * 1024 + 1, 0} });
    if (!Failed(NDIS_STATUS_BUFFER_OVERFLOW, 1)) return 7;
    Reset({ {NDIS_STATUS_BUFFER_OVERFLOW, 0, 32768, 0},
            {NDIS_STATUS_BUFFER_OVERFLOW, 0, 65536, 0},
            {NDIS_STATUS_BUFFER_OVERFLOW, 0, 131072, 0},
            {NDIS_STATUS_BUFFER_OVERFLOW, 0, 262144, 0} });
    if (!Failed(NDIS_STATUS_BUFFER_OVERFLOW, 4)) return 8;
    Reset({ {NDIS_STATUS_BUFFER_OVERFLOW, 0, 32768, 0} });
    FailAllocation = 2;
    if (!Failed(NDIS_STATUS_RESOURCES, 1)) return 9;
    Reset({ {NDIS_STATUS_SUCCESS, 16385, 0, 0} });
    if (!Failed(NDIS_STATUS_INVALID_LENGTH, 1)) return 10;
    Reset({ {NDIS_STATUS_SUCCESS, FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer) - 1, 0, 0} });
    if (!Failed(NDIS_STATUS_INVALID_LENGTH, 1)) return 11;
    Reset({ {NDIS_STATUS_SUCCESS, 100, 0, 100} });
    if (!Failed(NDIS_STATUS_INVALID_LENGTH, 1)) return 12;
    Reset({});
    FailAllocation = 1;
    if (!Failed(NDIS_STATUS_RESOURCES, 0)) return 13;

    std::puts("BSS query resizing, bounded errors, malformed results and allocation cleanup passed");
    return 0;
}
