#include "fs.h"
#include "flash.h"
#include <string.h>

#define FILE_NAME_MAX_LEN (8)
#define ERASE_UNITS_NUMBER (FLASH_SIZE/BLOCK_SIZE)
#define wait_for_flash_done
#define MAX_FILE_SIZE 512
#define MAX_FILES_CAPACITY 1000
#define FILE_NOT_FOUND_EU_INDEX ERASE_UNITS_NUMBER
//TODO add global mutex, try to take it with TX_NO_WAIT value (failure = not ready)

#define CANARY_VALUE 0xDEADBEEF

typedef enum
{
	DATA		= 0,
	LOG			= 1,
}EraseUnitType;

typedef enum
{
   ADD_FILE  = 0,
   BLOCK_MOVE = 1,
}LogEntryType;

#define MY_OFFSET(type, field) ((unsigned long ) &(((type *)0)->field))

#pragma pack(1)

/*
typedef union
{
	uint8_t data[5];
	struct
	{
		uint32_t logEraseNumber;
      struct
      {
         uint8_t logEuIndex   : 5; //what is the index of the new log
         uint8_t valid        : 1;
         uint8_t obsolete     : 1;
         uint8_t logBeenErase : 1; //the new log has been erased already
      }metadata;
	}bytes;
}HeaderTemp;
*/
typedef union
{
	uint8_t data;
	struct
	{
		uint8_t type		   : 1; //log = 0, sectors = 1
		uint8_t emptyEU		   : 1; //1 = yes, 0 = no
		uint8_t valid		   : 1; // whole eu valid
		//uint8_t obsolete     : 1; // whole eu oblolete
		uint8_t	reserved	   : 5;
	} bits;
}EraseUnitMetadata;

