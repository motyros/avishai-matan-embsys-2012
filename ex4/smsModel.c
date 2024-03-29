#include "smsModel.h"
#include "fs/fs.h"


#define SMS_BLOCK_SIZE (sizeof(SMS_DELIVER))
#define BLOCK_OVERHEAD (sizeof(void*))
#define NUM_OF_FLASH_BLOCK 16
#define SMS_POOL_REAL_SIZE (MAX_NUM_SMS*(SMS_BLOCK_SIZE+BLOCK_OVERHEAD))

#define SMS_DB_BLOCK_SIZE (sizeof(SmsLinkNode))
#define SMS_DB_POOL_REAL_SIZE (MAX_NUM_SMS*(SMS_DB_BLOCK_SIZE+BLOCK_OVERHEAD))
#define SMS_FILE_NAME_LENGTH 5
#define SMS_FILE_NAME_GARD 0x01010101

//the block of memory for the pool with all the smses
CHAR gSmsBlockPool[SMS_POOL_REAL_SIZE];
//the block of memory for the pool with the sms linked list data-base
CHAR gSmsDbBlockPool[SMS_DB_POOL_REAL_SIZE];

//macro for maximum
#define MAX(x,y) (x) > (y) ? (x) : (y)

// a sms linked list
typedef struct
{
	SmsLinkNodePtr  pHead;
	SmsLinkNodePtr  pTail;
	int             size;
}SmsLinkedList;

//cunter for the file names
uint32_t gFileNameCounter;

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
FS_STATUS fillSmsNodeFields(char* fileName, const message_type type, SmsLinkNodePtr pNewSms,char* pSms);
void addSmsToLinkedListEnd(SmsLinkNodePtr pNewSms);
void addSmsToLinkedList(SmsLinkNodePtr pNewSms);




UINT modelInit()
{
	UINT status;
gFileNameCounter = SMS_FILE_NAME_GARD;
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

	//init the file system
	FS_SETTINGS fsSettings;
	fsSettings.block_count = NUM_OF_FLASH_BLOCK;
	FS_STATUS fsStatus = fs_init(fsSettings);
	if(fsStatus != SUCCESS)
	{
			return fsStatus;
	}

	unsigned fileCount = 0;
	fsStatus = fs_count(&fileCount);
	if(fsStatus != SUCCESS)
	{
			return fsStatus;
	}

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

	return fsStatus;

}

FS_STATUS createAndAddSmsLinkNodeToDB(char* fileName, const message_type type, char* data)
{
	FS_STATUS fsStatus;
	UINT txStatus;
	SmsLinkNodePtr pNewSms;
	//allocate mem
	txStatus = allocateMemInPool(&pNewSms);
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
	unsigned smsSize;
	char data[SMS_BLOCK_SIZE];
	char pFileName[SMS_FILE_NAME_LENGTH];
	unsigned fileNameLength;
	//read all files
	for ( i = 0 ; i < fileCount ; ++i )
	{
		//reset the variables
		smsSize = SMS_BLOCK_SIZE;
		fileNameLength = SMS_FILE_NAME_LENGTH;
		memset(pFileName,0,SMS_FILE_NAME_LENGTH);
		memset(data,0,SMS_BLOCK_SIZE);

		//get the file name
		fsStatus = fs_get_filename_by_index(i, &fileNameLength,(char*) pFileName);
		//in case there is problems with the filename, continue to the next file
		if(fsStatus != SUCCESS) continue;

		//read the file by it's name
		fsStatus = fs_read((char*)pFileName,&smsSize,(char*)data);
		//in case there is problems with the file, continue to the next file
		if(fsStatus != SUCCESS) continue;


		//add the sms acording to the data
		if(smsSize == sizeof(SMS_DELIVER))
		{
			fsStatus = createAndAddSmsLinkNodeToDB((char*)pFileName, INCOMMING_MESSAGE, (char*)data);
		}
		else if(smsSize == sizeof(SMS_SUBMIT))
		{
			fsStatus = createAndAddSmsLinkNodeToDB((char*)pFileName, OUTGOING_MESSAGE, (char*)data);
		}
		else
		{
			//in case there is problems with the file size, continue to the next file
			continue;
		}
	uint32_t fileNumber = *(uint32_t*)pFileName;
	gFileNameCounter = MAX(gFileNameCounter,fileNumber);
	
	}

	return fsStatus;
}

