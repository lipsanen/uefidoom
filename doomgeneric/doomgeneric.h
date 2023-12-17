#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdint.h>

extern uint32_t* DG_ScreenBuffer;

void doomgeneric_Res(uint32_t* width, uint32_t* height);
void doomgeneric_Create(int argc, char **argv);
void doomgeneric_Tick();
void DG_AddKeyToQueue(int pressed, unsigned int keyCode);


//Implement below functions for your platform
void DG_Init();
void DG_DrawFrame();
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs();
int DG_GetKey(int* pressed, unsigned char* key);
void DG_SetWindowTitle(const char * title);

#endif //DOOM_GENERIC
