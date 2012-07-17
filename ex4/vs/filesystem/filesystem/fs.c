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

typedef enum
{
	LOG		= 0,
	DATA	= 1,
}EraseUnitType;

typedef enum
{
   ADD_FILE  = 0,
   BLOCK_MOVE = 1,
}LogEntryType;

#define MY_OFFSET(type, field) ((unsigned long ) &(((type *)0)->field))

#pragma pack(1)

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

typedef struct
{
	union
	{
		uint8_t data;
		struct
		{
			uint8_t type		   : 1; //log = 0, sectors = 1
			uint8_t lastHeader   : 1; //1 = yes, 0 = no
			uint8_t valid		   : 1; // whole eu valid
         uint8_t obsolete     : 1; // whole eu oblolete
			uint8_t	reserved	   : 4;
		} bits;
	}metadata;

	uint32_t eraseNumber;

	//this section should store the index of the new log and the new EraseNumber of EU[index]. this data important
	//in case of crash during log update, this info will enable to continue the operation.

	//when searching for "free" temp, zero all none 0xFF... temp values. if current temp & our temp value == our value, use it
	//this will solve the case when system crashes after writing temp, and eventually we will run out free temps.

	//from log erase to log erase we can be sure that free temp will be available because to make log full, the number
	//of required operation will cause more than one block to be zeroed (will create available temp)
	HeaderTemp temp;
}EraseUnitHeader;

typedef struct
{
	uint16_t offset;
	uint16_t size;

	union
	{
		uint8_t data;
		struct
		{
			uint8_t lastDesc		:	1;
			uint8_t validDesc	   :	1;
			uint8_t obsoleteDes	:	1;
         uint8_t validOffset	:	1;
         uint8_t	reserved	   :  4;
		} bits;
	}metadata;

	char fileName[FILE_NAME_MAX_LEN];

}SectorDescriptor;

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


#pragma pack()

typedef struct
{
	uint32_t eraseNumber;
   uint16_t bytesFree;
	uint16_t validDescriptors;
	uint16_t nextFreeOffset;
   uint16_t totalDescriptors;
	
   union
	{
		uint8_t data;
		struct
		{
			uint8_t type		   : 1; //log = 0, sectors = 1
			uint8_t lastHeader   : 1; //1 = yes, 0 = no
			uint8_t valid		   : 1; // whole eu valid
         uint8_t obsolete     : 1; // whole eu oblolete
			uint8_t	reserved	   : 4;
		} bits;
	}metadata;

}EraseUnitLogicalMetadata;




EraseUnitLogicalMetadata gEUList[ERASE_UNITS_NUMBER];

typedef struct
{
	uint16_t logEntryOffset;
	uint8_t logEUListIndex;

}Log;

Log gLog;
uint16_t gFilesCount = 0;

//if no log found during initilization, erase the whole flash and install the filesystem


void intstallFileSystem()
{
	int i;
	uint16_t euAddress = 0;
   EraseUnitHeader header;

   memset(&header,0xFF,sizeof(EraseUnitHeader));

	header.eraseNumber = 0;
	header.metadata.bits.type = DATA;
   header.metadata.bits.valid = 0;
	//1. erase the whole flash
	flash_bulk_erase_start();
	wait_for_flash_done;
	//2. write each EraseUnit header + logical metadata
	for(i = 0 ; i < (ERASE_UNITS_NUMBER-1) ; ++i)
	{
      flash_write(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);
		gEUList[i].bytesFree = BLOCK_SIZE-sizeof(EraseUnitHeader);
		gEUList[i].eraseNumber = 0;
		gEUList[i].nextFreeOffset = BLOCK_SIZE;
		gEUList[i].validDescriptors = 0;
      gEUList[i].totalDescriptors = 0;
		euAddress+=BLOCK_SIZE;
	}
   header.metadata.bits.valid = 1;
	//3. write log header + logical metadata
	header.metadata.bits.type = LOG;
	flash_write(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);
   header.metadata.bits.valid = 0;
   flash_write(euAddress + MY_OFFSET(EraseUnitHeader,metadata.data),sizeof(header.metadata.data),&header.metadata.data);

   gLog.logEUListIndex = i;
   gLog.logEntryOffset = sizeof(EraseUnitHeader);
   gFilesCount = 0;
}