typedef struct
{
	EraseUnitMetadata metadata;

	uint32_t eraseNumber;

	uint32_t canary;
	//this section should store the index of the new log and the new EraseNumber of EU[index]. this data important
	//in case of crash during log update, this info will enable to continue the operation.

	//when searching for "free" temp, zero all none 0xFF... temp values. if current temp & our temp value == our value, use it
	//this will solve the case when system crashes after writing temp, and eventually we will run out free temps.

	//from log erase to log erase we can be sure that free temp will be available because to make log full, the number
	//of required operation will cause more than one block to be zeroed (will create available temp)
	//HeaderTemp temp;
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

/*
typedef struct
{
   uint16_t valid       : 1;
   uint16_t obsolete    : 1;
   uint16_t type        : 1;
   uint16_t euIndex     : 4;
   uint16_t sectorIndex : 9;
}AddFileEntry;

typedef struct
{
   uint32_t eraseNumber;
      
   struct
   {
      uint16_t valid             : 1;
      uint16_t obsolete          : 1;
      uint16_t type              : 1;
      uint16_t blockBeenDeleted  : 1;
      uint16_t sectorToStart     : 9;
      uint16_t reserved          : 3;
   }bits1;

   struct
   {
      uint8_t euFromIndex       : 4;
      uint8_t euToIndex         : 4;
   }bits2;
}MoveBlockEntry;

typedef union
{

   uint8_t data[7];
   
   AddFileEntry addFile;

   MoveBlockEntry moveBlock;
}LogEntry;
*/

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



//hold the data we need about every EraseUnit. will save flash IO operations
EraseUnitLogicalHeader gEUList[ERASE_UNITS_NUMBER];


uint8_t gLogIndex;
uint16_t gFilesCount = 0;

//if no log found during initilization, erase the whole flash and install the filesystem


/*
 * install FileSystem on the flash. all previouse content will be deleted
 */
FS_STATUS intstallFileSystem()
{
	
	int i;
	uint16_t euAddress = 0;
    
	EraseUnitHeader header;

    memset(&header,0xFF,sizeof(EraseUnitHeader));

	header.canary = CANARY_VALUE;
	header.eraseNumber = 0;
	

	header.metadata.bits.emptyEU = 1;

	//1. erase the whole flash
	flash_bulk_erase_start();
	wait_for_flash_done;
	
	
	//2. write each EraseUnitHeader + update the data structure of the EU's logical metadata
	for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
	{
		header.metadata.bits.valid = 1;
		if (i > 0)
		{
			header.metadata.bits.type = DATA;
            gEUList[i].nextFreeOffset = BLOCK_SIZE;
            gEUList[i].bytesFree = BLOCK_SIZE-sizeof(EraseUnitHeader);
		}
        else
        {
            header.metadata.bits.type = LOG;
            gEUList[i].nextFreeOffset = BLOCK_SIZE - sizeof(LogEntry); //make room for one log entry at the end of the block
            gEUList[i].bytesFree = BLOCK_SIZE-sizeof(EraseUnitHeader) - sizeof(LogEntry);
        }
        
		flash_write(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);
		
		header.metadata.bits.valid = 0;
		flash_write(euAddress + MY_OFFSET(EraseUnitHeader,metadata.data),1,(uint8_t*)&header.metadata.data);

		
		gEUList[i].eraseNumber = 0;
		
		gEUList[i].validDescriptors = 0;
		gEUList[i].totalDescriptors = 0;
		gEUList[i].deleteFilesTotalSize = 0;
		gEUList[i].metadata = header.metadata;
		euAddress+=BLOCK_SIZE;
	}
	
	gLogIndex = 0;

    gFilesCount = 0;
}

void ParseEraseUnitContent(uint16_t i)
{
   uint16_t baseAddress = i*BLOCK_SIZE,secDescOffset = sizeof(EraseUnitHeader);

   SectorDescriptor sd;

   do
   {
        flash_read(baseAddress+secDescOffset,sizeof(SectorDescriptor),(uint8_t*)&sd);
        
        //check if we wrote the size and offset succesfully
        if (sd.metadata.bits.validOffset == 0)
        {
            gEUList[i].bytesFree-=sd.size;
            gEUList[i].nextFreeOffset-=sd.size;

            //if this file isn't valid anymore (deleted or just not valid) add it's size
            if (sd.metadata.bits.validDesc == 1 || sd.metadata.bits.obsoleteDes == 0)
            {
                gEUList[i].deleteFilesTotalSize+=(sd.size+sizeof(SectorDescriptor));
            }
        }

        if (sd.metadata.bits.validDesc == 0 && sd.metadata.bits.obsoleteDes == 1)
        {
            ++gEUList[i].validDescriptors;
        }
        
        ++gEUList[i].totalDescriptors;
        secDescOffset += sizeof(SectorDescriptor);


    }while(!sd.metadata.bits.lastDesc && secDescOffset+sizeof(SectorDescriptor) <= BLOCK_SIZE);

    gEUList[i].bytesFree -= sizeof(SectorDescriptor)*gEUList[i].totalDescriptors;
}

/*void handleLogSelfMigration(uint8_t headerHostIndex,HeaderTemp h)
{
   EraseUnitHeader header = {0xFF};

   if (h.bytes.metadata.logBeenErase == 1)
   {
      flash_block_erase_start(BLOCK_SIZE * h.bytes.metadata.logEuIndex);

      h.bytes.metadata.logBeenErase = 0;

      //set the logBeenErased bit
      flash_write(BLOCK_SIZE * headerHostIndex + sizeof(uint32_t),1,(uint8_t*)&h);
   }
   


   header.eraseNumber = h.bytes.logEraseNumber;
   header.metadata.bits.type = LOG;
   //set this entry as obsolete
   flash_write(BLOCK_SIZE * headerHostIndex,sizeof(EraseUnitHeader),(uint8_t*)&header);

   header.metadata.bits.valid = 0;

   flash_write(BLOCK_SIZE * headerHostIndex,1,(uint8_t*)&header.metadata);

   gLog.logEntryOffset = sizeof(EraseUnitHeader);

}
*/

bool isUninitializeSegment(uint8_t* pSeg,uint32_t size)
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
void parseLog()
{
   LogEntry e;
   LogEntry lastEntry;
   uint16_t entryBaseLine = gLog.logEUListIndex * BLOCK_SIZE;

   gLog.logEntryOffset = sizeof(EraseUnitHeader);

   while(gLog.logEntryOffset + sizeof(LogEntry) < BLOCK_SIZE)
   {
      flash_read(entryBaseLine + gLog.logEntryOffset,sizeof(LogEntry),(uint8_t*)&e);
      if (isUninitializeSegment((uint8_t*)&e,sizeof(LogEntry)))
      {
         return;
      }
      lastEntry = e;

      if (e.addFile.type == ADD_FILE)
      {
         gLog.logEntryOffset += sizeof(AddFileEntry);
      }
      else
      {
         gLog.logEntryOffset += sizeof(MoveBlockEntry);
      }
   }

   //TODO if last entry is valid and not obsolete, act accordingly

}
*/
//this function loads filesystem structure from FLASH
//need to verify that:
// 1. only one valid log exists
// 2. log is empty or last log entry is obsolete (we didn't crash in the middle of an operation)
void loadFilesystem()
{
	uint8_t corruptedCanariesCount = 0,i,firstLogIndex,secondLogIndex;
	uint8_t logCount = 0;
	uint16_t euAddress = 0;
	
    EraseUnitHeader header;
    

    flash_init(NULL,NULL);

    memset(gEUList,0x00,sizeof(gEUList));

	for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
	{
		uint16_t dataBytesInUse = 0;
		flash_read(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);
      
		if (header.metadata.bits.valid == 1 || header.canary != CANARY_VALUE)
		{
			++corruptedCanariesCount;
			if (corruptedCanariesCount > 1)
			{
				break;
			}
			euAddress+=BLOCK_SIZE;
			continue;
		}
      
		gEUList[i].eraseNumber = header.eraseNumber;
		gEUList[i].metadata.data = header.metadata.data;
		gEUList[i].bytesFree = BLOCK_SIZE - sizeof(EraseUnitHeader);
        gEUList[i].nextFreeOffset = BLOCK_SIZE;

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

            gEUList[i].bytesFree -= sizeof(LogEntry);
            gEUList[i].nextFreeOffset -= sizeof(LogEntry);
		}
		//regular sector
        else if (gEUList[i].metadata.bits.emptyEU == 0)
		{

			ParseEraseUnitContent(i);

            gFilesCount+=gEUList[i].validDescriptors;

		}

        
      
		euAddress+=BLOCK_SIZE;
	}

   //if no log found, or corrupted fs, create one
   if (logCount == 0 || logCount > 2 || corruptedCanariesCount > 1)
   {
      intstallFileSystem();
   }

   if (logCount == 1)
   {
       gLogIndex = firstLogIndex;
   }
   else
   {
       //resolve duplicity
   }

   //cehck if the "true" log has an entry and complete it if needed
}


