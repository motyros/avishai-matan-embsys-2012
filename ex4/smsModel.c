#include "smsModel.h"
#include "embsys_sms_protocol_mine.h"
#include "fs.h"


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

//cunter for the file names
uint_8 gFileCunter;

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

FS_STATUS fillLinkedList(unsigned fileCount);

UINT modelInit()
{
	UINT status;
	//create a pool that stores all the sms
	status = tx_mutex_create(&gModelMutex,"Model Mutex",TX_INHERIT);
	if (status != TX_SUCCESS)
	{
		return status;
	}

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

	memset(&gInEditSms,0,sizeof(SMS_SUBMIT));

	unsigned fileCount = 0;
	FS_STATUS fsStatus = fs_count(&file_count);

	//init the list to empty
	gSmsDb.pHead = NULL;
	gSmsDb.pTail = NULL;
	gSmsDb.size = 0;

	//the file system is empty
	if(fileCount==0)
	{
		return TX_SUCCESS;
	}

	//else the file system not empty
	fsStatus = fillLinkedList(fileCount);

	return fsStatus;?

}

FS_STATUS createAndAddSmsLinkNodeToDB(uint_8 fileName, const message_type type, char* data)
{
	FS_STATUS fsStatus;
	UINT txStatus;
	SmsLinkNodePtr pNewSms;
	//allocate mem
	txStatus = allocateMemInPool(pNewSms);
	if(txStatus != TX_SUCCESS) return FAILURE;

	//fill it's fields
	fsStatus = fillSmsNodeFields(fileName, type, pNewSms, data);
	//add to the linked list
	addSmsToLinkedList(pNewSms);

	return fsStatus;
}

FS_STATUS fillLinkedList(unsigned fileCount)
{
	FS_STATUS fsStatus;
	unsigned i;
	unsigned smsSize = SMS_BLOCK_SIZE;
	char data[SMS_BLOCK_SIZE];
	uint_8 fileName[1];
	//read all files
	for ( i = 0 ; i < fileCount ; ++i )
	{
		//read the file by index
		fsStatus = fs_read_by_index( i, &smsSize, (char*)data);
		//get it's name
		fsStatus = fs_get_filename_by_index(i, (char*) fileName)

			if(smsSize == sizeof(SMS_DELIVER))
			{
				fsStatus = createAndAddSmsLinkNodeToDB(fileName, INCOMMING_MESSAGE, (char*)data);
			}
			else if(smsSize == sizeof(SMS_SUBMIT))
			{
				fsStatus = createAndAddSmsLinkNodeToDB(fileName, OUTGOING_MESSAGE, (char*)data);, 
			}
			else
			{
				continue;
			}
	}
	return fsStatus;
}

