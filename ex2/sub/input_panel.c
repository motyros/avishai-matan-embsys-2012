#include "input_panel.h"
#include "common_defs.h"
#include "dbg_assert.h"

#define INPUT_PANEL_PADSTAT_ADDR 0x1E0
#define INPUT_PANEL_PADIER_ADDR 0x1E1

#define INPUT_PANEL_ENABLE_VAL 0xFFFFFFFF
#define INPUT_PANEL_DISABLE_VAL 0x0
#define INPUT_PANEL_NUM_OF_BUTTONS 13
#define BIT0 1

void (*gpButtonPressedCB)(button);

_Interrupt1 void buttonPressedISR()
{
  uint32_t buttonsPressed;
  
  //load the register
  buttonsPressed = _lr(INPUT_PANEL_PADSTAT_ADDR);

  //call the cb
  gpButtonPressedCB(buttonsPressed);
  
}

result_t ip_init(void (*button_pressed_cb)(button))
{
    if(!button_pressed_cb)
    {
        return NULL_POINTER;
    }

    //store the cb
    gpButtonPressedCB = button_pressed_cb;

    //disable the button interrupts
    ip_disable();

    return OPERATION_SUCCESS;

}

void ip_enable(void)
{
  _sr(INPUT_PANEL_ENABLE_VAL,INPUT_PANEL_PADIER_ADDR);
}

void ip_disable(void)
{
  _sr(INPUT_PANEL_DISABLE_VAL,INPUT_PANEL_PADIER_ADDR);
}
