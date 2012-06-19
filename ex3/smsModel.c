#include "smsModel.h"
#include "embsys_sms_protocol.h"


#define SMS_BLOCK_SIZE (sizeof(SMS_DELIVER))
#define BLOCK_OVERHEAD (sizeof(void*))

#define SMS_POOL_REAL_SIZE (MAX_NUM_SMS*(SMS_BLOCK_SIZE+BLOCK_OVERHEAD))

#define SMS_DB_BLOCK_SIZE (sizeof(SmsLinkNode))
#define SMS_DB_POOL_REAL_SIZE (MAX_NUM_SMS*(SMS_DB_BLOCK_SIZE+BLOCK_OVERHEAD))


//the block of memory for the pool with all the smses 
CHAR gSmsBlockPool[SMS_POOL_REAL_SIZE];
//the block of memory for the pool with the sms linked list data-base
CHAR gSmsDbBlockPool[SMS_DB_POOL_REAL_SIZE];

// a sms linked list
typedef struct
{
        SmsLinkNodePtr  pHead;
        SmsLinkNodePtr  pTail;
        int             size;
}SmsLinkedList;

//the sms linked list data-base
SmsLinkedList gSmsDb;

//the current screen on the LCD disply
screen_type gCurrentScreen;

//the selected sms on the screen
SmsLinkNodePtr gpSelectedSms;

//the first sms on the screen
SmsLinkNodePtr gpFirstSmsOnScreen;

//the last pressed button on the keypad
button gLastPressedButton;

//is the button in continuous mode
bool gIsContinuousButtonPress = false;

//the sms that is in edit
SMS_SUBMIT gInEditSms;

//the pool that stores all of the smses
TX_BLOCK_POOL gSmsPool;

//the pool that stores all the nodes of the sms linked list
TX_BLOCK_POOL gSmsLinkListPool;

//the lock on the smsModel
TX_MUTEX gModelMutex;

UINT modelInit()
{
        UINT status;
		//create a pool that stores all the sms
        status = tx_mutex_create(&gModelMutex,"Model Mutex",TX_INHERIT);
        if (status != TX_SUCCESS)
        {
                return status;
        }

        /* Create a memory pool whose total size is for 100 SMS (the overhead
         * of the block is sizeof(void *).
         * starting at address 'gSmsBlockPool'. Each block in this
         * pool is defined to be SMS_BLOCK_SIZE==sizeof(SMS_DELIVER) bytes long.
         */
        status = tx_block_pool_create(&gSmsPool, "SmsPool",
                        SMS_BLOCK_SIZE, gSmsBlockPool, SMS_POOL_REAL_SIZE);
        /* If status equals TX_SUCCESS, gSmsPool contains 100
         * memory blocks of SMS_BLOCK_SIZE bytes each. The reason
         * there are not more/less blocks in the pool is
         * because of the one overhead pointer associated with each
         * block.
         */
        if(status != TX_SUCCESS) return status;

		//create a pool that stores all the nodes of the sms linked list
        /* Create a memory pool whose total size is for 100 nodes (the overhead
         * of the block is sizeof(void *).
         * starting at address 'gSmsDbBlockPool'. Each block in this
         * pool is defined to be SMS_DB_BLOCK_SIZE==sizeof(SmsLinkNode) bytes long.
         */		
        status = tx_block_pool_create(&gSmsLinkListPool, "SmsLinkListPool",
                        SMS_DB_BLOCK_SIZE, gSmsDbBlockPool, SMS_DB_POOL_REAL_SIZE);

        //if didn't success delete the first pool
        if(status != TX_SUCCESS)
        {
                tx_block_pool_delete(&gSmsPool);
                return status;
        }

		//init the list to empty
        gSmsDb.pHead = NULL;
        gSmsDb.pTail = NULL;
        gSmsDb.size = 0;

        return TX_SUCCESS;

}

screen_type modelGetCurentScreenType()
{
        return gCurrentScreen;
}

void modelSetCurrentScreenType(screen_type screen)
{
        gCurrentScreen = screen;
}

button modelGetLastButton()
{
        return gLastPressedButton;
}

void modelSetLastButton(const button b)
{
        gLastPressedButton = b;
}

void modelSetIsContinuousButtonPress(bool status)
{
        gIsContinuousButtonPress = status;
}

bool modelIsContinuousButtonPress()
{
        return gIsContinuousButtonPress;
}

//updates the head and tail to point to each other
//to have a cyclic list
void updateHeadAndTailCyclic()
{
    gSmsDb.pHead->pPrev = gSmsDb.pTail;
    gSmsDb.pTail->pNext = gSmsDb.pHead;
}


