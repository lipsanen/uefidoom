#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "efi_utils.h"
#include "efi.h"
#include "dlibc.h"
#include "x86.h"

EFI_STATUS efi_main(
	EFI_HANDLE handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_BOOT_SERVICES* BS = system_table->BootServices;
	Init(system_table);

	EFI_GUID inputGuid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

	EFI_KEY_TOGGLE_STATE state;

	status = system_table->ConOut->ClearScreen(system_table->ConOut);
	if (status != 0)
		return status;

	EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* pProt;
	status = LibLocateProtocol(g_pSystemTable->BootServices, &inputGuid, (void**)&pProt);
	calibrate_cpu();

	if (status != 0)
		return status;

	while(1) {
		EFI_KEY_DATA data;
		status = pProt->ReadKeyStrokeEx(pProt, &data);

		if (status != EFI_SUCCESS)
			continue;

		d_printf("yes %u, clock: %u\n", data.Key.ScanCode ? data.Key.ScanCode : data.Key.UnicodeChar, clock_msec());
	}


	return 0;
}

