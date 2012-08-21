#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "network/network.h"
#include "smsController.h"
#include "smsView.h"
#include "smsModel.h"
#include "timer/timer.h"
#include "time.h"
#include "common_defs.h"
#include "embsys_sms_protocol_mine.h"

////////////////////////////////////////////////////////////////////
///////////////////// Defines & Declaration ////////////////////////
////////////////////////////////////////////////////////////////////

//**************** Times ***************//
#define KEY_PAD_TIMER_DURATION (150/TX_TICK_MS)
#define NETWORK_PERIODIC_SEND_DURATION (1000/TX_TICK_MS)

//******** Threads Stack Sizes *********//
#define KEY_PAD_THREAD_STACK_SIZE (1024)
#define NETWORK_SEND_THREAD_STACK_SIZE (1024)
#define NETWORK_RECEIVE_THREAD_STACK_SIZE (1024)
#define NETWORK_SEND_THREAD_STACK_SIZE (1024)

//********* Threads priorities *********//
#define KEY_PAD_PRIORITY (1)
#define NETWORK_PRIORITY (1)

//************* Const Sizes ************//
#define PROBE_MESSAGE_SIZE 8
#define PROBE_MESSAGE_MAX_SIZE 22
#define NET_BUF_SIZE 4

//*********** Const Variables **********//
#define DEVICE_ID "06664120"

//*********** Mocro Functions **********//
#define ARRAY_CHAR_LEN(a) (sizeof(a)/sizeof(CHAR))

//network send event flag values
typedef enum
{
        SEND_PROBE = 1,
        SEND_PROBE_ACK = 2,
} NetworkSendFlags;


//********** FWD Declaration ***********//
//** the documentation is in the code **//
void keyPadThreadMainFunc(ULONG v);
void networkReceiveThreadMainFun(ULONG v);
void networkSendThreadMainFunc(ULONG v);
void eraseEditSmsFields();
void updateModelFieldsAfterAdd();
void sendEditSms();
void handleAddCharToEditSms(const char buttonX[]);
void deleteSms(const SmsLinkNodePtr pFirstSms,const SmsLinkNodePtr pSelectedSms);
void handleNumberScreen(button but);
void handleEditScreen(button but);
void controllerPacketArrived();
void continuousButtonPressTimerCB(ULONG v);
CHAR getNumberFromButton(button but);
void deleteSmsFromScreen(const SmsLinkNodePtr pFirstSms,const SmsLinkNodePtr pSelectedSms);


////////////////////////////////////////////////////////////////////
//////////////////// Threads global variables //////////////////////
////////////////////////////////////////////////////////////////////