void ParseEraseUnitContent(uint16_t eraseUnitIndex,uint16_t* validDescriptors,uint16_t* totalDescriptors,uint16_t* dataBytesInUse)
{
   uint16_t baseAddress = eraseUnitIndex*BLOCK_SIZE;
   EraseUnitHeader header;
   SectorDescriptor sd;

   *validDescriptors = 0;
   *dataBytesInUse = 0;



   flash_read(baseAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);

   if (header.metadata.bits.lastHeader == 0) //there are some Sector Desciptors
   {
      baseAddress += sizeof(EraseUnitHeader);



      do
      {
         flash_read(baseAddress,sizeof(SectorDescriptor),(uint8_t*)&sd);
         
         if (sd.metadata.bits.validOffset == 0) //metadata been verified
         {
            *dataBytesInUse+=sd.size;
         }

         if (sd.metadata.bits.validDesc == 0 && sd.metadata.bits.obsoleteDes == 1)
         {
            ++*validDescriptors;
         }
         ++*totalDescriptors;
         baseAddress += sizeof(SectorDescriptor);

      }while(!sd.metadata.bits.lastDesc);
   }
}

void handleLogSelfMigration(uint8_t headerHostIndex,HeaderTemp h)
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

void parseLog()
{
   LogEntry e;
   LogEntry lastEntry;
   uint16_t entryBaseLine = gLog.logEUListIndex * BLOCK_SIZE;

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
//this function loads filesystem structure from FLASH
//need to verify that:
// 1. only one valid log exists
// 2. log is empty or last log entry is obsolete (we didn't crash in the middle of an operation)
void loadFilesystem()
{
	uint8_t i,firstLogIndex,secondLogIndex,tempHostIndex = 0;
	uint16_t euAddress = 0;
   HeaderTemp headerTemp;
   EraseUnitHeader header;
   bool logExists = false;
   bool logExistsTwice = false;
   bool headerTempFound = false;

   flash_init(NULL,NULL);

   memset(gEUList,0x00,sizeof(gEUList));

	



	for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
	{
      uint16_t dataBytesInUse = 0;
		flash_read(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);
      
      if (header.temp.bytes.metadata.valid == 0 && header.temp.bytes.metadata.obsolete == 1)
      {
         headerTemp = header.temp;
         headerTempFound = true;
         tempHostIndex = i;
      }

      gEUList[i].eraseNumber = header.eraseNumber;
      gEUList[i].metadata.data = header.metadata.data;
	
      //if we found valid log
      if (header.metadata.bits.type == LOG && header.metadata.bits.valid == 0 && header.metadata.bits.obsolete == 1)
      {
         if (logExists)
         {
            logExistsTwice = true;
            secondLogIndex = i;
            }
         else
         {
            logExists = true;
            firstLogIndex = i;
         }
      }

      if (header.metadata.bits.valid == 0 && header.metadata.bits.obsolete == 1)
      {
         ParseEraseUnitContent(i,&gEUList[i].validDescriptors,&gEUList[i].validDescriptors,&dataBytesInUse);
         gEUList[i].nextFreeOffset = BLOCK_SIZE - dataBytesInUse;
         gEUList[i].bytesFree = BLOCK_SIZE-sizeof(EraseUnitHeader) - sizeof(SectorDescriptor)*gEUList[i].validDescriptors - dataBytesInUse;
      }

      gFilesCount+=gEUList[i].validDescriptors;
      

		euAddress+=BLOCK_SIZE;
	}

   //if no log found, fs not exist, create one
   if (!logExists && headerTempFound == false)
   {
      intstallFileSystem();
   }

   if (headerTempFound)
   {
      handleLogSelfMigration(tempHostIndex,headerTemp);
   }
   else
   {
      parseLog();
   }
   
   //check if valid temp exist, error while reasing block and attempting to create new log on the same EU

   //check last entry at the Log and complete it

}


FS_STATUS fs_init(const FS_SETTINGS settings)
{
   loadFilesystem();
}


void test()
{
   int a = MY_OFFSET(EraseUnitHeader,eraseNumber);
   a = MY_OFFSET(EraseUnitHeader,metadata.data);
   loadFilesystem();
   loadFilesystem();
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
void AddLogEntry(LogEntry* pEntry,uint16_t size)
{
   uint16_t entryAbsAddr;

   if (gLog.logEntryOffset + sizeof(MoveBlockEntry) >= (FLASH_SIZE-sizeof(MoveBlockEntry)))
   {
      //TODO create new log, (add log entry if needed)
   }

   //calc log entry address
   entryAbsAddr = BLOCK_SIZE*gLog.logEUListIndex + gLog.logEntryOffset;

   //write entry
   flash_write(entryAbsAddr,size,(uint8_t*)pEntry);

   //write valid bit
   pEntry->addFile.valid = 0;

   //TODO check if we can write the specific byte only
   flash_write(entryAbsAddr,size,(uint8_t*)pEntry);

   gLog.logEntryOffset+=size;
}

void ObsoleteLogEntry(LogEntry* pEntry,uint16_t size)
{
   pEntry->addFile.obsolete = 0;
   flash_write(BLOCK_SIZE*gLog.logEUListIndex + gLog.logEntryOffset - size,size,(uint8_t*)pEntry);
}

void writeFile(const char* filename, unsigned length, const char* data,uint8_t euIndex)
{
   LogEntry entry;

   uint16_t sectorDescAddr = BLOCK_SIZE*euIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*gEUList[euIndex].totalDescriptors;

   uint16_t dataAddr = BLOCK_SIZE*euIndex + gEUList[euIndex].nextFreeOffset - length;

   SectorDescriptor desc;

   memset(&entry,0xFF,sizeof(LogEntry));
   memset(&desc,0xFF,sizeof(SectorDescriptor));

   entry.addFile.euIndex = euIndex;
   entry.addFile.sectorIndex = gEUList[euIndex].totalDescriptors;
   entry.addFile.type = ADD_FILE;

   AddLogEntry(&entry,sizeof(AddFileEntry));

   strncpy(desc.fileName,filename,FILE_NAME_MAX_LEN);
   desc.offset = gEUList[euIndex].nextFreeOffset - length;
   desc.size = length;

   flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);
   desc.metadata.bits.validDesc = 0;
   flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);

   flash_write(dataAddr,length,(uint8_t*)data);
   desc.metadata.bits.validDesc = 0;
   flash_write(sectorDescAddr,sizeof(SectorDescriptor),(uint8_t*)&desc);
      

   //mark prev sector as non last
   memset(&desc,0xFF,sizeof(SectorDescriptor));
   desc.metadata.bits.lastDesc = 0;
   flash_write(sectorDescAddr - sizeof(SectorDescriptor),sizeof(SectorDescriptor),(uint8_t*)&desc);

   ObsoleteLogEntry(&entry,sizeof(AddFileEntry));

   gEUList[euIndex].bytesFree -= (sizeof(SectorDescriptor) + length);
   gEUList[euIndex].nextFreeOffset-=length;
   gEUList[euIndex].totalDescriptors++;
   gEUList[euIndex].validDescriptors++;

}

