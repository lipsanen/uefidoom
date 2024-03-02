#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdint.h>

struct doom_data_t_;

void doomgeneric_Res(uint32_t* width, uint32_t* height);
void doomgeneric_Create(struct doom_data_t_* doom, int argc, char **argv);
void doomgeneric_Tick(struct doom_data_t_* doom);


//Implement below functions for your platform
void DG_Init();
void DG_DrawFrame();
int DG_GetKey(int* pressed, unsigned char* key);

#endif //DOOM_GENERIC
