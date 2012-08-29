#include "fs.h"
#include "../flash/flash.h"
#include "../common_defs.h"
#include "../TX/tx_api.h"
//#include <string.h>

#define FILE_NAME_MAX_LEN (8)
#define ERASE_UNITS_NUMBER (FLASH_CAPACITY/FLASH_BLOCK_SIZE)
#define MAX_FILE_SIZE 512
#define MAX_FILES_CAPACITY 1000
#define FILE_NOT_FOUND_EU_INDEX ERASE_UNITS_NUMBER
#define FLASH_UNINIT_4_BYTES 0xFFFFFFFF
#define MAX_UINT_32 0xFFFFFFFF

#define CANARY_VALUE 0xDEADBEEF

//types of BLOCK
typedef enum
{
	DATA		= 0, 
	LOG			= 1, //important because we convert LOG block to DATA block and in NOR flash we can convert 1 to zero but not vice versa
}EraseUnitType;

//calculate the byte offset of field inside a struct
#define MY_OFFSET(type, field) ((unsigned long ) &(((type *)0)->field))
#define READ_LOG_ENTRY(logIndex,entry) (flash_read((logIndex+1)*FLASH_BLOCK_SIZE - sizeof(LogEntry),sizeof(LogEntry),(uint8_t*)&entry))
#define WRITE_LOG_ENTRY(logIndex,entry) (flash_write((logIndex+1)*FLASH_BLOCK_SIZE - sizeof(LogEntry),sizeof(LogEntry),(uint8_t*)&entry))

#pragma pack(1)

typedef union
{
	uint8_t data;
	struct
	{
		uint8_t type		   : 1; //log = 0, sectors = 1
		uint8_t emptyEU		   : 1; //1 = yes, 0 = no
		uint8_t valid		   : 1; // whole eu valid
		uint8_t	reserved	   : 5;
	} bits;
}EraseUnitMetadata;

typedef struct
{
	EraseUnitMetadata metadata;

	uint32_t eraseNumber;

	uint32_t canary;

}EraseUnitHeader;

typedef struct
{
	//offset of the file's data from the beginning of the block
	uint16_t offset;

	//file's data's size
	uint16_t size;

	union
	{
		uint8_t data;
		struct
		{
			//indication whether this secctor desciptor is the last one
			uint8_t lastDesc		:	1;
			uint8_t validDesc	    :	1;
			uint8_t obsoleteDes		:	1;
			//indication whether the size and the offset of the file are valid
			uint8_t validOffset		:	1;
			uint8_t	reserved	    :   4;
		} bits;
	}metadata;

	char fileName[FILE_NAME_MAX_LEN];

}SectorDescriptor;

typedef union
{
	uint32_t data;
	struct
	{
		//index of the block we are going erase
		uint32_t euIndex		: 4;
		//new erase number of the block euIndex
		uint32_t eraseNumber	: 25;
		//is LogEntry valid
		uint32_t valid			: 1;
		//0 if we finished to copy euIndex to the log's EU
		uint32_t copyComplete	: 1;
		//0 if we erased the euIndex
		uint32_t eraseComplete	: 1;
	} bits;
}LogEntry;

#pragma pack()

typedef struct
{
	uint32_t eraseNumber;
    //the actual free bytes
	uint16_t bytesFree;
	//the number of bytes we will earn during eviction
	uint16_t deleteFilesTotalSize;
	//how many files exist in this EU
	uint16_t validDescriptors;
	//how many non empty descriptors exist
    uint16_t totalDescriptors;
	
	//what is the next offset (upper bound) to write data to
	uint16_t nextFreeOffset;
	
    EraseUnitMetadata metadata;

}EraseUnitLogicalHeader;


/**** Forward Declaration Begin*****/


static FS_STATUS intstallFileSystem();
static FS_STATUS ParseEraseUnitContent(uint16_t i);
static bool isUninitializeSegment(uint8_t* pSeg,uint32_t size);
static void CorrectLogEntry(LogEntry* pEntry);
static FS_STATUS CompleteCompaction(uint8_t firstLogIndex,bool secondLogExists,uint8_t secondLogIndex,bool corruptedEUExists,uint8_t corruptedEUIndex);
static FS_STATUS loadFilesystem();
static FS_STATUS createLogEntry(uint8_t euIndexToDelete,uint32_t newEraseNumber);
static FS_STATUS writeFile(const char* filename, unsigned length, const char* data,uint8_t euIndex);
static bool findFreeEraseUnit(uint16_t fileSize,uint8_t* euIndex);
static bool findEraseUnitToCompact(uint8_t* euIndex,unsigned length);
static FS_STATUS CopyDataInsideFlash(uint16_t srcAddr,uint16_t srcSize,uint16_t destAddr);
static FS_STATUS CompactBlock(uint8_t euIndex,LogEntry entry);
static FS_STATUS getNextValidSectorDescriptor(uint8_t euIndex,uint16_t* pNextActualSectorIndex,SectorDescriptor* pSecDesc);
static bool isFilenameMatchs(SectorDescriptor *pSecDesc,const char* filename);
static FS_STATUS getSectorDescriptor(const char* filename,SectorDescriptor *pSecDesc,uint8_t *pEuIndex,uint16_t* pSecActualIndex);
static FS_STATUS eraseFileFromFS(const char* filename);
static void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size);
static void flash_request_done_cb(void);
/**** Forward Declaration End*****/