bool findFreeSector(uint16_t fileSize,uint8_t* euIndex)
{
   uint8_t i;
   for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
   {
      if (gLog.logEntryOffset != i && gEUList[i].bytesFree >= fileSize + sizeof(SectorDescriptor))
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
      //...
      return SUCCESS;
   }
   else //if (relocateBlock(&euIndex))
   {
      //...
      return SUCCESS;
   }
   
   return MAXIMUM_FLASH_SIZE_EXCEEDED;
}

//assumption, file exist i.e euIndex is valid and not log and validSectorIndex <= number of valid files per this EU.
FS_STATUS getSectorDescriptorByIndices(uint8_t euIndex,uint16_t validSectorIndex,uint16_t* pPrevActualSectorIndex,SectorDescriptor* pSecDesc)
{

   uint16_t address,sectorIndexInc = 0;
   
   if (pPrevActualSectorIndex != NULL && *pPrevActualSectorIndex !=0)
   {
      address = BLOCK_SIZE*euIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*(*pPrevActualSectorIndex);
   }
   else
   {
      address = BLOCK_SIZE*euIndex + sizeof(EraseUnitHeader) + sizeof(SectorDescriptor)*validSectorIndex;
   }


   do
   {
      flash_read(address,sizeof(SectorDescriptor),(uint8_t*)pSecDesc);
      address+=sizeof(SectorDescriptor);
      ++sectorIndexInc;
   }while(pSecDesc->metadata.bits.validDesc != 0 || pSecDesc->metadata.bits.obsoleteDes == 0);

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
      if (gLog.logEUListIndex == euIdx)
      {
         continue;
      }

      lastActualSecIdx = 0;

      for(secIdx = 0 ; secIdx < gEUList[euIdx].validDescriptors ; ++secIdx)
      {
         getSectorDescriptorByIndices(euIdx,secIdx,&lastActualSecIdx,pSecDesc);
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

FS_STATUS fs_erase(const char* filename)
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

   return SUCCESS;

}

FS_STATUS fs_count(unsigned* file_count)
{
   //take the lock
   *file_count = gFilesCount;
}

FS_STATUS fs_list(unsigned length, char* files)
{
   return SUCCESS;
}