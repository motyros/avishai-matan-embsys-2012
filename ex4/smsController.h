#ifndef __SMS_CONTROLLER_H__
#define __SMS_CONTROLLER_H__


#include "TX/tx_api.h"
#include "common_defs.h"
#include "input_panel/input_panel.h"

/*
 * init controller component
 */
TX_STATUS controllerInit();

/*
 * let the controller know that button pressed
 */
void controllerButtonPressed(const button b);

#endif