//hold the data we need about every EraseUnit. will save flash IO operations
static EraseUnitLogicalHeader gEUList[ERASE_UNITS_NUMBER];

static uint8_t gLogIndex;
static uint16_t gFilesCount = 0;

static TX_EVENT_FLAGS_GROUP gFsGlobalEventFlags;
static TX_MUTEX gFsGlobalLock;
static bool gFsIsReady = false;

//wrappers functions for all none blockinf flash functions





/*
 * install FileSystem on the flash. all previouse content will be deleted
 */
static FS_STATUS intstallFileSystem()
{
	
	int i;
	uint16_t euAddress = 0;
    uint32_t acutalFlags;
	
	EraseUnitHeader header;

    memset(&header,0xFF,sizeof(EraseUnitHeader));

	header.canary = CANARY_VALUE;
	header.eraseNumber = 0;
	

	header.metadata.bits.emptyEU = 1;

	//1. erase the whole flash
	if (flash_bulk_erase_start() != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }

	tx_event_flags_get(&gFsGlobalEventFlags,1,TX_AND_CLEAR,&acutalFlags,TX_WAIT_FOREVER);
	
	
	//2. write each EraseUnitHeader + update the data structure of the EU's logical metadata.
    //the LOG will be placed at the first block.
	for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
	{
		header.metadata.bits.valid = 1;
		if (i > 0)
		{
			header.metadata.bits.type = DATA;
            gEUList[i].nextFreeOffset = FLASH_BLOCK_SIZE;
            gEUList[i].bytesFree = FLASH_BLOCK_SIZE-sizeof(EraseUnitHeader);
		}
        else
        {
            header.metadata.bits.type = LOG;
            gEUList[i].nextFreeOffset = FLASH_BLOCK_SIZE - sizeof(LogEntry); //make room for one log entry at the end of the block
            gEUList[i].bytesFree = FLASH_BLOCK_SIZE-sizeof(EraseUnitHeader) - sizeof(LogEntry);
        }
        
		if (flash_write(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
        {
            return FAILURE_ACCESSING_FLASH;
        }
		
		header.metadata.bits.valid = 0;
		
		if (flash_write(euAddress + MY_OFFSET(EraseUnitHeader,metadata.data),1,(uint8_t*)&header.metadata.data) != OPERATION_SUCCESS)
		{
			return FAILURE_ACCESSING_FLASH;
		}

		//init the DAST which saves data about every EraseUnit (block)
		gEUList[i].eraseNumber = 0;
		
		gEUList[i].validDescriptors = 0;
		gEUList[i].totalDescriptors = 0;
		gEUList[i].deleteFilesTotalSize = 0;
		gEUList[i].metadata = header.metadata;
		euAddress+=FLASH_BLOCK_SIZE;
	}
	
	gLogIndex = 0;

    gFilesCount = 0;

	return FS_SUCCESS;
}

/*
 * parse an EraseUnit on the flash and update the DAST accordingly
 *
 */
static FS_STATUS ParseEraseUnitContent(uint16_t i)
{
   uint16_t baseAddress = i*FLASH_BLOCK_SIZE,secDescOffset = sizeof(EraseUnitHeader);

   SectorDescriptor sd;

   do
   {
        if (flash_read(baseAddress+secDescOffset,sizeof(SectorDescriptor),(uint8_t*)&sd) != OPERATION_SUCCESS)
        {
            return FAILURE_ACCESSING_FLASH; 
        }
        
        //check if we wrote the size and offset succesfully
        if (sd.metadata.bits.validOffset == 0)
        {
            gEUList[i].bytesFree-=sd.size;
            gEUList[i].nextFreeOffset-=sd.size;

            //if this file isn't valid anymore (deleted or just not valid) add it's size
            if (sd.metadata.bits.validDesc == 1 || sd.metadata.bits.obsoleteDes == 0)
            {
                gEUList[i].deleteFilesTotalSize+=sd.size;
            }
        }

        if (sd.metadata.bits.validDesc == 0 && sd.metadata.bits.obsoleteDes == 1)
        {
            ++gEUList[i].validDescriptors;
        }
        
        ++gEUList[i].totalDescriptors;
        secDescOffset += sizeof(SectorDescriptor);


    }while(!sd.metadata.bits.lastDesc && secDescOffset+sizeof(SectorDescriptor) <= FLASH_BLOCK_SIZE);

    gEUList[i].bytesFree -= sizeof(SectorDescriptor)*gEUList[i].totalDescriptors;

    gEUList[i].deleteFilesTotalSize+=sizeof(SectorDescriptor)*(gEUList[i].totalDescriptors-gEUList[i].validDescriptors);

    return FS_SUCCESS;
}


/*
 * check if a given data segment is uninitilize (= every byte equals 0xFF)
 */
static bool isUninitializeSegment(uint8_t* pSeg,uint32_t size)
{
   while(size-- != 0)
   {
      if (*pSeg++ != 0xFF)
      {
         return false;
      }
   }

   return true;
}


/*
 * correct invalid LogEntry
 * assumption: given log entry != uninitilized log entry
 */
static void CorrectLogEntry(LogEntry* pEntry)
{
    LogEntry entry;
    uint32_t minEN;
    uint8_t i;

    //correct only invalid entries
    if (pEntry->bits.valid != 0)
    {
        entry.data = FLASH_UNINIT_4_BYTES;
        minEN = MAX_UINT_32;

        //iterate over the LogEntry combinations (erase number + erase unit index)
        //and check which one might fit the partial log entry we got. choose the one with
        //minimal EraseNumber in case more than one EraseUnits fit.
        for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
        {
            if (i == gLogIndex)
            {
                continue;
            }

            //set current EraseUnit info
            entry.bits.eraseNumber = gEUList[i].eraseNumber + 1;
            entry.bits.euIndex = i;

            //check if pEntry zeros bits are subset of entry
            if ((entry.data & pEntry->data) == entry.data && minEN < gEUList[i].eraseNumber)
            {
                minEN = gEUList[i].eraseNumber;
                *pEntry = entry;
            }
        }

    }

        
}



/*
 * check if FS crashed during compaction and if yes, complete the compaction operation
 */
static FS_STATUS CompleteCompaction(uint8_t firstLogIndex,bool secondLogExists,uint8_t secondLogIndex,bool corruptedEUExists,uint8_t corruptedEUIndex)
{
    LogEntry firstEntry,secondEntry;

    if (READ_LOG_ENTRY(firstLogIndex,firstEntry) != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }

    if (secondLogExists)
    {
        
        if (READ_LOG_ENTRY(secondLogIndex,secondEntry) != OPERATION_SUCCESS)
        {
            return FAILURE_ACCESSING_FLASH;
        }

        //if both entries are uninitilize, no log exist in contradicaion to the fact that 
        //if two logs exist we are in the middle of Block compaction and one LogEntry must exists
        if (firstEntry.data == FLASH_UNINIT_4_BYTES && secondEntry.data == FLASH_UNINIT_4_BYTES)
        {
            return FAILURE;
        }

        //if the first entry is not uninitilize and the second is, this means that firstLogIndex is the real log
        //and we are trying to erase secondLogIndex
        if (firstEntry.data != FLASH_UNINIT_4_BYTES)
        {
            gLogIndex = firstLogIndex;
            firstEntry.bits.euIndex = secondLogIndex;
            firstEntry.bits.valid = 0;
        }
        //vice versa
        else
        {
            gLogIndex = secondLogIndex;
            secondEntry.bits.euIndex = firstLogIndex;
            secondEntry.bits.valid = 1;
            
        }

        //firstEntry will hold the valid data
        firstEntry.data &= secondEntry.data;

        
    }

    //no valid entry - FS didn't crash during block compaction, do nothing
    if (firstEntry.data == FLASH_UNINIT_4_BYTES)
    {
        return FS_SUCCESS;
    }

    //in case of invalid LogEntry
    CorrectLogEntry(&firstEntry);

    //verify that if coruppted EraseUnit exist, this is indeed the EraseUnit we tried to erase
    if (corruptedEUExists && firstEntry.bits.euIndex != corruptedEUIndex)
    {
        return FAILURE;
    }

    return CompactBlock(firstEntry.bits.euIndex,firstEntry);
}

/* 
 * load FileSystem structure from FLASH. in case of corrupted FS structure, install it from the begginning
 */
static FS_STATUS loadFilesystem()
{
	uint8_t corruptedCanariesCount = 0,i,firstLogIndex,secondLogIndex = 0,corruptedEUIndex = 0;
	uint8_t logCount = 0;
	uint16_t euAddress = 0;
	
    EraseUnitHeader header;

     memset(gEUList,0x00,sizeof(gEUList));
    gFilesCount = 0;
    //read every EraseUnit header and parse it's SectorDescriptors list
	for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
	{
		uint16_t dataBytesInUse = 0;
		if (flash_read(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
        {
            return FAILURE_ACCESSING_FLASH;
        }
      
        //check for valid header's structure. we allow one courrupted EraseUnit - in case FS crashs during block_erase
		if (header.metadata.bits.valid == 1 || header.canary != CANARY_VALUE)
		{
			++corruptedCanariesCount;
			if (corruptedCanariesCount > 1)
			{
				break;
			}
            corruptedEUIndex = i;
			euAddress+=FLASH_BLOCK_SIZE;
			continue;
		}
      
		gEUList[i].eraseNumber = header.eraseNumber;
		gEUList[i].metadata.data = header.metadata.data;
		gEUList[i].bytesFree = FLASH_BLOCK_SIZE - sizeof(EraseUnitHeader) - sizeof(LogEntry);
        gEUList[i].nextFreeOffset = FLASH_BLOCK_SIZE - sizeof(LogEntry);

		//if we found valid log
		if (header.metadata.bits.type == LOG)
		{
			logCount++;

			if (logCount > 2)
			{
				break;
			}

			if (logCount == 2)
			{
				secondLogIndex = i;
			}
			else
			{
				firstLogIndex = i;
			}
		}
		//regular sector, since the first operation we doing when adding file is to zero enptyEU bit, this condition is valid
        else if (gEUList[i].metadata.bits.emptyEU == 0)
		{

			ParseEraseUnitContent(i);

            gFilesCount+=gEUList[i].validDescriptors;

		}

        
      
		euAddress+=FLASH_BLOCK_SIZE;
	}

   //if no log found, or corrupted fs, create one
   if (logCount == 0 || logCount > 2 || corruptedCanariesCount > 1)
   {
      return intstallFileSystem();
   }

   if (logCount == 1)
   {
       gLogIndex = firstLogIndex;
   }

   //if we have two logs, we are in the middle of compaction and at the end of the compaction we will set the true gLogIndex
   return CompleteCompaction(firstLogIndex,logCount > 1,secondLogIndex,corruptedCanariesCount>0,corruptedEUIndex);
   
}


static void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size)
{
    if (buffer == NULL || size == 0)
    {
        DBG_ASSERT(false);
    }
    
	DBG_ASSERT(false);
}

static void flash_request_done_cb(void)
{
	tx_event_flags_set(&gFsGlobalEventFlags,1,TX_OR);
}




/*
 * write new LogEntry with relevant data to the Log
 */
static FS_STATUS createLogEntry(uint8_t euIndexToDelete,uint32_t newEraseNumber)
{
    LogEntry entry;

    memset(&entry,0xFF,sizeof(LogEntry));

    //set relevant info
    entry.bits.euIndex = euIndexToDelete;
    entry.bits.eraseNumber = newEraseNumber;

    //write entry
    if (WRITE_LOG_ENTRY(gLogIndex,entry) != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }

    //set the valid bit and write it
    entry.bits.valid = 0;

    if (WRITE_LOG_ENTRY(gLogIndex,entry) != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }

    return FS_SUCCESS;
}

/*
 * writes a file to a given EraseUnit
 */
static FS_STATUS writeFile(const char* filename, unsigned length, const char* data,uint8_t euIndex)
{
    SectorDescriptor desc;
    EraseUnitHeader header;

    //calculate the absolute address of the next free secotr descriptor
    uint16_t sectorDescAddr = FLASH_BLOCK_SIZE*euIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*gEUList[euIndex].totalDescriptors;

    //calculate the absolute address of the file's content to be written
    uint16_t dataAddr = FLASH_BLOCK_SIZE*euIndex + gEUList[euIndex].nextFreeOffset - length;

    memset(&desc,0xFF,sizeof(SectorDescriptor));

    //mark prev sector/EUHeader as non last/empty
    if (gEUList[euIndex].totalDescriptors != 0)
    {
	    memset(&desc,0xFF,sizeof(SectorDescriptor));
	    desc.metadata.bits.lastDesc = 0;
	    if (flash_write(sectorDescAddr - sizeof(SectorDescriptor),sizeof(SectorDescriptor),(uint8_t*)&desc) != OPERATION_SUCCESS)
        {
            return FAILURE_ACCESSING_FLASH;
        }
        desc.metadata.bits.lastDesc = 1;
    }
    else
    {
	    memset(&header,0xFF,sizeof(EraseUnitHeader));
	    header.metadata.bits.emptyEU = 0;

	    if (flash_write(FLASH_BLOCK_SIZE*euIndex,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
        {
            return FAILURE_ACCESSING_FLASH;
        }
    }
    
    //fill SectorDescriptor info
    strncpy(desc.fileName,filename,FILE_NAME_MAX_LEN);
    desc.offset = gEUList[euIndex].nextFreeOffset - length;
    desc.size = length;

    //write entire descriptor
    if (flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc) != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }

    //mark descriptor metadata as valid
    desc.metadata.bits.validOffset = 0;
    flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);


    //write file content
    if (flash_write(dataAddr,length,(uint8_t*)data) != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }
    //mark entire file as valid
    desc.metadata.bits.validDesc = 0;

    if (flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc) != OPERATION_SUCCESS)
    {
        return FAILURE_ACCESSING_FLASH;
    }
    
    //update FS DAST
    gEUList[euIndex].bytesFree -= (sizeof(SectorDescriptor) + length);
    gEUList[euIndex].nextFreeOffset-=length;
    gEUList[euIndex].totalDescriptors++;
    gEUList[euIndex].validDescriptors++;
    ++gFilesCount;
    return FS_SUCCESS;
}

/*
 * search if exist EraseUnit with enough free space to host new file descriptor and file content as well
 * work according the first fit algorithm
 */
static bool findFreeEraseUnit(uint16_t fileSize,uint8_t* euIndex)
{
   uint8_t i;

   //check if none EraseUnit has enough sapce
   for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
   {
	  if (gLogIndex != i && gEUList[i].bytesFree >= (fileSize + sizeof(SectorDescriptor)))
      {
         *euIndex = i;
         return true;
      }
   }
   return false;
}


/*
 * given some size, find if exist EraseUnit that after compaction wil have enough free
 * space to store size bytes (size = file's content + sizeof(SectorDescriptor))
 */
static bool findEraseUnitToCompact(uint8_t* euIndex,unsigned size)
{
    uint32_t minEN = MAX_UINT_32;
    uint16_t maxFreeBytes = 0;
    uint8_t i;
    bool found = false;

    for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
    {
        if (i == gLogIndex)
        {
            continue;
        }

        //if few EU share the minimal EraseNumber and , choose the one which will yield 
        //the largest freeBytes aftet compaction

                //this block will have enough space to host the new file after compaction
        if (    ((uint16_t)(gEUList[i].deleteFilesTotalSize+gEUList[i].bytesFree) >= size) &&   
                //his EN is minimal
                (gEUList[i].eraseNumber <= minEN || 
                //his EN equals to the minimal and the erase will yeild the largest free space
                (gEUList[i].eraseNumber == minEN && maxFreeBytes <= (gEUList[i].deleteFilesTotalSize+gEUList[i].bytesFree))) 
           )
        {
            minEN = gEUList[i].eraseNumber;
            maxFreeBytes = gEUList[i].deleteFilesTotalSize+gEUList[i].bytesFree;
            *euIndex = i;
            found = true;
        }
    }

    return found;
}

/*
 * copy some buffer inside the flash to another palce inside the flash
 *
 */
 static FS_STATUS CopyDataInsideFlash(uint16_t srcAddr,uint16_t srcSize,uint16_t destAddr)
 {
     uint16_t bytesToRead;
     uint8_t data[FLASH_BYTES_PER_TRANSACTION];
     FS_STATUS status = FS_SUCCESS;

     //sinec the flash can read/write in FLASH_BYTES_PER_TRANSACTION chuncks, do it this size
     while (srcSize > 0)
     {
         bytesToRead = (srcSize > FLASH_BYTES_PER_TRANSACTION)?FLASH_BYTES_PER_TRANSACTION:srcSize;

         if (flash_read(srcAddr,bytesToRead,data) != FS_SUCCESS)
         {
             status = FAILURE_ACCESSING_FLASH;
             break;
         }

         if (flash_write(destAddr,bytesToRead,data) != FS_SUCCESS)
         {
             status = FAILURE_ACCESSING_FLASH;
             break;
         }

         srcAddr+=bytesToRead;
         destAddr+=bytesToRead;
         srcSize-=bytesToRead;
     }

     return status;
 }

/*
 * compact given EraseUnit. if previous compaction failed, supply the LogEntry as
 * argument and the function will continue the process from the failure point
 */
static FS_STATUS CompactBlock(uint8_t euIndex,LogEntry entry)
{

	EraseUnitHeader header;
	uint32_t acutalFlags;
    uint16_t actualNextFileDescIdx = 0,secDescAddr,dataOffset;
    uint8_t i;
	FS_STATUS status;
	
    SectorDescriptor sec;

    //if the LogEntry we got is uninitilize one, init it with data
    if (entry.bits.valid == 1)
    {
        entry.bits.euIndex = euIndex;
        entry.bits.eraseNumber = gEUList[euIndex].eraseNumber+1;
        if (WRITE_LOG_ENTRY(gLogIndex,entry) != OPERATION_SUCCESS)
		{
			return FAILURE_ACCESSING_FLASH;
		}
		//flash_write((gLogIndex+1)*FLASH_BLOCK_SIZE - sizeof(LogEntry),sizeof(LogEntry),(uint8_t*)&entry);
    }

    //mark the entry as valid if needed
    if (entry.bits.valid == 1)
    {
        entry.bits.valid = 0;

        //make the entry valid
        if (WRITE_LOG_ENTRY(gLogIndex,entry) != OPERATION_SUCCESS)
		{
			return FAILURE_ACCESSING_FLASH;
		}
    }
    
    
    
    //if we didn't copy files yet
    if (entry.bits.copyComplete == 1)
    {
        memset(&header,0xFF,sizeof(EraseUnitHeader));

        //if we going to copy some valid files, mark the header as non empty
        if (gEUList[euIndex].validDescriptors > 0)
        {
            
            header.metadata.bits.emptyEU = 0;
            gEUList[gLogIndex].metadata.bits.emptyEU = 0;
        }

    	if (flash_write(gLogIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
    	{
			return FAILURE_ACCESSING_FLASH;
		}

        //set initial addres and offset
        secDescAddr = gLogIndex*FLASH_BLOCK_SIZE + sizeof(EraseUnitHeader);
        dataOffset = FLASH_BLOCK_SIZE - sizeof(LogEntry);

        //start to copy files
        for(i = 0 ; i < gEUList[euIndex].validDescriptors ; ++i)
        {
            getNextValidSectorDescriptor(euIndex,&actualNextFileDescIdx,&sec);
            
            dataOffset -= sec.size;

            //copy the file's conent to the new block
            if ((status = CopyDataInsideFlash(FLASH_BLOCK_SIZE*euIndex + sec.offset,sec.size,FLASH_BLOCK_SIZE*gLogIndex + dataOffset)) != FS_SUCCESS)
			{
				return status;;
			}

            sec.offset = dataOffset;

            //set the file's descriptor (SecotrDescriptor)
            if (flash_write(secDescAddr,sizeof(SectorDescriptor),(uint8_t*)&sec) != OPERATION_SUCCESS)
			{
				return FAILURE_ACCESSING_FLASH;
			}

            secDescAddr+=sizeof(SectorDescriptor);

        }

        //set copy complete
        entry.bits.copyComplete = 0;
        if (flash_write(gLogIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(uint8_t*)&header) != FS_SUCCESS)
		{
			return FAILURE_ACCESSING_FLASH;
		}
    }
    
    //erase the block if need to
    if (entry.bits.eraseComplete == 1)
    {
        if (flash_block_erase_start(FLASH_BLOCK_SIZE*euIndex) != OPERATION_SUCCESS)
		{
			return FAILURE_ACCESSING_FLASH;
		}
		
		tx_event_flags_get(&gFsGlobalEventFlags,1,TX_AND_CLEAR,&acutalFlags,TX_WAIT_FOREVER);
		
        //set erase complete
        entry.bits.eraseComplete = 0;
        
		if (flash_write(gLogIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
		{
			return FAILURE_ACCESSING_FLASH;
		}
    }

    
    //set log header to this erased block
    memset(&header,0xFF,sizeof(EraseUnitHeader));

    header.canary = CANARY_VALUE;
    header.eraseNumber = entry.bits.eraseNumber;
    header.metadata.bits.type = LOG;
    header.metadata.bits.valid = 0;

    if (flash_write(euIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
	{
		return FAILURE_ACCESSING_FLASH;
	}

    
    //change old log's type to data type
    memset(&header,0xFF,sizeof(EraseUnitHeader));
    header.metadata.bits.type = DATA;
    
	if (flash_write(gLogIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(uint8_t*)&header) != OPERATION_SUCCESS)
	{
		return FAILURE_ACCESSING_FLASH;
	}


    //set some general data
    gEUList[euIndex].eraseNumber++;

    gEUList[gLogIndex].bytesFree = gEUList[euIndex].bytesFree + gEUList[euIndex].deleteFilesTotalSize;
    gEUList[gLogIndex].deleteFilesTotalSize = 0;
    gEUList[gLogIndex].nextFreeOffset = dataOffset;
    gEUList[gLogIndex].totalDescriptors = gEUList[gLogIndex].validDescriptors = gEUList[euIndex].validDescriptors;

    gEUList[euIndex].metadata.bits.type = LOG;

    //set new log index
    gLogIndex = euIndex;

	return FS_SUCCESS;

}



/*
 * get the next valid sector descriptor inside a given EraseUnit
 * pPrevActualSectorIndex - when calling the function this holds the last actual SectorDescriptor index we processed or 0 if we want the
 *							first SectorDescriptor. when returning this holds the actual SectorDescriptor index of the next vaild sector 
 *							descriptor
 * assumptions: the given erase unit has more SectorDescriptors (i.e. pPrevActualSectorIndex < the last actual sector descriptor index)
 */
static FS_STATUS getNextValidSectorDescriptor(uint8_t euIndex,uint16_t* pNextActualSectorIndex,SectorDescriptor* pSecDesc)
{

   uint16_t baseAddress = FLASH_BLOCK_SIZE*euIndex,secDescOffset = sizeof(EraseUnitHeader) ,sectorIndexInc = 0;
   
   //advance the index if we got relevant value value
   if (pNextActualSectorIndex != NULL && *pNextActualSectorIndex > 0)
   {
      secDescOffset += sizeof(SectorDescriptor)*(*pNextActualSectorIndex);
       //++sectorIndexInc;
   }

   do
   {
      flash_read(baseAddress + secDescOffset,sizeof(SectorDescriptor),(uint8_t*)pSecDesc);
      secDescOffset+=sizeof(SectorDescriptor);
      ++sectorIndexInc;
   }while(pSecDesc->metadata.bits.validDesc != 0 || pSecDesc->metadata.bits.obsoleteDes == 0 && secDescOffset + sizeof(SectorDescriptor) < FLASH_BLOCK_SIZE);

   //cancle last increment, since we want the actual index of the current file and not the next
   --sectorIndexInc;


   //updtae the actual index if such been provided
   if (pNextActualSectorIndex != NULL)
   {
      *pNextActualSectorIndex+=(sectorIndexInc + 1);
   }

   return FS_SUCCESS;
}

/*
 * check if a given file name matchs a given SectorDescriptor's file name 
 *
 */
static bool isFilenameMatchs(SectorDescriptor *pSecDesc,const char* filename)
{
   char fsFileName[FILE_NAME_MAX_LEN+1] = {'\0'};
   memcpy(fsFileName,pSecDesc->fileName,FILE_NAME_MAX_LEN);
   return strcmp(fsFileName,filename)==0;
}

/*
 * search for a sector descriptor that matchs a given file name.
 * return the SectorDescriptor itself, it's actual index and his EraseUnit's index
 */
static FS_STATUS getSectorDescriptor(const char* filename,SectorDescriptor *pSecDesc,uint8_t *pEuIndex,uint16_t* pSecActualIndex)
{
   uint16_t secIdx,nextActualSecIdx;
   bool found = false;
   uint8_t euIdx;

   for(euIdx = 0 ; euIdx < ERASE_UNITS_NUMBER ; ++euIdx)
   {
      //skip the log EU since we can't save files there
      if (gLogIndex == euIdx)
      {
         continue;
      }

      nextActualSecIdx = 0;

	  //parse vaild secor descriptor and check if the given filename matchs
      for(secIdx = 0 ; secIdx < gEUList[euIdx].validDescriptors ; ++secIdx)
      {
         if (getNextValidSectorDescriptor(euIdx,&nextActualSecIdx,pSecDesc) != FS_SUCCESS)
		 {
			 break;
		 }

         if (isFilenameMatchs(pSecDesc,filename))
         {
            found = true;
            break;
         }
      }

	  //if we found, no need to parse the rest of the EUs.
      if (found)
      {
         break;
      }
   }
   
   //update relevant info if we found mathc
   if (found)
   {
      if (pEuIndex != NULL)
      {
         *pEuIndex = euIdx;
      }
      if (pSecActualIndex != NULL)
      {
         *pSecActualIndex = nextActualSecIdx-1;
      }
   }

   return found?FS_SUCCESS:FILE_NOT_FOUND;
}



/*
 * erase given filename from FS
 */
static FS_STATUS eraseFileFromFS(const char* filename)
{
   SectorDescriptor sec;
   FS_STATUS status;
   uint16_t actualSecIndex;
   uint8_t secEuIndex;
   
   status = getSectorDescriptor(filename,&sec,&secEuIndex,&actualSecIndex);

   if (status != FS_SUCCESS)
   {
      return status;
   }

   //mark the file as obsolete and write it to the flash
   sec.metadata.bits.obsoleteDes = 0;

   if (flash_write(FLASH_BLOCK_SIZE*secEuIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*actualSecIndex,sizeof(SectorDescriptor),(uint8_t*)&sec) != OPERATION_SUCCESS)
   {
	   return FAILURE_ACCESSING_FLASH;
   }

   //update internal DAST
   --gEUList[secEuIndex].validDescriptors;
   gEUList[secEuIndex].deleteFilesTotalSize+=(sec.size + sizeof(SectorDescriptor));
   --gFilesCount;

   return FS_SUCCESS;

}

FS_STATUS fs_erase(const char* filename)
{
    FS_STATUS status;
	
	if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}
	
	//take lock
	if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
	{
		return FS_IS_BUSY;
	}
    
    status = eraseFileFromFS(filename);
    
	tx_mutex_put(&gFsGlobalLock);
	
    return status;
}

FS_STATUS fs_count(unsigned* file_count)
{
   //take the lock
   if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}
	
	//take lock
	if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
	{
		return FS_IS_BUSY;
	}
	
   *file_count = gFilesCount;

   tx_mutex_put(&gFsGlobalLock);
   
   return FS_SUCCESS;
}

FS_STATUS fs_list(unsigned* length, char* files)
{
   SectorDescriptor secDesc;
   uint16_t secIdx,lastActualSecIdx,ansIndex = 0;
   bool found = false;
   uint8_t euIdx;
   uint8_t filenameLen;
   char fsFilename[FILE_NAME_MAX_LEN+1] = {'\0'};
   FS_STATUS status = FS_SUCCESS;

   if (files == NULL || length == NULL)
   {
	   return COMMAND_PARAMETERS_ERROR;
   }
   
   if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}
	
	//take lock
	if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
	{
		return FS_IS_BUSY;
	}

   for(euIdx = 0 ; euIdx < ERASE_UNITS_NUMBER ; ++euIdx)
   {
      //skip the log EU
	   if (gLogIndex == euIdx || gEUList[euIdx].validDescriptors == 0)
      {
         continue;
      }

      lastActualSecIdx = 0;

	  //read current EraseUnit valid sectors
      for(secIdx = 0 ; secIdx < gEUList[euIdx].validDescriptors ; ++secIdx)
      {
         getNextValidSectorDescriptor(euIdx,&lastActualSecIdx,&secDesc);
         strncpy(fsFilename,secDesc.fileName,FILE_NAME_MAX_LEN);
         filenameLen = strlen(fsFilename);

		 //check if the given buffer long enough to store current file name + '\0'
         if ((uint16_t)(ansIndex + filenameLen + 1) > *length)
         {
             status = FAILURE;
			 break;
         }
         strcpy(&files[ansIndex],fsFilename);
         ansIndex+=(filenameLen+1);//+1 for the terminator '\0'
      }
   }

   *length  = ansIndex;

   //return lock
   tx_mutex_put(&gFsGlobalLock);
   
   return status;
}

FS_STATUS fs_get_filename_by_index(unsigned index,unsigned* length, char* name)
{
	SectorDescriptor desc;
	uint16_t actualSecDesc;
	uint8_t euIdx = 0,i,fileNameLen = 0;
	FS_STATUS status = FS_SUCCESS;


	if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}



	//take lock
	if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
	{
		return FS_IS_BUSY;
	}

	do
	{
		if (index >= gFilesCount)
		{
			status = FILE_NOT_FOUND;
			break;
		}

		while (index >= gEUList[euIdx].validDescriptors)
		{
			index-=gEUList[euIdx].validDescriptors;
			++euIdx;
		}

		actualSecDesc = 0;

		for(i = 0 ; i <= index ; ++i)
		{
			if ((status = getNextValidSectorDescriptor(euIdx,&actualSecDesc,&desc)) != FS_SUCCESS)
			{
				break;
			}
		}

		while(fileNameLen < FILE_NAME_MAX_LEN && desc.fileName[fileNameLen] != '\0')
		{
			++fileNameLen;
		}

		if (fileNameLen > *length)
		{
			status = FAILURE;
			break;
		}

		memcpy(name,desc.fileName,fileNameLen);

		*length = fileNameLen;

	}while(false);

	tx_mutex_put(&gFsGlobalLock);

	return status;
}

FS_STATUS fs_init(const FS_SETTINGS settings)
{
   
	TX_STATUS status;
	FS_STATUS fStatus;
    if (settings.block_count == 0)
    {
    }
   
   if ((status=tx_event_flags_create(&gFsGlobalEventFlags,"fs global event flags")) != TX_SUCCESS)
   {
		return FAILURE;
   }
   
   if ((status=tx_mutex_create(&gFsGlobalLock,"fs global lock",TX_NO_INHERIT)) != TX_SUCCESS)
   {
		return FAILURE;
   }
   
   if (flash_init(flash_data_recieve_cb,flash_request_done_cb) != OPERATION_SUCCESS)
   {
		return FAILURE;
   }
   
   fStatus = loadFilesystem();

   if (fStatus == FS_SUCCESS)
   {
	   gFsIsReady = true;
   }

   return fStatus;
}



FS_STATUS fs_write(const char* filename, unsigned length, const char* data)
{
   LogEntry entry;
   FS_STATUS status = MAXIMUM_FLASH_SIZE_EXCEEDED;
   uint8_t euIndex,oldLogIndex;
   
   if (filename == NULL || data == NULL)
   {
      return COMMAND_PARAMETERS_ERROR;
   }

   if (length > MAX_FILE_SIZE)
   {
      return MAXIMUM_FILE_SIZE_EXCEEDED;
   }

   if (gFilesCount > MAX_FILES_CAPACITY)
   {
      return MAXIMUM_FILES_CAPACITY_EXCEEDED;
   }

   if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}
	
   if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
   {
		return FS_IS_BUSY;
   }
   
   do
   {
   
	   //try to find free space
	   if (findFreeEraseUnit(length,&euIndex))
	   {
			status = eraseFileFromFS(filename);
			if (status != FS_SUCCESS && status != FILE_NOT_FOUND)
			{
				status = FAILURE_ACCESSING_FLASH;
				break;
			}
			status =  writeFile(filename,length,data,euIndex);
			
	   }
	   //try to find EraseUnit that after compaction will have enough space 
	   else if (findEraseUnitToCompact(&euIndex,length+sizeof(SectorDescriptor)))
	   {
		   entry.data = FLASH_UNINIT_4_BYTES;
		   oldLogIndex = gLogIndex;
		   if (CompactBlock(euIndex,entry) == FS_SUCCESS)
		   {
				//erase previouse file with same name if such exist
				status = eraseFileFromFS(filename);
				
				if (status != FS_SUCCESS && status != FILE_NOT_FOUND)
				{
					status =  FAILURE_ACCESSING_FLASH;
					break;
				}

				//write the actual file
				status =  writeFile(filename,length,data,oldLogIndex);
		   }
	   }
   }while(false);
   
   tx_mutex_put(&gFsGlobalLock);
   
   return status;
}


FS_STATUS fs_filesize(const char* filename, unsigned* length)
{
   SectorDescriptor sec;
   FS_STATUS status;
   uint8_t secEuIdx;
   
   if (filename == NULL || length == NULL)
   {
	   return COMMAND_PARAMETERS_ERROR;
   }
   
   if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}
	
   if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
   {
		return FS_IS_BUSY;
   }
   //search for this filename
   status = getSectorDescriptor(filename,&sec,&secEuIdx,NULL);

   *length = sec.size;
   
   tx_mutex_put(&gFsGlobalLock);
   
   return status;
   
}

FS_STATUS fs_read(const char* filename, unsigned* length, char* data)
{
   SectorDescriptor sec;
   FS_STATUS status = FS_SUCCESS;
   uint8_t secEuIndex;
   
   if (filename == NULL || length == NULL || data == NULL)
   {
	   return COMMAND_PARAMETERS_ERROR;
   }

   if (!gFsIsReady)
	{
		return FS_NOT_READY;
	}
	
   if (tx_mutex_get(&gFsGlobalLock,TX_NO_WAIT) != TX_SUCCESS)
   {
		return FS_IS_BUSY;
   }
   
   do
   {
	   //get the file
	   status = getSectorDescriptor(filename,&sec,&secEuIndex,NULL);

	   if (status != FS_SUCCESS)
	   {
		  break;
	   }

	   //given buffer is to small
	   if (sec.size > *length)
	   {
		   status =  FAILURE;
		   break;
	   }

	   
	   *length = sec.size;
	   
	   //read the file's content
	   if (flash_read(FLASH_BLOCK_SIZE*secEuIndex + sec.offset,sec.size,(uint8_t*)data) != OPERATION_SUCCESS)
	   {
		   status = FAILURE_ACCESSING_FLASH;
		   break;
	   }
	}while(false);
   
   tx_mutex_put(&gFsGlobalLock);
   
   return status;
   
}