UINT modelGetSmsByFileName(const uint_8 fileName, unsigned* smsSize, char* data)
{
	//unsigned smsSize = SMS_BLOCK_SIZE;
	//data[SMS_BLOCK_SIZE];
	FS_STATUS status = fs_read(&fileName, &smsSize, data);
	if(status != SUCCESS) return TX_NO_INSTANCE;?
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

/*
* updates the head and tail to point to each other
* to have a cyclic list
*/
void updateHeadAndTailCyclic()
{
	gSmsDb.pHead->pPrev = gSmsDb.pTail;
	gSmsDb.pTail->pNext = gSmsDb.pHead;
}


uint_8 findFileName()
{
	uint_8 fileName;
	int tempDataLength = 1;
	char tempData[1];
	FS_STATUS sta;
	do
	{
		//get the next name
		fileName = (uint_8)(gFileCunter%MAX_NUM_SMS);//[FILE_NAME_LENGTH];
		//check if the name is available
		sta = fs_read(&fileName, &tempDataLength, (char*)tempData);
		gFileCunter++;
	}while(sta != FILE_NOT_FOUND);//sta == SUCCESS

	return fileName;

}

FS_STATUS fillTitle(const message_type type, SmsLinkNodePtr pNewSms, char* data)
{
	DBG_ASSERT(pNewSms != NULL);
	DBG_ASSERT(data != NULL);
	SMS_DELIVER* pInSms;
	SMS_SUBMIT* pOutSms;
	int i;
	switch(type)
	{
	case INCOMMING_MESSAGE:
		//mesType = 'I';
		pInSms = (SMS_DELIVER*)data;
		//set the title
		for(i = 0 ; (pInSms->sender_id[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
		{
			*(pNewSms->title)++ = pInSms->sender_id[i];
		}
		return SUCCESS;

	case OUTGOING_MESSAGE:
		//mesType = 'O';
		pOutSms = (SMS_SUBMIT*)data;
		//set the title
		for(i = 0 ; (pOutSms->recipient_id[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
		{
			*(pNewSms->title)++ = pOutSms->recipient_id[i];
		}
		return SUCCESS;
	}
	return COMMAND_PARAMETERS_ERROR;
}

FS_STATUS writeSmsToFileSystem(const message_type type, SmsLinkNodePtr pNewSms, char* data)
{
	DBG_ASSERT(pNewSms != NULL);
	DBG_ASSERT(data != NULL);
	switch(type)
	{
	case INCOMMING_MESSAGE:
		//add the sms-deliver to the file system
		return fs_write(pNewSms->fileName, sizeof(SMS_DELIVER), data);

	case OUTGOING_MESSAGE:
		//add the sms-submit to the file system
		return fs_write(pNewSms->fileName, sizeof(SMS_SUBMIT), data);

	}
	return COMMAND_PARAMETERS_ERROR;
}

void addSmsToLinkedList(SmsLinkNodePtr pNewSms);
{
	//in case the list is empty
	if(gSmsDb.size == 0)
	{
		DBG_ASSERT(gSmsDb.pHead == NULL);
		DBG_ASSERT(gSmsDb.pTail == NULL);

		//it is a cyclic list
		pNewSms->pNext = pNewSms;
		pNewSms->pPrev = pNewSms;
		//insert the first node in to the list - replace the head and tail
		gSmsDb.pHead = pNewSms;
		gSmsDb.pTail = pNewSms;
	}
	//in case the list has only one node (head==tail!=null)
	else if(gSmsDb.size == 1)
	{
		DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);

		//insert the second node in to the list - replace to the tail
		gSmsDb.pTail = pNewSms;
		gSmsDb.pHead->pNext = gSmsDb.pTail;
		gSmsDb.pTail->pPrev = gSmsDb.pHead;

		//update the head and tail to be a cyclic list
		updateHeadAndTailCyclic();

	}
	//in case the list has more then one node (head!=tail)
	else
	{
		//update the tail of the linked list
		gSmsDb.pTail->pNext = pNewSms;
		pNewSms->pPrev = gSmsDb.pTail;
		gSmsDb.pTail = pNewSms;

		//update the head and tail to be a cyclic list
		updateHeadAndTailCyclic();

	}

	//increase the size of the linked list
	gSmsDb.size++;

}

UINT allocateMemInPool(SmsLinkNodePtr pNewSms)
{
	DBG_ASSERT(pNewSms != NULL);
	//allocate a memory block for the linked list node, form the SmsLinkListPool
	return = tx_block_allocate(&gSmsLinkListPool, (VOID**) &pNewSms,TX_NO_WAIT);
	/* If status equals TX_SUCCESS, pNewNode contains the
	* address of the allocated block of memory.
	*/
}

FS_STATUS fillSmsNodeFields(uint_8 fileName, const message_type type, SmsLinkNodePtr pNewSms,char* pSms)
{
	FS_STATUS fsStatus = fillTitle(type, pNewSms, pSms);

	//set the fields of the linked list node
	pNewSms->pNext = NULL;
	pNewSms->pPrev = gSmsDb.pTail;
	pNewSms->type = type;
	pNewSms->fileName = fileName;

	return fsStatus;
}

UINT modelAddSmsToDb(void* pSms,const message_type type)
{
	DBG_ASSERT(pSms != NULL);

	SmsLinkNodePtr pNewSms;
	UINT txStatus = allocateMemInPool(pNewSms);

	if(txStatus != TX_SUCCESS) return txStatus

	FS_STATUS fsStatus;

	uint_8 fileName = findFileName();

	fsStatus = fillSmsNodeFields(fileName, type, pNewSms,(char*)pSms);

	fsStatus = writeSmsToFileSystem(type,pNewSms,(char*)pSms);

	addSmsToLinkedList(pNewSms);

	//copy the given sms to the allocated memory
	//        memcpy(pNewSms->pSMS,pSms,SMS_BLOCK_SIZE);

	return fsStatus;?
}

TX_STATUS modelRemoveSmsFromDb(const SmsLinkNodePtr pSms)
{
	DBG_ASSERT(pSms != NULL);

	TX_STATUS status;

	//can't remove a sms from empty data-base
	if(gSmsDb.size == 0)
	{
		DBG_ASSERT(gSmsDb.pHead == NULL);
		DBG_ASSERT(gSmsDb.pTail == NULL);

		return TX_PTR_ERROR;
	}

	//erase the sms from the file system
	status = fs_erase(pSms->fileName);

	//if the node is the last node in the list, so the node is 
	//the head and the tail of the list.
	if(gSmsDb.size == 1)
	{
		DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);
		//reset the head and tail
		gSmsDb.pHead = NULL;
		gSmsDb.pTail = NULL;
	}

	//if the node is the head of the list
	else if(pSms == gSmsDb.pHead)
	{
		//remove the head
		gSmsDb.pHead = pSms->pNext;
		gSmsDb.pHead->pPrev = NULL;

		//update the head and tail to be a cyclic list
		updateHeadAndTailCyclic();
	}

	//if the node is the tail of the list
	else if(pSms == gSmsDb.pTail)
	{
		//remove the tail
		gSmsDb.pTail = pSms->pPrev;
		gSmsDb.pTail->pNext = NULL;

		//update the head and tail to be a cyclic list
		updateHeadAndTailCyclic();
	}
	//if the node is not the head and not the tail of the list
	else
	{
		//remove the node
		SmsLinkNodePtr pPrevNode = pSms->pPrev;
		pPrevNode->pNext = pSms->pNext;
		pSms->pNext->pPrev = pPrevNode;
	}

	//release the memory block to the smses linked list pool.
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

	//start the serial number from 1
	int serialNum = 0;
	//if the pointer points to the start of the list
	/* note: this check is needed, because the loop, later in this method, does not 
	* checks the head - in case the list has only one node.*/
	SmsLinkNodePtr pNode = gSmsDb.pHead;

	for(;serialNum < gSmsDb.size ; ++serialNum,pNode = pNode->pNext)
	{
		if(pNode == pSms) return serialNum;
	}

	/*
	if(pSms == gSmsDb.pHead)
	{
	return serialNum;
	}


	//go over the list to indicate the serial number
	//note: it is a cyclic list, so the loop will finish
	for(pNode = gSmsDb.pHead ; pNode->pNext != gSmsDb.pHead ; pNode = pNode->pNext)
	{
	if(pNode == pSms) return serialNum;
	++serialNum;
	}
	*/
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

