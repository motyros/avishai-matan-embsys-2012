#include "smsModel.h"
#include "embsys_sms_protocol.h"

typedef struct
{
        SmsLinkNodePtr  pHead;
        SmsLinkNodePtr  pTail;
        int                     size;
}SmsLinkedList;

SmsLinkedList gSmsDb;

screen_type gCurrentScreen;

SmsLinkNodePtr gpSelectedSms;

SmsLinkNodePtr gpFirstSmsOnScreen;

button gLastPreddedButton;

bool gIsContinuousButtonPress;

SMS_SUBMIT gInEditSms;

TX_BLOCK_POOL gSmsPool;

TX_BLOCK_POOL gSmsLinkListPool;

UINT modelInit()
{
        //create a pool that stores all the sms
        UINT status;
        /* Create a memory pool whose total size is for 100 SMS (the overhead
         * of the block is sizeof(void *).
         * starting at address 0x1000. Each block in this
         * pool is defined to be sizeof(SMS_SUBMIT) bytes long.
         */
        status = tx_block_pool_create(&gSmsPool, "SmsPool",
                        SMS_BLOCK_SIZE, (void *) 0x1000, (SMS_BLOCK_SIZE+ sizeof(void *))*100);
        /* If status equals TX_SUCCESS, gSmsPool contains 100
         * memory blocks of SMS_BLOCK_SIZE bytes each. The reason
         * there are not more/less blocks in the pool is
         * because of the one overhead pointer associated with each
         * block.
         */
        if(status != TX_SUCCESS) return status;

        status = tx_block_pool_create(&gSmsLinkListPool, "SmsLinkListPool",
                        sizeof(SmsLinkNode), (void *) 0x100000, (sizeof(SmsLinkNode)+sizeof(void *))*100);

        //if didn't success delete the first pool
        if(status != TX_SUCCESS)
        {
                tx_block_pool_delete(&gSmsPool);
                return status;
        }

        gSmsDb.pHead = null;
        gSmsDb.pTail = null;
        gSmsDb.size = 0;

}

screen_type modelGetCurentScreenType()
{
        return gCurrentScreen;
}

button modelGetLastButton()
{
        return gLastPreddedButton;
}

bool modelIsContinuousButtonPress();

UINT modelAddSmsToDb(void* pSms,const message_type type)
{
        DBG_ASSERT(pSms != null);

        UINT status;
        /* Allocate a memory block from SmsPool.*/
        status = tx_block_allocate(&gSmsPool, (VOID**) &pSms,TX_NO_WAIT);
        /* If status equals TX_SUCCESS, pSms contains the
         * address of the allocated block of memory.
         */
        if(status != TX_SUCCESS) return status;

        //create a new node
        SmsLinkNode newNode;
        newNode.pSMS = pSms;
        newNode.pNext = null;
        newNode.pPrev = gSmsDb.pTail;
        newNode.type = type;

        //create a pointer to the new node
        SmsLinkNodePtr pNewNode = &newNode;

        //allocate a block for the new node
        status = tx_block_allocate(&gSmsLinkListPool, (VOID**) &pNewNode,TX_NO_WAIT);
        /* If status equals TX_SUCCESS, pNewNode contains the
         * address of the allocated block of memory.
         */

        //if failed: release the first allocate
        if(status != TX_SUCCESS)
        {
                tx_block_release(pSms);
                return status;
        }

        if(gSmsDb.size == 0)
        {
                DBG_ASSERT(gSmsDb.pHead == null);
                DBG_ASSERT(gSmsDb.pTail == null);

                //the first node in the list - the head+tail
                gSmsDb.pHead = pNewNode;
                gSmsDb.pTail = pNewNode;
                gSmsDb.pHead->pPrev = null;

                //the tail points to null
                gSmsDb.pTail->pNext = null;
        }
        else if(gSmsDb.size == 1)
        {
                DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);

                //the second node in the list - add to the tail (that is one after the head)
                gSmsDb.pTail = pNewNode;
                gSmsDb.pHead->pNext = gSmsDb.pTail;
                gSmsDb.pTail->pPrev = gSmsDb.pHead;

                //the tail points to null
                gSmsDb.pTail->pNext = null;
        }
        else
        {
                //update the tail of the linked list
                gSmsDb.pTail->pNext = pNewNode;
                pNewNode->pPrev = gSmsDb.pTail;
                gSmsDb.pTail = pNewNode;

                //the tail points to null
                gSmsDb.pTail->pNext = null;
        }

        //increase the size of the linked list
        gSmsDb.size++;
        return status;
}

UINT modelRemoveSmsFromDb(const SmsLinkNodePtr pSms)
{

        DBG_ASSERT(pSms != null);

        UINT status;
        if(gSmsDb.size == 0)
        {
                DBG_ASSERT(gSmsDb.pHead == null);
                DBG_ASSERT(gSmsDb.pTail == null);

                return TX_PTR_ERROR;
        }

        /* Release a memory block back to the pool. */
        status = tx_block_release((VOID *) pSms->pSMS);
        /* If status equals TX_SUCCESS, the block of memory pointed
         * to by pSms->pSMS has been returned to the pool.
         */

        //if(status != TX_SUCCESS) return status;

        //if it is the last node
        if(gSmsDb.size == 1)
        {
                DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);
                gSmsDb.pHead = null;
                gSmsDb.pTail = null;
        }

        //if the node is the head
        else if(pSms == gSmsDb.pHead)
        {
                gSmsDb.pHead = pSms->pNext;
                gSmsDb.pHead->pPrev = null;
        }

        //if the node is the tail
        else if(pSms == gSmsDb.pTail)
        {
                gSmsDb.pTail = pSms->pPrev;
                gSmsDb.pTail->pNext = null;
        }
        else
        {
                pPrevNode = pSms->pPrev;
                pPrevNode->pNext = pSms->pNext;
                pSms->pNext->pPrev = pPrevNode;
        }
        status = tx_block_release((VOID *) pSms);

        //if(status != TX_SUCCESS) return status;

        //Decrease the size of the list
        gSmsDb.size--;
        return status;
}

int modelGetSmsSerialNumber(const SmsLinkNodePtr pSms)
{
    DBG_ASSERT(pSms != null);
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
		for(pNode = gSmsDb.pHead ; pNode->pNext != null ; pNode = pNode->pNext)
		{
				if(pNode == pSms) return serialNum;
				++serialNum;
		}
		
		//if the sms pointer was not found
		return -1;
		
}

SMS_SUBMIT* modelGetInEditSms()
{
        return gInEditSms;
}


SmsLinkNodePtr modelGetFirstSmsOnScreen()
{
        return gpFirstSmsOnScreen;
}

SmsLinkNodePtr modelGetSelectedSms()
{
        return gpSelectedSms;
}
