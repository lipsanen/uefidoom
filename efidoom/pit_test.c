#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "dlibc.h"
#include "efi_utils.h"
#include "efi.h"
#include "x86.h"

EFI_STATUS efi_main(
	EFI_HANDLE handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_BOOT_SERVICES *BS = system_table->BootServices;
	Init(system_table);

	EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	status = system_table->ConOut->ClearScreen(system_table->ConOut);
	if (status != 0)
		return status;

	calibrate_cpu();

	while (1)
	{
	}

	return 0;
}