UINT modelAddSmsToDb(void* pSms,const message_type type)
{
        DBG_ASSERT(pSms != NULL);

        UINT status;

        SmsLinkNodePtr pNewSms;


        //allocate the linked list node
        status = tx_block_allocate(&gSmsLinkListPool, (VOID**) &pNewSms,TX_NO_WAIT);
        /* If status equals TX_SUCCESS, pNewNode contains the
         * address of the allocated block of memory.
         */

        if(status != TX_SUCCESS) return status;


        /* Allocate a memory block from SmsPool.*/
        status = tx_block_allocate(&gSmsPool, (VOID**) &pNewSms->pSMS,TX_NO_WAIT);
        /* If status equals TX_SUCCESS, pSms contains the
         * address of the allocated block of memory.
         */
        if(status != TX_SUCCESS)
                {
            tx_block_release(pNewSms);
                return status;
                }

        pNewSms->pNext = NULL;
        pNewSms->pPrev = gSmsDb.pTail;
        pNewSms->type = type;

        memcpy(pNewSms->pSMS,pSms,SMS_BLOCK_SIZE);

        if(gSmsDb.size == 0)
        {
                DBG_ASSERT(gSmsDb.pHead == NULL);
                DBG_ASSERT(gSmsDb.pTail == NULL);

                //it is a cyclic list
                pNewSms->pNext = pNewSms;
                pNewSms->pPrev = pNewSms;
                //the first node in the list - the head+tail
                gSmsDb.pHead = pNewSms;
                gSmsDb.pTail = pNewSms;
        }
        else if(gSmsDb.size == 1)
        {
                DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);

                //the second node in the list - add to the tail (that is one after the head)
                gSmsDb.pTail = pNewSms;
                gSmsDb.pHead->pNext = gSmsDb.pTail;
                gSmsDb.pTail->pPrev = gSmsDb.pHead;

                //it is a cyclic list
                updateHeadAndTailCyclic();

        }
        else
        {
                //update the tail of the linked list
                gSmsDb.pTail->pNext = pNewSms;
                pNewSms->pPrev = gSmsDb.pTail;
                gSmsDb.pTail = pNewSms;

                //it is a cyclic list
                updateHeadAndTailCyclic();

        }

        //increase the size of the linked list
        gSmsDb.size++;
        return status;
}

TX_STATUS modelRemoveSmsFromDb(const SmsLinkNodePtr pSms)
{

        DBG_ASSERT(pSms != NULL);

        TX_STATUS status;

        if(gSmsDb.size == 0)
        {
                DBG_ASSERT(gSmsDb.pHead == NULL);
                DBG_ASSERT(gSmsDb.pTail == NULL);

                return TX_PTR_ERROR;
        }

        /* Release a memory block back to the pool. */
        status = tx_block_release(pSms->pSMS);
        /* If status equals TX_SUCCESS, the block of memory pointed
         * to by pSms->pSMS has been returned to the pool.
         */

        //if(status != TX_SUCCESS) return status;

        //if it is the last node
        if(gSmsDb.size == 1)
        {
                DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);
                gSmsDb.pHead = NULL;
                gSmsDb.pTail = NULL;
        }

        //if the node is the head
        else if(pSms == gSmsDb.pHead)
        {
                gSmsDb.pHead = pSms->pNext;
                gSmsDb.pHead->pPrev = NULL;

                //it is a cyclic list
                updateHeadAndTailCyclic();
        }

        //if the node is the tail
        else if(pSms == gSmsDb.pTail)
        {
                gSmsDb.pTail = pSms->pPrev;
                gSmsDb.pTail->pNext = NULL;

                //it is a cyclic list
                updateHeadAndTailCyclic();
        }
        else
        {
                        SmsLinkNodePtr pPrevNode = pSms->pPrev;
                pPrevNode->pNext = pSms->pNext;
                pSms->pNext->pPrev = pPrevNode;
        }
        status = tx_block_release(pSms);

        //if(status != TX_SUCCESS) return status;

        //Decrease the size of the list
        gSmsDb.size--;
        return status;
}

int modelGetSmsSerialNumber(const SmsLinkNodePtr pSms)
{
        DBG_ASSERT(pSms != NULL);
                DBG_ASSERT(gSmsDb.size > 0);
                //if the pointer points to the end of the list
                //note: this check is needed, because the loop does not checks the tail.
                if(pSms == gSmsDb.pTail)
                {
                                return gSmsDb.size;
                }
                int serialNum = 1;
                SmsLinkNodePtr pNode;
                //go over the list to indicate the serial number
                for(pNode = gSmsDb.pHead ; pNode->pNext != NULL ; pNode = pNode->pNext)
                {
                                if(pNode == pSms) return serialNum;
                                ++serialNum;
                }
                
                //if the sms pointer was not found
                return -1;
                
}

SMS_SUBMIT* modelGetInEditSms()
{
        return &gInEditSms;
}


SmsLinkNodePtr modelGetFirstSmsOnScreen()
{
        return gpFirstSmsOnScreen;
}

void modelSetFirstSmsOnScreen(const SmsLinkNodePtr pSms)
{
        gpFirstSmsOnScreen = pSms;
}

SmsLinkNodePtr modelGetSelectedSms()
{
        return gpSelectedSms;
}

void modelSetSelectedSms(const SmsLinkNodePtr pSms)
{
        gpSelectedSms = pSms;
}


SmsLinkNodePtr modelGetFirstSms()
{
        return gSmsDb.pHead;
}

UINT modelGetSmsDbSize()
{
        return gSmsDb.size;
}

TX_STATUS modelAcquireLock()
{
        return tx_mutex_get(&gModelMutex,TX_WAIT_FOREVER);
}

TX_STATUS modelReleaseLock()
{
        return tx_mutex_put(&gModelMutex);
}