UINT modelGetSmsByFileName(const uint32_t fileName, unsigned* smsSize, char* data)
{
	char pFileName[SMS_FILE_NAME_LENGTH] = {0};
	*((uint32_t*)pFileName) = fileName;
	FS_STATUS status = fs_read((char*)pFileName, smsSize, data);
	if(status != SUCCESS) return TX_NO_INSTANCE;
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

/*
 * find the next available file name
 */
uint32_t findFileName()
{
	char pFileName[SMS_FILE_NAME_LENGTH];
	unsigned tempDataLength;
	char tempData[1];
	FS_STATUS sta;
	uint32_t fileNumber;
	do
	{
		//reset the variables
		tempDataLength = 1;
		memset(pFileName,0,SMS_FILE_NAME_LENGTH);

		//get the next file name
		fileNumber = gFileNameCounter;
		//get the next name
		*(uint32_t*)pFileName = gFileNameCounter;//[FILE_NAME_LENGTH];
		//check if the name is available
		sta = fs_read((char*)pFileName, &tempDataLength, (char*)tempData);
		gFileNameCounter = ++gFileNameCounter | SMS_FILE_NAME_GARD;
	}while(sta != FILE_NOT_FOUND);//sta == SUCCESS

	return fileNumber;

}

/*
 * fill the title of the message
 */
FS_STATUS fillTitle(const message_type type, SmsLinkNodePtr pNewSms, char* data)
{
	DBG_ASSERT(pNewSms != NULL);
	DBG_ASSERT(data != NULL);
	SMS_DELIVER* pInSms;
	SMS_SUBMIT* pOutSms;
	int i;
	memset(pNewSms->title,0,sizeof(ID_MAX_LENGTH));
	switch(type)
	{
	case INCOMMING_MESSAGE:
		//mesType = 'I';
		pInSms = (SMS_DELIVER*)data;
		//set the title
		for(i = 0 ; (pInSms->sender_id[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
		{
			pNewSms->title[i] = pInSms->sender_id[i];
		}

		return SUCCESS;

	case OUTGOING_MESSAGE:
		//mesType = 'O';
		pOutSms = (SMS_SUBMIT*)data;
		//set the title
		for(i = 0 ; (pOutSms->recipient_id[i] != (char)NULL) && (i < ID_MAX_LENGTH) ; ++i)
		{
			pNewSms->title[i] = pOutSms->recipient_id[i];
		}

		return SUCCESS;
	}
	return COMMAND_PARAMETERS_ERROR;
}

/*
 * write the sms to the file system
 */
FS_STATUS writeSmsToFileSystem(const message_type type, SmsLinkNodePtr pNewSms, char* data)
{
	DBG_ASSERT(pNewSms != NULL);
	DBG_ASSERT(data != NULL);
	char pFileName[SMS_FILE_NAME_LENGTH] = {0};
	*(uint32_t*)pFileName = pNewSms->fileName;
	switch(type)
	{
	case INCOMMING_MESSAGE:
		//add the sms-deliver to the file system
		return fs_write((char*)pFileName, sizeof(SMS_DELIVER), data);

	case OUTGOING_MESSAGE:
		//add the sms-submit to the file system
		return fs_write((char*)pFileName, sizeof(SMS_SUBMIT), data);

	}
	return COMMAND_PARAMETERS_ERROR;
}

/*
 * add the first sms to the linked list
 */
void addFirstSmsToLinkedList(SmsLinkNodePtr pNewSms)
{
	DBG_ASSERT( pNewSms != NULL);
	DBG_ASSERT(gSmsDb.pHead == NULL);
	DBG_ASSERT(gSmsDb.pTail == NULL);

	//it is a cyclic list
	pNewSms->pNext = pNewSms;
	pNewSms->pPrev = pNewSms;
	//insert the first node in to the list - replace the head and tail
	gSmsDb.pHead = pNewSms;
	gSmsDb.pTail = pNewSms;

}

/*
 * add the sms to the linked list data base, according to the file name
 */
void addSmsToLinkedList(SmsLinkNodePtr pNewSms)
{
	DBG_ASSERT( pNewSms != NULL);
	//in case the linked list in empty or the sms needs to be insert to the end
	if(gSmsDb.size == 0 || gSmsDb.pTail->fileName < pNewSms->fileName)
	{
		addSmsToLinkedListEnd(pNewSms);
		return;
	}

	//in case the list has only one node (head==tail!=null)
	if(gSmsDb.size == 1)
	{
		DBG_ASSERT(gSmsDb.pHead == gSmsDb.pTail);

		//insert the second node in to the list - replace to the head
		gSmsDb.pHead = pNewSms;
		gSmsDb.pHead->pNext = gSmsDb.pTail;
		gSmsDb.pTail->pPrev = gSmsDb.pHead;

	}
	//in case the linked list has more then 1 sms
	//in case the sms needs to be insert to the beginning of the linked list
	else if(gSmsDb.pHead->fileName > pNewSms->fileName)
	{
		//update the head of the linked list
		pNewSms->pNext = gSmsDb.pHead;
		gSmsDb.pHead->pPrev = pNewSms;
		gSmsDb.pHead = pNewSms;


	}
	//in case the sms needs to be insert, not to the edges of the linked list
	else
	{
		//update the tail of the linked list
		SmsLinkNodePtr iterator;
		for(iterator = gSmsDb.pHead ; iterator->fileName < pNewSms->fileName ; iterator = iterator->pNext);
		SmsLinkNodePtr iteratorPrev = iterator->pPrev;

		//update the iterator node
		iterator->pPrev = pNewSms;
		pNewSms->pNext = iterator;

		//update the iterator previous node
		iteratorPrev->pNext = pNewSms;
		pNewSms->pPrev = iteratorPrev;
	}
	//update the head and tail to be a cyclic list
	updateHeadAndTailCyclic();
	//increase the size of the linked list
	gSmsDb.size++;
}


/*
 * add the sms to the end of the linked list data base
 */
void addSmsToLinkedListEnd(SmsLinkNodePtr pNewSms)
{
	DBG_ASSERT( pNewSms != NULL);
	//in case the list is empty
	if(gSmsDb.size == 0)
	{
		addFirstSmsToLinkedList(pNewSms);
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

/*
 * allocate memory in the pool for the given sms
 */
UINT allocateMemInPool(SmsLinkNodePtr * pNewSms)
{
	//allocate a memory block for the linked list node, form the SmsLinkListPool
	return tx_block_allocate(&gSmsLinkListPool, (VOID**) pNewSms,TX_NO_WAIT);
	/* If status equals TX_SUCCESS, pNewNode contains the
	* address of the allocated block of memory.
	*/
}

/*
 * fill the sms node fields
 */
FS_STATUS fillSmsNodeFields(char* fileName, const message_type type, SmsLinkNodePtr pNewSms,char* data)
{
	DBG_ASSERT(pNewSms != NULL);
	FS_STATUS fsStatus = fillTitle(type, pNewSms, data);

	//set the fields of the linked list node
	pNewSms->pNext = NULL;
	pNewSms->pPrev = gSmsDb.pTail;
	pNewSms->type = type;

//fill the file name
	int i;
	char* pFileName = (char*)&pNewSms->fileName;
	for(i = 0; i < SMS_FILE_NAME_LENGTH-1 ; ++i)
	{
		*pFileName++ = *fileName++;
	}

	return fsStatus;
}




UINT modelAddSmsToDb(void* pSms,const message_type type)
{
	DBG_ASSERT(pSms != NULL);

	SmsLinkNodePtr pNewSms;
	UINT txStatus = allocateMemInPool(&pNewSms);

	if(txStatus != TX_SUCCESS) return txStatus;

	FS_STATUS fsStatus;

	char fileName[SMS_FILE_NAME_LENGTH] = {0};
	*(uint32_t*)fileName = findFileName();

	//fill the fields
	fsStatus = fillSmsNodeFields(fileName, type, pNewSms,(char*)pSms);

	//write the sms to file system
	fsStatus = writeSmsToFileSystem(type,pNewSms,(char*)pSms);

	addSmsToLinkedListEnd(pNewSms);

	return fsStatus;
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
	char pFileName[SMS_FILE_NAME_LENGTH] = {0};
	*(uint32_t*)pFileName = pSms->fileName;
	status = fs_erase((char*)pFileName);

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
