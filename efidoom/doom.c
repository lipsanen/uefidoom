#include "doomdef.h"
#include "d_event.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "efi_utils.h"
#include "efi.h"
#include "doomgeneric.h"
#include "doomkeys.h"
#include "dlibc.h"
#include "x86.h"

static EFI_GRAPHICS_OUTPUT_PROTOCOL *pGraphics = NULL;
static EFI_SIMPLE_POINTER_PROTOCOL* pPointer = NULL;
static size_t WIDTH;
static size_t HEIGHT;
static uint8_t keyStateMap[258];
static uint8_t mouse_detected = 0;

void doomgeneric_Res(uint32_t *width, uint32_t *height)
{
	*width = WIDTH;
	*height = HEIGHT;
}

uint32_t DG_GetTicksMs()
{
	return clock_msec();
}

static void pressDoomKey(uint8_t pressed, unsigned int key)
{
	if(mouse_detected && keyStateMap[key] == pressed) {
		return;
	} else if(mouse_detected == 0) {
		pressed = !keyStateMap[key];
	}

	keyStateMap[key] = pressed;

	switch (key)
	{
	case 1:
	case 256: // Mouse 1
		DG_AddKeyToQueue(pressed, KEY_UPARROW);
		break;
	case 2:
	case 257: // Mouse 2
		DG_AddKeyToQueue(pressed, KEY_DOWNARROW);
		break;
	case 3:
		DG_AddKeyToQueue(pressed, KEY_RIGHTARROW);
		break;
	case 4:
		DG_AddKeyToQueue(pressed, KEY_LEFTARROW);
		break;
	case 9:
		DG_AddKeyToQueue(pressed, KEY_FIRE);
		break;
	case 13:
		DG_AddKeyToQueue(pressed, KEY_ENTER);
		break;
	case 32:
		DG_AddKeyToQueue(pressed, KEY_USE);
		break;
	default:
		break;
	}
}

static void ReadKeys()
{
	EFI_STATUS status;
	EFI_INPUT_KEY key;

	while (true)
	{
		EFI_INPUT_KEY key;
		status = g_pSystemTable->ConIn->ReadKeyStroke(g_pSystemTable->ConIn, &key);

		if (status != EFI_SUCCESS)
			break;

		pressDoomKey(1, key.ScanCode ? key.ScanCode : key.UnicodeChar);
	}
}

static void ResetPressedKeys()
{
	// In keyboard only mode we just toggle
	if(!mouse_detected) {
		return;
	}

	// Does not affect the mouse buttons, for those we have key release state
	for(size_t i=0; i < 256; ++i) {
		if(keyStateMap[i] == 1) {
			pressDoomKey(0, i);
		}
	}
}

static int32_t mouse_lowest_dx = INT32_MAX;

static void ReadMouse()
{
	if(pPointer == NULL) {
		return;
	}

	EFI_SIMPLE_POINTER_STATE state;
	EFI_STATUS status = pPointer->GetState(pPointer, &state);

	if(status != EFI_SUCCESS) {
		return;
	}

	// We have seen mouse movement, change how keyboard input works :D
	mouse_detected = 1;

	// No idea what the units provided by UEFI are
	// My laptop reported multiples of 8192 O_O
	// Therefore make this code self-calibrating
	int32_t absMov = d_abs(state.RelativeMovementX);

	if(absMov < mouse_lowest_dx && absMov > 0) {
		mouse_lowest_dx = absMov;
	}

	//    data1: Bitfield of buttons currently held down.
    //           (bit 0 = left; bit 1 = right; bit 2 = middle).
    //    data2: X axis mouse movement (turn).
    //    data3: Y axis mouse movement (forward/backward).
	event_t event;
	d_memset(&event, 0, sizeof(event_t));
	event.type = ev_mouse;
	event.data2 = state.RelativeMovementX / mouse_lowest_dx;
	D_PostEvent(&event);
	pressDoomKey(state.LeftButton, 256);
	pressDoomKey(state.RightButton, 257);
}

void DG_DrawFrame()
{
	ResetPressedKeys();
	ReadKeys();
	ReadMouse();
	pGraphics->Blt(pGraphics, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)DG_ScreenBuffer, EfiBltBufferToVideo, 0, 0, 0, 0, WIDTH, HEIGHT, 0);
}

EFI_STATUS efi_main(
	EFI_HANDLE handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_BOOT_SERVICES *BS = system_table->BootServices;
	Init(system_table);
	calibrate_cpu();

	status = system_table->ConOut->ClearScreen(system_table->ConOut);
	if (status != 0)
		return status;

	EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	status = LibLocateProtocol(BS, &gEfiGraphicsOutputProtocolGuid, (void **)&pGraphics);

	if (status != 0)
		return status;

	EFI_GUID gPointerGuid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
	LibLocateProtocol(BS, &gPointerGuid, (void**)&pPointer);

	WIDTH = pGraphics->Mode->Info->HorizontalResolution;
	HEIGHT = pGraphics->Mode->Info->VerticalResolution;

	int argc = 1;
	char *argv[] = {"efidoom"};
	doom_data_t doom;
	doomdata_init(&doom);
	doomgeneric_Create(argc, argv);
	d_memset(keyStateMap, 0, sizeof(keyStateMap));

	for (int i = 0;; i++)
	{
		doomgeneric_Tick(&doom);
	}

	while (1)
		;

	return 0;
}
