#include "input_panel.h"
#include "common_defs.h"

#define INPUT_PANEL_PADSTAT_ADDR 0x1E0
#define INPUT_PANEL_PADIER_ADDR 0x1E1

void (*gpButtonPressedCB)(button);

void buttonPressedIsr()
{
  uint8_t i;
  uint32_t buttonsPressed;
  
  //load the register
  _lr(buttonsPressed,INPUT_PANEL_PADSTAT_ADDR);
  
  for(i=0;i<13;++i)
  {
    //if the "i" button is pressed call the CB function
    if((buttonsPressed>>i)&1)
    {
      gpButtonPressedCB(0x001  << i);
    }
  }
}

result_t ip_init(void (*button_pressed_cb)(button))
{
  gpButtonPressedCB = button_pressed_cb;
  ip_disable();
}

void ip_enable(void)
{
  _sr(0xFFFFFFFF,INPUT_PANEL_PADIER_ADDR);
}

void ip_disable(void)
{
  _sr(0x0,INPUT_PANEL_PADIER_ADDR);
}