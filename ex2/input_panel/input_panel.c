#include "input_panel.h"
#include "defs.h"

#define INPUT_PANEL_BASE_ADDR 0x1E0
//pack the struct with no internal spaces
#pragma pack(1)
typedef struct
{
	uint32_t PADSTAT;
	uint32_t PADIER;
}volatile input_panelRegs;
#pragma pack()
