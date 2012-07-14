#include "fs.h"
#include "flash.h"
#include <string.h>

#define FILE_NAME_MAX_LEN (8)
#define ERASE_UNITS_NUMBER (FLASH_SIZE/BLOCK_SIZE)
#define wait_for_flash_done

typedef enum
{
	LOG		= 0,
	DATA	= 1,
}EraseUnitType;

#pragma pack(1)
typedef struct
{
	union
	{
		uint8_t data;
		struct
		{
			uint8_t type		: 2; //log (0) , regular sector (1), free (2)
			uint8_t lastHeader	: 1;
			uint8_t valid		: 1;
			uint8_t	reserved	: 5;
		} bits;
	}metadata;

	uint32_t eraseNumber;

	//this section should store the index of the new log and the new EraseNumber of EU[index]. this data important
	//in case of crash during log update, this info will enable to continue the operation.

	//when searching for "free" temp, zero all none 0xFF... temp values. if current temp & our temp value == our value, use it
	//this will solve the case when system crashes after writing temp, and eventually we will run out free temps.

	//from log erase to log erase we can be sure that free temp will be available because to make log full, the number
	//of required operation will cause more than one block to be zeroed (will create available temp)
	union
	{
		uint8_t data[5];
		struct
		{
			uint8_t edIndex;
			uint32_t edEraseNumber;
		}bytes;
	}temp;
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
			uint8_t	lastDes		:	1;
			uint8_t validDes	:	1;
			uint8_t obsoleteDes	:	1;
		} bits;
	}metadata;

	char fileName[FILE_NAME_MAX_LEN];

}SectorDescriptor;

#pragma pack()

typedef struct
{
	uint16_t bytesFree;
	uint16_t validDescriptors;
	uint16_t nextFreeOffset;
	uint32_t eraseNumber;
}EraseUnitLogicalMetadata;

/*
typedef struct
{

}LogEntry;
*/

EraseUnitLogicalMetadata gEUList[ERASE_UNITS_NUMBER];

typedef struct
{
	uint16_t logEntryAddress;
	uint8_t logEUListIndex;

}Log;

Log gLog;


//if no log found during initilization, erase the whole flash and install the filesystem


void intstallFileSystem()
{
	int i;
	uint16_t euAddress = 0;
	EraseUnitHeader header;
	header.eraseNumber = 0;
	header.metadata.data = 0xFF;
	header.metadata.bits.type = DATA;

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
		euAddress+=BLOCK_SIZE;
	}

	//3. write log header + logical metadata
	header.metadata.bits.type = LOG;
	flash_write(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);

	gLog.logEntryAddress = i;
	gLog.logEntryAddress = sizeof(EraseUnitHeader);
}

void loadFilesystem()
{
	int i;
	uint16_t euAddress = 0;

	memset(gEUList,0x00,sizeof(gEUList));

	EraseUnitHeader header;

	for(i = 0 ; i < (ERASE_UNITS_NUMBER-1) ; ++i)
	{
		flash_read(euAddress,sizeof(EraseUnitHeader),(uint8_t*)&header);
		gEUList[i].bytesFree = BLOCK_SIZE-sizeof(EraseUnitHeader);
		gEUList[i].eraseNumber = 0;
		gEUList[i].nextFreeOffset = BLOCK_SIZE;
		gEUList[i].validDescriptors = 0;
		euAddress+=BLOCK_SIZE;
	}


}