#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "efi_utils.h"
#include "efi.h"
#include "dlibc.h"

EFI_STATUS efi_main(
	EFI_HANDLE handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_BOOT_SERVICES* BS = system_table->BootServices;
	Init(system_table);

	EFI_GUID guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;

	status = system_table->ConOut->ClearScreen(system_table->ConOut);
	if (status != 0)
		return status;

	EFI_SIMPLE_POINTER_PROTOCOL*pProt;
	status = LibLocateProtocol(BS, &guid, (void**)&pProt);

	if (status != 0)
		return status;
	d_printf("status returned %u\n", status);
	while(1) {
		EFI_SIMPLE_POINTER_STATE state;
		status = pProt->GetState(pProt, &state);

		if(status != EFI_SUCCESS) {
			continue;
		}

		d_printf("x: %d, y: %d, left %d, right %d\n", state.RelativeMovementX, state.RelativeMovementY, state.LeftButton, state.RightButton);
	}

	return 0;
}

