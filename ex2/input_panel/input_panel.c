#include "input_panel.h"
#include "common_defs.h"

#define INPUT_PANEL_PADSTAT_ADDR 0x1E0
#define INPUT_PANEL_PADIER_ADDR 0x1E1

#define INPUT_PANEL_ENABLE_VAL 0xFFFFFFFF
#define INPUT_PANEL_DISABLE_VAL 0x0
#define INPUT_PANEL_NUM_OF_BUTTONS 13
#define BIT0 1

void (*gpButtonPressedCB)(button);

_Interrupt1 void buttonPressedIsr()
{
  uint8_t i;
  uint32_t buttonsPressed;
  
  //load the register
  buttonsPressed = _lr(INPUT_PANEL_PADSTAT_ADDR);
  
  for(i = 0 ; i < INPUT_PANEL_NUM_OF_BUTTONS ; ++i)
  {
    //if the "i" button is pressed call the CB function
    if((buttonsPressed>>i) & BIT0)
    {
      gpButtonPressedCB(0x001 << i);
    }
  }
}

result_t ip_init(void (*button_pressed_cb)(button))
{
    if(!button_pressed_cb)
    {
        return NULL_POINTER;
    }

    gpButtonPressedCB = button_pressed_cb;
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
