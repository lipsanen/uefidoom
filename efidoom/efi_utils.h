#include "efi.h"

extern EFI_SYSTEM_TABLE* g_pSystemTable;

void Init(EFI_SYSTEM_TABLE* pSystemTable);

EFI_STATUS LibLocateProtocol (
    IN EFI_BOOT_SERVICES* BS,
    IN  EFI_GUID    *ProtocolGuid,
    OUT VOID        **Interface
    );

void* AllocatePool(size_t size);
void FreePool(void* ptr);
void Print(uint16_t* str);

EFI_STATUS
LibLocateHandle (
    IN EFI_BOOT_SERVICES* BS,
    IN EFI_LOCATE_SEARCH_TYPE       SearchType,
    IN EFI_GUID                     *Protocol OPTIONAL,
    IN VOID                         *SearchKey OPTIONAL,
    IN OUT UINTN                    *NoHandles,
    OUT EFI_HANDLE                  **Buffer
    );

BOOLEAN
GrowBuffer(
    IN EFI_BOOT_SERVICES* BS,
    IN OUT EFI_STATUS   *Status,
    IN OUT VOID         **Buffer,
    IN UINTN            BufferSize
    );
