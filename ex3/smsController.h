#ifndef __SMS_CONTROLLER_H__
#define __SMS_CONTROLLER_H__

#include "smsController.h"
#include "TX/tx_api.h"
#include "common_defs.h"
#include "input_panel/input_panel.h"

/*
 * let the controller know that button pressed
 */
void controllerButtonPressed(const button b);

/*
 * let the controller know that packet has arrived
 */
void controllerPacketArrived(const uint8_t* pBuffer,const uint32_t size);

#endif