FS_STATUS fs_init(const FS_SETTINGS settings)
{
   loadFilesystem();
}


void createLogEntry(uint8_t euIndexToDelete,uint32_t newEraseNumber)
{
    LogEntry entry;
    uint16_t entryAbsAddr = (gLogIndex+1)*(BLOCK_SIZE) - sizeof(LogEntry);

    memset(&entry,0xFF,sizeof(LogEntry));

    entry.bits.euIndex = euIndexToDelete;
    entry.bits.eraseNumber = newEraseNumber;

    flash_write(entryAbsAddr,sizeof(LogEntry),(uint8_t*)&entry);

    entry.bits.valid = 0;

    flash_write(entryAbsAddr,sizeof(LogEntry),(uint8_t*)&entry);
}

/*void ObsoleteLogEntry(LogEntry* pEntry,uint16_t size)
{
   pEntry->addFile.obsolete = 0;
   flash_write(BLOCK_SIZE*gLog.logEUListIndex + gLog.logEntryOffset - size,size,(uint8_t*)pEntry);
}*/

FS_STATUS writeFile(const char* filename, unsigned length, const char* data,uint8_t euIndex)
{
   uint16_t sectorDescAddr = BLOCK_SIZE*euIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*gEUList[euIndex].totalDescriptors;

   uint16_t dataAddr = BLOCK_SIZE*euIndex + gEUList[euIndex].nextFreeOffset - length;

   SectorDescriptor desc;

   EraseUnitHeader header;

   memset(&desc,0xFF,sizeof(SectorDescriptor));

   //mark prev sector as non last
   if (gEUList[euIndex].totalDescriptors != 0)
   {

	   memset(&desc,0xFF,sizeof(SectorDescriptor));
	   desc.metadata.bits.lastDesc = 0;
	   flash_write(sectorDescAddr - sizeof(SectorDescriptor),sizeof(SectorDescriptor),(uint8_t*)&desc);
       desc.metadata.bits.lastDesc = 1;
   }
   else
   {
	   memset(&header,0xFF,sizeof(EraseUnitHeader));
	   header.metadata.bits.emptyEU = 0;
	   flash_write(BLOCK_SIZE*euIndex,sizeof(EraseUnitHeader),(uint8_t*)&header);
   }

   strncpy(desc.fileName,filename,FILE_NAME_MAX_LEN);
   desc.offset = gEUList[euIndex].nextFreeOffset - length;
   desc.size = length;

   //write entire descriptor
   flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);
   //mark descriptor metadata as valid
   desc.metadata.bits.validOffset = 0;
   flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);


   //write file content
   flash_write(dataAddr,length,(uint8_t*)data);
   //mark entire file as valid
   desc.metadata.bits.validDesc = 0;
   flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);
      
   gEUList[euIndex].bytesFree -= (sizeof(SectorDescriptor) + length);
   gEUList[euIndex].nextFreeOffset-=length;
   gEUList[euIndex].totalDescriptors++;
   gEUList[euIndex].validDescriptors++;

   return SUCCESS;
}

