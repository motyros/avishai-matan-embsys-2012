#ifndef __SMS_VIEW_H__
#define __SMS_VIEW_H__

#include "TX/tx_api.h"
#include "common_defs.h"

/*
 * set the refresh flag which indicate that the view should
 * refresh the whole screen.
 */
void viewSetRefreshScreen();

/*
 * return the free height of the given screen (taken out all reserved lines) 
 */
int viewGetScreenHeight(const screen_type screen);

/*
 * set the events flag to signal the gui thread to start working
 */
void viewSetGuiThreadEventsFlag(TX_EVENT_FLAGS_GROUP *pGuiThreadEventsFlag);

/*
 * signal the events flag to wake up the gui thread for refreshing the screen
 */
void viewSignal();

/*
 * instruct the view to refresh the screen in order to display new
 */
void viewRefresh();

#endif