//keypad Thread
TX_THREAD gKeyPadThread;
CHAR gKeyPadThreadStack[KEY_PAD_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gKeyPadEventFlags;

//network received thread
TX_THREAD gNetworkReceiveThread;
CHAR gNetworkReceiveThreadStack[NETWORK_RECEIVE_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gNetworkReceiveEventFlags;

//network send thread
TX_THREAD gNetworkSendThread;
CHAR gNetworkSendThreadStack[NETWORK_SEND_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gNetworkSendEventFlags;


////////////////////////////////////////////////////////////////////
////////////////////// other global variables //////////////////////
////////////////////////////////////////////////////////////////////

//********** arrays for the buttons keypad ***********//
const CHAR gButton1[] = {'.',',','?','1'};
const CHAR gButton2[] = {'a','b','c','2'};
const CHAR gButton3[] = {'d','e','f','3'};
const CHAR gButton4[] = {'g','h','i','4'};
const CHAR gButton5[] = {'j','k','l','5'};
const CHAR gButton6[] = {'m','n','o','6'};
const CHAR gButton7[] = {'p','q','r','s','7'};
const CHAR gButton8[] = {'t','u','v','8'};
const CHAR gButton9[] = {'w','x','y','z','9'};
const CHAR gButton0[] = {' ','0'};

//the number of clicks on the same button in the continuos time
int gInEditContinuosNum;

//the length of the recipient_id in the edited sms
int gInEditRecipientIdLen;

//the timer of the ContinuousButtonPress
TX_TIMER gContinuousButtonPressTimer;

//the timer of the PeriodicSend
TX_TIMER gPeriodicSendTimer;

//the mutex if the ProbeAck
TX_MUTEX gProbeAckMutex;

//the probe sms
SMS_PROBE gSmsProbe;

//the button that latest pressed
button gPressedButton;

//********** buffers to handel the network ***********//
desc_t gTranBuf[NET_BUF_SIZE];
desc_t gRecBuf[NET_BUF_SIZE];
uint8_t gRecData[NET_BUF_SIZE*NETWORK_MAXIMUM_TRANSMISSION_UNIT];
char gSendBuf[NETWORK_MAXIMUM_TRANSMISSION_UNIT];
char gProbeBuf[PROBE_MESSAGE_MAX_SIZE];

//buffer for a packet that latest received
uint8_t gPacketReceivedBuffer[NETWORK_MAXIMUM_TRANSMISSION_UNIT];


////////////////////////////////////////////////////////////////////
////////////////////// Call Back Functions /////////////////////////
////////////////////////////////////////////////////////////////////
/*
 * call back when the periodic probe timer alerted
 */
void periodicSendTimerCB(ULONG v)
{

        //get the lock on the var which indicates if we need to ack or just probe
        //signal the thread to send probe
        tx_event_flags_set(&gNetworkSendEventFlags,SEND_PROBE,TX_OR);
}

/*
 * call back when the continuous button press timer alerted
 */
void continuousButtonPressTimerCB(ULONG v)
{
        modelSetIsContinuousButtonPress(false);
}

/*
 * call back when a button was pressed
 */
void buttonPressedCB(button b)
{
        gPressedButton = b;
        //wake up the keyPad thread
        tx_event_flags_set(&gKeyPadEventFlags,1,TX_OR);
}

/*
 * call back when a packet was received.
 */
void networkReceiveCB(uint8_t buffer[], uint32_t size, uint32_t length)
{

        DBG_ASSERT(size>=length);
        DBG_ASSERT(length<=NETWORK_MAXIMUM_TRANSMISSION_UNIT);

        //stop the periodic timer to avoid recv of multiple copies of the
        //same SMS if we send another probe message and not probe ack
        tx_timer_deactivate(&gPeriodicSendTimer);

        //copy the packet to the global
        memcpy(gPacketReceivedBuffer,buffer,length);

        //wake up the networkReceive thread
        tx_event_flags_set(&gNetworkReceiveEventFlags,1,TX_OR);
}

/*
 * call back when transmission done
 */
void txDoneCB(const uint8_t *buffer, uint32_t size)
{
}

/*
 * call back when a packet was dropped during receiving.
 */
void rxPacketDropCB(packet_dropped_reason_t)
{
}

/*
 * call back when a packet was dropped during transmission.
 */
void txPacketDropCB(transmit_error_reason_t,
                                           uint8_t *buffer,
                                           uint32_t size,
                                           uint32_t length )
{
}


////////////////////////////////////////////////////////////////////
////////////////////// Initialize Controller ///////////////////////
////////////////////////////////////////////////////////////////////

/*
 * init the network
 */
result_t networkInit()
{
        result_t result;
        int i;

        network_call_back_t callBacks;
        network_init_params_t initParams;

        callBacks.dropped_cb = rxPacketDropCB;
        callBacks.recieved_cb = networkReceiveCB;
        callBacks.transmit_error_cb = txPacketDropCB;
        callBacks.transmitted_cb = txDoneCB;

        initParams.list_call_backs = callBacks;

        for(i = 0 ; i < NET_BUF_SIZE ; ++i)
        {
            gRecBuf[i].pBuffer = (uint32_t)(gRecData+i*NETWORK_MAXIMUM_TRANSMISSION_UNIT);
            gRecBuf[i].buff_size = NETWORK_MAXIMUM_TRANSMISSION_UNIT;
        }

        initParams.transmit_buffer = gTranBuf;
        initParams.size_t_buffer = NET_BUF_SIZE;
        initParams.recieve_buffer = gRecBuf;
        initParams.size_r_buffer = NET_BUF_SIZE;

        result = network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);
        if(result != OPERATION_SUCCESS)
            return result;

        return network_init(&initParams);
}

/*
 * creates the timers
 */
TX_STATUS createTimers()
{
        TX_STATUS status;
        status = tx_timer_create(       &gContinuousButtonPressTimer,
                                                                "Continuous Button Press Timer",
                                                                continuousButtonPressTimerCB,
                                                                0,
                                                                KEY_PAD_TIMER_DURATION,
                                                                0,
                                                                TX_NO_ACTIVATE);
        if(status != TX_SUCCESS) return status;

        status = tx_timer_create(       &gPeriodicSendTimer,
                                                                "Periodic send timer",
                                                                periodicSendTimerCB,
                                                                0,
                                                                NETWORK_PERIODIC_SEND_DURATION,
                                                                NETWORK_PERIODIC_SEND_DURATION,
                                                                TX_AUTO_ACTIVATE);
        return status;

}

/*
 * creates the threads
 */
TX_STATUS createThreads()
{
        TX_STATUS status;
        status = tx_thread_create(      &gKeyPadThread,
                                                                "Keypad Thread",
                                                                keyPadThreadMainFunc,
                                                                0,
                                                                gKeyPadThreadStack,
                                                                KEY_PAD_THREAD_STACK_SIZE,
                                                                KEY_PAD_PRIORITY,
                                                                KEY_PAD_PRIORITY,
                                                                TX_NO_TIME_SLICE, TX_AUTO_START
                                                                );
        if(status != TX_SUCCESS) return status;

        status = tx_thread_create(      &gNetworkReceiveThread,
                                                                "Network Receive Thread",
                                                                networkReceiveThreadMainFun,
                                                                0,
                                                                gNetworkReceiveThreadStack,
                                                                NETWORK_RECEIVE_THREAD_STACK_SIZE,
                                                                NETWORK_PRIORITY,
                                                                NETWORK_PRIORITY,
                                                                TX_NO_TIME_SLICE, TX_AUTO_START
                                                                );
        if(status != TX_SUCCESS) return status;

        status = tx_thread_create(      &gNetworkSendThread,
                                                                "Network Send Thread",
                                                                networkSendThreadMainFunc,
                                                                0,
                                                                gNetworkSendThreadStack,
                                                                NETWORK_SEND_THREAD_STACK_SIZE,
                                                                NETWORK_PRIORITY,
                                                                NETWORK_PRIORITY,
                                                                TX_NO_TIME_SLICE, TX_AUTO_START
                                                                );
        return status;

}

/*
 * creates the event flags
 */
TX_STATUS createEventFlags()
{
        TX_STATUS status;
        status = tx_event_flags_create(&gKeyPadEventFlags,"Keypad Event Flags");
        if(status != TX_SUCCESS) return status;

        status = tx_event_flags_create(&gNetworkReceiveEventFlags,"Network Rec Event Flags");
        if(status != TX_SUCCESS) return status;

        status = tx_event_flags_create(&gNetworkSendEventFlags,"Network Send Event Flags");
        return status;
}

/*
 * init the controller
 */
TX_STATUS controllerInit()
{
        gInEditRecipientIdLen = 0;
        gInEditContinuosNum = 0;
        /* create three threads
         *
         * 1. will listed to the keypad
         * 2. will listed to the network: received_cb
         * 3. will send data to the network: transmitted_cb, error_cb, dropped_cb
         */

        TX_STATUS status;

        //create the timers
        status = createTimers();
        if(status != TX_SUCCESS) return status;

        //create the threads
        status = createThreads();
        if(status != TX_SUCCESS) return status;

        //create the event flags
        status = createEventFlags();
        if(status != TX_SUCCESS) return status;


        //create the mutex
        status = tx_mutex_create(&gProbeAckMutex,"Probe Ack Mutex",TX_INHERIT);
        if(status != TX_SUCCESS) return status;

        //add the device id to the prob sms
        memcpy(gSmsProbe.device_id,DEVICE_ID,strlen(DEVICE_ID));

        result_t result;

        //init the key pad
        result = ip_init(buttonPressedCB);

        if(result != OPERATION_SUCCESS)
        {
                return TX_PTR_ERROR;
        }

        ip_enable();

        //note: no need to take any lock since all init are single threaded
        modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
        //update the first sms on screen
        modelSetFirstSmsOnScreen(modelGetFirstSms());
        //update the selected sms
        modelSetSelectedSms(modelGetFirstSms());

        //init the network
        result = networkInit();
        if(result != OPERATION_SUCCESS)
        {
                return TX_START_ERROR;
        }

        viewSetRefreshScreen();
        viewSignal();

        return TX_SUCCESS;
}


////////////////////////////////////////////////////////////////////
///////////////////// Network Receive Thread ///////////////////////
////////////////////////////////////////////////////////////////////
/*
 * the main function for the networkReceive thread
 */
void networkReceiveThreadMainFun(ULONG v)
{
        ULONG actualFlag;
        while(true)
        {
                //wait for a flag wake the thread up
                tx_event_flags_get(&gNetworkReceiveEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);

                //handle the packet that arrived
                controllerPacketArrived();

                tx_timer_activate(&gPeriodicSendTimer);
        }
}

/*
 * let the controller know that packet has arrived
 */
void controllerPacketArrived()
{
        EMBSYS_STATUS status;

        //try to parse to submit ack;
        SMS_SUBMIT_ACK submitAck;

        do
        {
                status = embsys_parse_submit_ack((char*)gPacketReceivedBuffer,&submitAck);
                if(status == SUCCESS) break;

                //try to parse to deliver sms
                SMS_DELIVER deliverSms;
                status = embsys_parse_deliver((char*)gPacketReceivedBuffer,&deliverSms);
                if(status != SUCCESS) break;

                //in case it is a delivered sms, add it to the DB
                if(modelAcquireLock() != TX_SUCCESS)
                {
                        break;
                }

                //add the delivered sms to the liked list
                modelAddSmsToDb(&deliverSms,INCOMMING_MESSAGE);

                //update the relevant fields
                updateModelFieldsAfterAdd();

                //release the lock
                modelReleaseLock();

                //copy the sender id, for the probe ack
                memcpy(gSmsProbe.sender_id,deliverSms.sender_id,ID_MAX_LENGTH);
                memcpy(gSmsProbe.timestamp,deliverSms.timestamp,TIMESTAMP_MAX_LENGTH);

                //signal the send thread to send probe akc
                tx_event_flags_set(&gNetworkSendEventFlags,SEND_PROBE_ACK,TX_OR);

                //in case the screen is on the message listing, refresh the screen
                if(modelGetCurentScreenType() == MESSAGE_LISTING_SCREEN)
                {
                        viewSetRefreshScreen();
                        viewSignal();
                }
        }while(false);

}

/*
 * updates the model fields if needed.
 */
void updateModelFieldsAfterAdd()
{
    //if it is the first sms, update the relevant fields
    if (modelGetSmsDbSize() == 1)
	{
			modelSetFirstSmsOnScreen(modelGetFirstSms());
			modelSetSelectedSms(modelGetFirstSms());
	}
	else
	{
		SmsLinkNodePtr firstSmsOnScreen = modelGetFirstSmsOnScreen();
		 if(viewIsLastRowSelected()
				&& modelGetSmsSerialNumber(firstSmsOnScreen) > modelGetSmsSerialNumber(modelGetSelectedSms()))
		 {
			 modelSetFirstSmsOnScreen(firstSmsOnScreen->pNext);
		 }
		 modelGetSelectedSms();
	}
}


////////////////////////////////////////////////////////////////////
/////////////////////// Network Send Thread ////////////////////////
////////////////////////////////////////////////////////////////////
/*
 * the main function for the networkSend thread
 */
void networkSendThreadMainFunc(ULONG v)
{
        ULONG actualFlag;

        while(true)
        {
                //wait for a flag wake the thread up
                tx_event_flags_get(&gNetworkSendEventFlags,SEND_PROBE|SEND_PROBE_ACK,TX_OR_CLEAR,&actualFlag,TX_WAIT_FOREVER);

                UINT bufLen = 0;

                embsys_fill_probe(gProbeBuf,&gSmsProbe,actualFlag & SEND_PROBE_ACK,&bufLen);
                network_send_packet_start((uint8_t*)gProbeBuf,bufLen,bufLen);
        }
}


////////////////////////////////////////////////////////////////////
////////////////////////// KeyPad Thread ///////////////////////////
////////////////////////////////////////////////////////////////////
/*
 * the main function for the keyPad thread
 */
void keyPadThreadMainFunc(ULONG v)
{
        ULONG actualFlag;

        while(true)
        {
                //wait for a flag wake the thread up
                tx_event_flags_get(&gKeyPadEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);

                //handle the button that pressed
                controllerButtonPressed(gPressedButton);
        }
}

/*
 * handles a pressed button on the Display Screen
 */
void handleDisplayScreen(button b)
{
        bool needToRefershGui = false;
        switch (b)
        {
        case BUTTON_STAR: //go back to sms listing screen
                modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
                needToRefershGui = true;
                break;
        case BUTTON_NUMBER_SIGN: //delete current sms
                modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
                SmsLinkNodePtr pSelectedSms = modelGetSelectedSms();
                SmsLinkNodePtr pFirstSms = modelGetFirstSmsOnScreen();

                //delete the sms from screen and from db
                deleteSms(pFirstSms,pSelectedSms);
                needToRefershGui = true;
                break;
        }

        if (needToRefershGui)
        {
                viewSetRefreshScreen();
                viewSignal();
        }

}

/*
 * handles a pressed button on the Edit Screen
 */
void handleEditScreen(button but)
{
        if(but!=modelGetLastButton() || !modelIsContinuousButtonPress())
        {
                gInEditContinuosNum = 0;
        }
        else
        {
                gInEditContinuosNum++;
        }
        SMS_SUBMIT* inEditSms = modelGetInEditSms();
        switch (but)
        {
        case BUTTON_OK: //go to the Number Screen
                gInEditRecipientIdLen = 0;
                //clean the recipient_id
                memset(inEditSms->recipient_id ,0, ID_MAX_LENGTH);
                modelSetCurrentScreenType(MESSAGE_NUMBER_SCREEN);
                viewSetRefreshScreen();
                viewSignal();
                break;
        case BUTTON_STAR: //delete the edited sms and go to Listing Screen
            /* note: the edited sms will be deleted before the next time
             * the user in enter to the Edit Screen*/
                modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
                viewSetRefreshScreen();
                viewSignal();
                break;
        case BUTTON_NUMBER_SIGN: //delete the last char
                if(inEditSms->data_length > 0)
                {
                        inEditSms->data_length--;
                        viewSignal();
                }
                break;
        case BUTTON_1:
                handleAddCharToEditSms(gButton1);
                break;
        case BUTTON_2:
                handleAddCharToEditSms(gButton2);
                break;
        case BUTTON_3:
                handleAddCharToEditSms(gButton3);
                break;
        case BUTTON_4:
                handleAddCharToEditSms(gButton4);
                break;
        case BUTTON_5:
                handleAddCharToEditSms(gButton5);
                break;
        case BUTTON_6:
                handleAddCharToEditSms(gButton6);
                break;
        case BUTTON_7:
                handleAddCharToEditSms(gButton7);
                break;
        case BUTTON_8:
                handleAddCharToEditSms(gButton8);
                break;
        case BUTTON_9:
                handleAddCharToEditSms(gButton9);
                break;
        case BUTTON_0:
                handleAddCharToEditSms(gButton0);
                break;
        }
}

/*
 * handles the added char (by button) to the edited sms
 */
void handleAddCharToEditSms(const char buttonX[])
{
        SMS_SUBMIT* inEditSms = modelGetInEditSms();
        if(gInEditContinuosNum > 0 || inEditSms->data_length == DATA_MAX_LENGTH)
        {
                //the specific char in this button according to the timer delay
                int numChar = gInEditContinuosNum % ARRAY_CHAR_LEN(buttonX);
                //replace the last char
                inEditSms->data[inEditSms->data_length-1] = buttonX[numChar];
        }
        else
        {
                //add char to the end of the sms
                inEditSms->data[inEditSms->data_length] = buttonX[0];
                inEditSms->data_length++;
        }

        //update the screen
        viewSignal();
}

/*
 * handles a pressed button on the Listing Screen
 */
void handleListingScreen(button b)
{
        //modelAcquireLock();

        SmsLinkNodePtr pSelectedSms = modelGetSelectedSms();
        SmsLinkNodePtr pFirstSms = modelGetFirstSmsOnScreen();

        bool refreshView = false;

        switch (b)
        {
        case BUTTON_2: //up

                //there are no messages to display
                if (pSelectedSms == NULL)
                {
                        break;
                }

                /* if first row is selected and we are moving up:
                 * 1. if we have more sms than the screen height, first line remain selected
                 *    and the first sms will be the predecessor
                 * 2. if all sms fit the screen, select the last sms
                 */
                if (viewIsFirstRowSelected())
                {
                        if (modelGetSmsDbSize() > viewGetScreenHeight(MESSAGE_LISTING_SCREEN))
                        {
                                modelSetFirstSmsOnScreen(pFirstSms->pPrev);
                        }
                }
                modelSetSelectedSms(pSelectedSms->pPrev);
                refreshView = true;
                break;
        case BUTTON_8: //down
                //there are no message to display
                if (pSelectedSms == NULL)
                {
                        break;
                }
                if (viewIsLastRowSelected())
                {
                        if (modelGetSmsDbSize() > viewGetScreenHeight(MESSAGE_LISTING_SCREEN))
                        {
                                modelSetFirstSmsOnScreen(pFirstSms->pNext);
                        }
                }
                modelSetSelectedSms(pSelectedSms->pNext);
                refreshView = true;
                break;

        case BUTTON_STAR: //enter new message screen

                //erase all fields of inEditSms
                eraseEditSmsFields();

                //if we don't have room for new sms, don't let write new one
                if (modelGetSmsDbSize() == MAX_NUM_SMS)
                {
                        break;
                }
                modelSetCurrentScreenType(MESSAGE_EDIT_SCREEN);
                viewSetRefreshScreen();
                refreshView = true;
                break;
        case BUTTON_NUMBER_SIGN: //delete message
                if (pSelectedSms == NULL) //no message to delete
                {
                        break;
                }
                deleteSms(pFirstSms,pSelectedSms);
                viewSetRefreshScreen();
                refreshView = true;
                break;
        case BUTTON_OK: //display message
                modelSetCurrentScreenType(MESSAGE_DISPLAY_SCREEN);
                viewSetRefreshScreen();
                refreshView = true;
                break;
        }

        //signal the view to refresh itself
        if (refreshView)
        {
                viewSignal();
        }

}

/*
 * erase the edited sms fields
 */
void eraseEditSmsFields()
{
        SMS_SUBMIT* inEditSms = modelGetInEditSms();
        inEditSms->data_length = 0;
        memset(inEditSms->recipient_id ,0, ID_MAX_LENGTH);
        memset(inEditSms->device_id ,0, ID_MAX_LENGTH);
}

/*
 * delete the sms from the linked list data-base
 */
void deleteSms(const SmsLinkNodePtr pFirstSms,const SmsLinkNodePtr pSelectedSms)
{
        deleteSmsFromScreen(pFirstSms,pSelectedSms);
        modelRemoveSmsFromDb(pSelectedSms);
}

/*
 * delete the given selected sms from the screen
 */
void deleteSmsFromScreen(const SmsLinkNodePtr pFirstSms,const SmsLinkNodePtr pSelectedSms)
{
        //we are deleting the first sms in the screen
        if (viewIsFirstRowSelected())
        {
                //if this is the only sms
                if (modelGetSmsDbSize() == 1)
                {
                        //set the first on screen snd the selected to null
                        modelSetFirstSmsOnScreen(NULL);
                        modelSetSelectedSms(NULL);
                }
                else
                {
                        //set the selected
                        modelSetFirstSmsOnScreen(pFirstSms->pNext);
                }
        }

        //do it only if after the delete db is not empty
        if (modelGetSmsDbSize() > 1)
        {
                modelSetSelectedSms(pSelectedSms->pNext);
        }
}

/*
 * handles a pressed button on the Number Screen
 */
void handleNumberScreen(button but)
{
        SMS_SUBMIT* inEditSms = modelGetInEditSms();

        switch (but)
        {
        case BUTTON_OK: //send the edited sms
            //do nothing when no number presents
            if (gInEditRecipientIdLen == 0)
            {
                return;
            }    
            //TODO*************************************************/
            //int i;
            //for(i=0;i<50;++i){
           /*************************************************/

            sendEditSms();

            /*************************************************/

            //}
            /***************************************************/
            modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
            viewSetRefreshScreen();
            viewSignal();
            return;
        case BUTTON_STAR: //delete the edited sms and go to Listing Screen
                /* note: the edited sms will be deleted before the next time
                 * the user in enter to the Edit Screen*/
                 
                //update the gui
                modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
                viewSetRefreshScreen();
                viewSignal();
                return;
        case BUTTON_NUMBER_SIGN: //delete the last digit
                if (gInEditRecipientIdLen == 0)
                {
                        return;
                }

                //decrease the length
                gInEditRecipientIdLen--;

                //erase the last digit
                inEditSms->recipient_id[gInEditRecipientIdLen] = 0;
                viewSignal();
                return;

        }
        
        //handel all other buttons:
        
        if(gInEditRecipientIdLen == ID_MAX_LENGTH)
        {
                //replace the last digit
                inEditSms->recipient_id[gInEditRecipientIdLen-1] = getNumberFromButton(but);
        }
        else
        {
                //add the digit in the end
                inEditSms->recipient_id[gInEditRecipientIdLen] = getNumberFromButton(but);
                gInEditRecipientIdLen++;
        }

        //update the screen
        viewSignal();

}

/*
 * send the edited sms
 */
void sendEditSms()
{
        SMS_SUBMIT* smsToSend = modelGetInEditSms();
        unsigned dataLen;

        //reset the junk
        memset(smsToSend->device_id,0,ID_MAX_LENGTH);

        //add the device id to the edited sms
        memcpy(smsToSend->device_id,DEVICE_ID,strlen(DEVICE_ID));

        //fill the sms to send
        embsys_fill_submit(gSendBuf,smsToSend,&dataLen);

        //send the packet
        network_send_packet_start((uint8_t*)gSendBuf,dataLen,dataLen);

        //in case it is a delivered sms, add it to the DB
        if(modelAcquireLock() != TX_SUCCESS)
        {
        	return;
        }

        //add the sms to the linked list
        modelAddSmsToDb(smsToSend, OUTGOING_MESSAGE);

        //update the relevant fields
        updateModelFieldsAfterAdd();

        //release the lock
        modelReleaseLock();

}

/*
 * gets the number from the given button
 */
CHAR getNumberFromButton(button but)
{
        int counter = 0;
        while (but != 0)
        {
                but = but >> 1;
                counter++;
        }

        return '0' + (counter-1)%11;
}

/*
 * decide what to do according the cuurent screen, current state etc
 */
void controllerButtonPressed(const button b)
{
        //try to get the lock on the sms model
        if (modelAcquireLock() != TX_SUCCESS)
        {
                return;
        }
        
        //check which screen we are, and handel it
        switch (modelGetCurentScreenType())
        {
        case MESSAGE_DISPLAY_SCREEN:
                handleDisplayScreen(b);
                break;
        case MESSAGE_EDIT_SCREEN:
                handleEditScreen(b);
                break;
        case MESSAGE_LISTING_SCREEN:
                handleListingScreen(b);
                break;
        case MESSAGE_NUMBER_SCREEN:
                handleNumberScreen(b);
                break;
        }

        //update the relevant fields in the sms model
        modelSetLastButton(b);
        modelSetIsContinuousButtonPress(true);

        //stop the timer
        TX_STATUS status = tx_timer_deactivate(&gContinuousButtonPressTimer);

        //change timer expiration characteristics
        if(status == TX_SUCCESS)
        {
            status = tx_timer_change(&gContinuousButtonPressTimer,KEY_PAD_TIMER_DURATION,0);
        }

        //activate the timer
        if(status == TX_SUCCESS)
        {
        	status = tx_timer_activate(&gContinuousButtonPressTimer);
        }

        //release the lock
        modelReleaseLock();
}


