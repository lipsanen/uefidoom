#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "efi_utils.h"
#include "efi.h"

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

	EFI_GRAPHICS_OUTPUT_PROTOCOL *pGraphics;
	status = LibLocateProtocol(BS, &gEfiGraphicsOutputProtocolGuid, (void **)&pGraphics);

	if (status != 0)
		return status;

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *buffer;

	size_t width = pGraphics->Mode->Info->HorizontalResolution;
	size_t height = pGraphics->Mode->Info->VerticalResolution;
	buffer = AllocatePool(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * width * height);

	for (size_t y = 0; y < height; ++y)
	{
		for (size_t x = 0; x < width; ++x)
		{
			buffer[y * width + x].Blue = 0;
			buffer[y * width + x].Green = y * 255 / height;
			buffer[y * width + x].Red = x * 255 / width;
		}
	}

	pGraphics->Blt(pGraphics, buffer, EfiBltBufferToVideo, 0, 0, 0, 0, width, height, 0);

	BS->FreePool(buffer);

	while (1)
		;

	return 0;
}