bool findFreeSector(uint16_t fileSize,uint8_t* euIndex)
{
   uint8_t i;
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

//first fit algo
FS_STATUS fs_write(const char* filename, unsigned length, const char* data)
{
   uint8_t i,euIndex;
   uint16_t address;
   
   
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

   if (findFreeSector(length,&euIndex))
   {
        eraseFileFromFS(filename);
        return writeFile(filename,length,data,euIndex);
   }
   else //if (relocateBlock(&euIndex)) - find if compaction will lead to succesful insertion
   {
        eraseFileFromFS(filename);
        //...
        return SUCCESS;
   }
   
   return MAXIMUM_FLASH_SIZE_EXCEEDED;
}

//assumption, file exist i.e euIndex is valid and not log and validSectorIndex <= number of valid files per this EU.
FS_STATUS getNextValidSectorDescriptor(uint8_t euIndex,uint16_t* pPrevActualSectorIndex,SectorDescriptor* pSecDesc)
{

   uint16_t baseAddress = BLOCK_SIZE*euIndex,secDescOffset = sizeof(EraseUnitHeader) ,sectorIndexInc = 0;
   
   if (pPrevActualSectorIndex != NULL && *pPrevActualSectorIndex > 0)
   {
      secDescOffset += sizeof(SectorDescriptor)*(*pPrevActualSectorIndex + 1);
       ++sectorIndexInc;
   }

   do
   {
      flash_read(baseAddress + secDescOffset,sizeof(SectorDescriptor),(uint8_t*)pSecDesc);
      secDescOffset+=sizeof(SectorDescriptor);
      ++sectorIndexInc;
   }while(pSecDesc->metadata.bits.validDesc != 0 || pSecDesc->metadata.bits.obsoleteDes == 0 && secDescOffset + sizeof(SectorDescriptor) < BLOCK_SIZE);

   //cancle last increment, since we want the actual index of the current file and not the next
   --sectorIndexInc;

   if (pPrevActualSectorIndex != NULL)
   {
      *pPrevActualSectorIndex+=sectorIndexInc;
   }

   return SUCCESS;
}

bool isFilenameMatchs(SectorDescriptor *pSecDesc,const char* filename)
{
   char fsFileName[FILE_NAME_MAX_LEN+1] = {'\0'};
   memcpy(fsFileName,pSecDesc->fileName,FILE_NAME_MAX_LEN);
   return strcmp(fsFileName,filename)==0;
}

FS_STATUS getSectorDescriptor(const char* filename,SectorDescriptor *pSecDesc,uint8_t *pEuIndex,uint16_t* pSecActualIndex)
{
   uint16_t secIdx,lastActualSecIdx;
   bool found = false;
   uint8_t euIdx;

   for(euIdx = 0 ; euIdx < ERASE_UNITS_NUMBER ; ++euIdx)
   {
      //skip the log EU
      if (gLogIndex == euIdx)
      {
         continue;
      }

      lastActualSecIdx = 0;

      for(secIdx = 0 ; secIdx < gEUList[euIdx].validDescriptors ; ++secIdx)
      {
         getNextValidSectorDescriptor(euIdx,&lastActualSecIdx,pSecDesc);
         if (isFilenameMatchs(pSecDesc,filename))
         {
            found = true;
            break;
         }
      }

      if (found)
      {
         break;
      }
   }

   if (found)
   {
      if (pEuIndex != NULL)
      {
         *pEuIndex = euIdx;
      }
      if (pSecActualIndex != NULL)
      {
         *pSecActualIndex = lastActualSecIdx;
      }
   }

   return found?SUCCESS:FILE_NOT_FOUND;
}

FS_STATUS fs_filesize(const char* filename, unsigned* length)
{
   SectorDescriptor sec;
   FS_STATUS status;
   uint8_t secEuIdx;
   
   status = getSectorDescriptor(filename,&sec,&secEuIdx,NULL);

   if (status != SUCCESS)
   {
      return status;
   }

   *length = sec.size;
   
   return SUCCESS;
   
}

FS_STATUS fs_read(const char* filename, unsigned* length, char* data)
{
   SectorDescriptor sec;
   FS_STATUS status;
   uint8_t secEuIndex;
   
   status = getSectorDescriptor(filename,&sec,&secEuIndex,NULL);

   if (status != SUCCESS)
   {
      return status;
   }

   
   *length = sec.size;
   flash_read(BLOCK_SIZE*secEuIndex + sec.offset,sec.size,(uint8_t*)data); //TODO retvalue
   
   return SUCCESS;
   
}

FS_STATUS eraseFileFromFS(const char* filename)
{
   SectorDescriptor sec;
   FS_STATUS status;
   uint16_t actualSecIndex;
   uint8_t secEuIndex;
   
   status = getSectorDescriptor(filename,&sec,&secEuIndex,&actualSecIndex);

   if (status != SUCCESS)
   {
      return status;
   }

   sec.metadata.bits.obsoleteDes = 0;

   flash_write(BLOCK_SIZE*secEuIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*actualSecIndex,sizeof(SectorDescriptor),(uint8_t*)&sec); //TODO retvalue

   --gEUList[secEuIndex].validDescriptors;
   gEUList[secEuIndex].deleteFilesTotalSize+=(sec.size + sizeof(SectorDescriptor));


   return SUCCESS;

}

FS_STATUS fs_erase(const char* filename)
{
    FS_STATUS status;
    //take lock
    status = eraseFileFromFS(filename);
    //release lock
    return status;
}

FS_STATUS fs_count(unsigned* file_count)
{
   //take the lock
   *file_count = gFilesCount;
}

FS_STATUS fs_list(unsigned* length, char* files)
{
   SectorDescriptor secDesc;
   uint16_t secIdx,lastActualSecIdx,ansIndex = 0;
   bool found = false;
   uint8_t euIdx;
   uint8_t filenameLen;
   char fsFilename[FILE_NAME_MAX_LEN+1] = {'\0'};

   if (files == NULL)
   {
	   return COMMAND_PARAMETERS_ERROR;
   }

   for(euIdx = 0 ; euIdx < ERASE_UNITS_NUMBER ; ++euIdx)
   {
      //skip the log EU
	   if (gLogIndex == euIdx || gEUList[euIdx].validDescriptors == 0)
      {
         continue;
      }

      lastActualSecIdx = 0;

      for(secIdx = 0 ; secIdx < gEUList[euIdx].validDescriptors ; ++secIdx)
      {
         getNextValidSectorDescriptor(euIdx,&lastActualSecIdx,&secDesc);
         strncpy(fsFilename,secDesc.fileName,FILE_NAME_MAX_LEN);
         filenameLen = strlen(fsFilename);

         if ((ansIndex + filenameLen + 1) > *length)
         {
             return FAILURE;
         }
         strcpy(&files[ansIndex],fsFilename);
         ansIndex+=(filenameLen+1);//+1 for the terminator \0
      }
   }

   *length  = ansIndex;

   
   return SUCCESS;
}


void test()
{
   FS_STATUS status;
   char data1[MAX_FILE_SIZE] = {'a'};
   char data2[MAX_FILE_SIZE] = {'b'};
   char data3[MAX_FILE_SIZE] = {'c'};
   char data4[MAX_FILE_SIZE] = {'d'};
   char data5[MAX_FILE_SIZE] = {'e'};
   char data6[MAX_FILE_SIZE] = {'f'};

   char data[MAX_FILE_SIZE];
   char filenames[(FILE_NAME_MAX_LEN+1)*1000];
   unsigned filenamesSize = (FILE_NAME_MAX_LEN+1)*1000;
   uint32_t fileSize;
   uint32_t i;

   loadFilesystem();

   /*for(i = 0 ; i < 1000 ; ++i)
   {
       status = fs_write("file_a",0,data1);
       if (status != SUCCESS)
       {
           i = i;
       }
   }*/

   fs_write("file_a",1,data1);
   fs_write("file_b",1,data2);
   fs_write("file_c",1,data3);
   fs_write("file_d",1,data4);
   fs_write("file_e",1,data5);
   fs_write("file_f",1,data6);

   /*fs_read("file_a",&fileSize,data);
   fs_read("file_b",&fileSize,data);
   fs_read("file_c",&fileSize,data);
   fs_read("file_d",&fileSize,data);
   fs_read("file_e",&fileSize,data);
   fs_read("file_f",&fileSize,data);
   fs_read("file_g",&fileSize,data);

   fs_erase("file_a");
   fs_erase("file_t");
   fs_read("file_a",&fileSize,data);*/
   filenamesSize = 42;
   fs_list(&filenamesSize,filenames);
   
   
   /*
   LogEntry e = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
   int a = sizeof(LogEntry);

   e.moveBlock.eraseNumber = 0;
   e.moveBlock.bits1.blockBeenDeleted = 0;
   e.moveBlock.bits1.obsolete = 0;
   e.moveBlock.bits1.sectorToStart = 0;
   e.moveBlock.bits1.type = 0;
   e.moveBlock.bits1.valid = 0;
   e.moveBlock.bits2.euFromIndex = 0;
   e.moveBlock.bits2.euToIndex = 0;
   */

}