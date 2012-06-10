#include "smsView.h"

#define SCREEN_HEIGHT 18
#define BOTTOM
//we want to refresh the screen at the first time
bool gViewRefreshScreen = true;
TX_EVENT_FLAGS_GROUP *gViewGuiThreadEventsFlag = NULL;

void viewSetRefreshScreen()
{
	gViewRefreshScreen = true;
}

int viewGetScreenHeight(const screen_type screen)
{

	switch(screen)
	{
		case MESSAGE_LISTING_SCREEN:
		case MESSAGE_EDIT_SCREEN:
		case MESSAGE_NUMBER_SCREEN:
			return SCREEN_HEIGHT-1;
			break;
		case MESSAGE_DISPLAY_SCREEN:
			return  SCREEN_HEIGHT- 3;
	}
	
	DBG_ASSERT(false);
	return SCREEN_HEIGHT;
}

void viewSetGuiThreadEventsFlag(TX_EVENT_FLAGS_GROUP *pGuiThreadEventsFlag)
{
	DBG_ASSERT(pGuiThreadEventsFlag!=NULL);
	gViewGuiThreadEventsFlag = pGuiThreadEventsFlag;
}

void viewSignal()
{
	//signal the first bit at the events flag
}

void viewRefresh()
{
	//according the current screen, refresh and current operation refresh the display
}

