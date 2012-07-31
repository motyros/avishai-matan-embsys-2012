/*
 * input_panel.h
 *
 * Descriptor: API of the input panel.
 *
 */

#ifndef INPUT_PANEL_H_
#define INPUT_PANEL_H_

#include "./common_defs.h"

// list of buttons that can be pressed on the input panel.
typedef enum
{
	BUTTON_OK 			= 	0x001,
	BUTTON_1 			= 	(0x001 << 1),
	BUTTON_2 			= 	(0x001 << 2),
	BUTTON_3 			= 	(0x001 << 3),
	BUTTON_4 			= 	(0x001 << 4),
	BUTTON_5 			= 	(0x001 << 5),
	BUTTON_6 			= 	(0x001 << 6),
	BUTTON_7 			= 	(0x001 << 7),
	BUTTON_8 			=	(0x001 << 8),
	BUTTON_9 			= 	(0x001 << 9),
	BUTTON_STAR 		        = 	(0x001 << 10),
	BUTTON_0 			= 	(0x001 << 11),
	BUTTON_NUMBER_SIGN 	        = 	(0x001 << 12),

}button;

/**********************************************************************
 *
 * Function:    ip_init
 *
 * Descriptor:  Initialize the input panel.
 *
 * Parameters:  button_pressed_cb: call back whenever a button was pressed.
 *
 * Notes:       This device should work by default with no interrupts enabled.
 *
 * Return:      OPERATION_SUCCESS:      Initialization done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *
 ***********************************************************************/
result_t ip_init(void (*button_pressed_cb)(button));

/**********************************************************************
 *
 * Function:    ip_enable
 *
 * Descriptor:  Enable interrupts when a button pressed.
 *
 * Notes:
 *
 * Return:
 *
 ***********************************************************************/
void ip_enable(void);

/**********************************************************************
 *
 * Function:    ip_disable
 *
 * Descriptor:  Disable interrupts when a button pressed.
 *
 * Notes:
 *
 * Return:
 *
 ***********************************************************************/
void ip_disable(void);

#endif /* INPUT_PANEL_H_ */
