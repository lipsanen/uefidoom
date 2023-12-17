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
	EFI_BOOT_SERVICES* BS = system_table->BootServices;
	Init(system_table);

	EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	status = system_table->ConOut->ClearScreen(system_table->ConOut);
	if (status != 0)
		return status;

	calibrate_cpu();
	uint64_t start = clock_msec();

	for(size_t i=0;;++i)
	{
		uint64_t now;

		while((now = clock_msec()) - start < 1000 * i);

		d_printf("Tick %u\n", i);
	}


	return 0;
}

