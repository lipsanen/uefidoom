#include "efi_utils.h"

EFI_SYSTEM_TABLE *g_pSystemTable;

void Init(EFI_SYSTEM_TABLE *pSystemTable)
{
    g_pSystemTable = pSystemTable;
}

#define BUFFER_SIZE 128
static uint16_t BUFFER[BUFFER_SIZE];
static uint32_t index = 0;

void _put(int c)
{
    uint16_t BUFFER[2];
    BUFFER[0] = c;
    BUFFER[1] = '\0';
    g_pSystemTable->ConOut->OutputString(g_pSystemTable->ConOut, BUFFER);
}

int d_putchar(int c)
{
    if (c == '\n')
    {
        _put('\r');
        _put('\n');
    }
    else
    {
        _put(c);
    }
    return 1;
}

void Print(uint16_t *str)
{
    g_pSystemTable->ConOut->OutputString(g_pSystemTable->ConOut, str);
}

EFI_STATUS LibLocateProtocol(
    IN EFI_BOOT_SERVICES *BS,
    IN EFI_GUID *ProtocolGuid,
    OUT VOID **Interface)
//
// Find the first instance of this Protocol in the system and return it's interface
//
{
    EFI_STATUS Status;
    UINTN NumberHandles, Index;
    EFI_HANDLE *Handles;

    *Interface = NULL;
    Status = LibLocateHandle(BS, ByProtocol, ProtocolGuid, NULL, &NumberHandles, &Handles);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    for (Index = 0; Index < NumberHandles; Index++)
    {
        Status = BS->HandleProtocol(Handles[Index], ProtocolGuid, Interface);
        if (!EFI_ERROR(Status))
        {
            break;
        }
    }

    if (Handles)
    {
        BS->FreePool(Handles);
    }

    return Status;
}

void *AllocatePool(size_t size)
{
    EFI_STATUS Status;
    VOID *p;

    Status = g_pSystemTable->BootServices->AllocatePool(0, size, &p);
    if (Status < 0)
    {
        p = NULL;
    }
    return p;
}

void FreePool(void *ptr)
{
    g_pSystemTable->BootServices->FreePool(ptr);
}

EFI_STATUS
LibLocateHandle(
    IN EFI_BOOT_SERVICES *BS,
    IN EFI_LOCATE_SEARCH_TYPE SearchType,
    IN EFI_GUID *Protocol OPTIONAL,
    IN VOID *SearchKey OPTIONAL,
    IN OUT UINTN *NoHandles,
    OUT EFI_HANDLE **Buffer)

{
    EFI_STATUS Status;
    UINTN BufferSize;

    //
    // Initialize for GrowBuffer loop
    //

    Status = EFI_SUCCESS;
    *Buffer = NULL;
    BufferSize = 50 * sizeof(EFI_HANDLE);

    //
    // Call the real function
    //

    while (GrowBuffer(BS, &Status, (VOID **)Buffer, BufferSize))
    {

        Status = BS->LocateHandle(
            SearchType,
            Protocol,
            SearchKey,
            &BufferSize,
            *Buffer);
    }

    *NoHandles = BufferSize / sizeof(EFI_HANDLE);
    if (EFI_ERROR(Status))
    {
        *NoHandles = 0;
    }

    return Status;
}

BOOLEAN
GrowBuffer(
    IN EFI_BOOT_SERVICES *BS,
    IN OUT EFI_STATUS *Status,
    IN OUT VOID **Buffer,
    IN UINTN BufferSize)
{
    BOOLEAN TryAgain;

    if (!*Buffer && BufferSize)
    {
        *Status = EFI_BUFFER_TOO_SMALL;
    }

    TryAgain = FALSE;
    if (*Status == EFI_BUFFER_TOO_SMALL)
    {

        if (*Buffer)
        {
            BS->FreePool(*Buffer);
        }

        *Buffer = AllocatePool(BufferSize);

        if (*Buffer)
        {
            TryAgain = TRUE;
        }
        else
        {
            *Status = EFI_OUT_OF_RESOURCES;
        }
    }

    if (!TryAgain && EFI_ERROR(*Status) && *Buffer)
    {
        BS->FreePool(*Buffer);
        *Buffer = NULL;
    }

    return TryAgain;
}
