#include "flash.h"
#include "common_defs.h"
#include "dbg_assert.h"

#ifndef NULL
#define NULL 0x0
#endif

#define FLASH_CAPACITY (64*1024)
#define FLASH_BLOCK_SIZE (4*1024)

//flash HW definitions
#define NUM_OF_DATA_REG 16
#define FLASH_BASE_ADDR 0x150
#define DATA_REG_BYTES NUM_OF_DATA_REG*sizeof(uint32_t)

//flash REG addresses
#define FLASH_STATUS_REG_ADDR (FLASH_BASE_ADDR+0x0)
#define FLASH_CONTROL_REG_ADDR (FLASH_BASE_ADDR+0x1)
#define FLASH_ADDRESS_REG_ADDR (FLASH_BASE_ADDR+0x2)
#define FLASH_FDATA_BASE_ADDR (FLASH_BASE_ADDR+0x3)

//wait for the flash hardware to be idle
#define wait_for_flash_to_be_idle while(!flashIsIdle())

#define MIN(a,b) ((a)>(b)?(b):(a))

//buffer to store non-blocking data
typedef struct
{
	uint32_t size;
	uint8_t data[MAX_REQUEST_BUFFER_SIZE];
}FlashBuffer;

#pragma pack(1)

typedef union
{
	uint32_t data;
	struct
	{
		const uint32_t SCIP			:1;
		uint32_t cycleDone			:1;
		const uint32_t reserved		:30;
	}bits;
}FlashStatusRegister;

typedef union
{
	uint32_t data;
	struct
	{
		uint32_t SCGO				:1;
		uint32_t SME				:1;
		const uint32_t reserved1	:6;
		uint32_t CMD				:8;
		uint32_t FDBC				:6;
		const uint32_t reserved2	:10;
	}bits;
}FlashControlRegister;

#pragma pack

typedef enum
{
	IDLE						= 0x00,
	BLOCKING_READ_DATA			= 0x01,
	NONE_BLOCKING_READ_DATA		= 0x02,
	READ_DATA					= 0x03,
	PAGE_PROGRAM 				= 0x05,
	BLOCKING_PAGE_PROGRAM		= 0x06,
	NONE_BLOCKING_PAGE_PROGRAM	= 0x07,
	BLOCK_ERASE					= 0xD8,
	BULK_ERASE					= 0xC7,
}SPILogicCommand;

//callbacks pointers
void (*gpFlashDataReciveCB)(uint8_t const *,uint32_t) = NULL;
void (*gpFlashRequestDoneCB)(void) = NULL;



SPILogicCommand gCurrentCommand;

//globals to handle none blocking read and program
FlashBuffer gNoneBlockingBuffer;
uint16_t gNoneBlockingOpSize;
uint16_t gNoneBlockingStartAddress;

//FWD declaration
void loadDataFromRegs(uint8_t buffer[],const uint16_t size);
void loadDataToRegs(const uint8_t buffer[],const uint16_t size);


//this function starts none blocking read operation. this function can read up to DATA_REG_BYTES bytes
void startNoneBlockingRead()
{

	uint16_t transactionBytes = MIN(gNoneBlockingOpSize-gNoneBlockingBuffer.size,DATA_REG_BYTES);
	FlashControlRegister cr;

	cr.bits.CMD = READ_DATA;

	cr.bits.FDBC = transactionBytes-1;

	//enable interrupts
	cr.bits.SME = 1;

	//go bit
	cr.bits.SCGO = 1;

	//set address to read from
	_sr(gNoneBlockingStartAddress+gNoneBlockingBuffer.size,FLASH_ADDRESS_REG_ADDR);
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);


}

//finish none blocking read, update all relevant counters
bool finishNoneBlockingRead()
{
	//calculate how many bytes should been read
	uint16_t transactionBytes = MIN(gNoneBlockingOpSize-gNoneBlockingBuffer.size,DATA_REG_BYTES);

	//load read data from FDATA regs into our internal buffer
	loadDataFromRegs(gNoneBlockingBuffer.data + gNoneBlockingBuffer.size,transactionBytes);

	//advance buffer's suze
	gNoneBlockingBuffer.size+=transactionBytes;

	//return true iff we read all the bytes
	return gNoneBlockingBuffer.size==gNoneBlockingOpSize;
}

//perform up to DATA_REG_BYTES bytes none blocking write
void performNoneBlockingWrite()
{
	uint16_t transactionBytes;
	FlashControlRegister cr;

	//calculate transaction size in bytes
	transactionBytes = MIN(gNoneBlockingBuffer.size-gNoneBlockingOpSize,DATA_REG_BYTES);

	//set command type
	cr.bits.CMD = PAGE_PROGRAM;

	//set transaction size
	cr.bits.FDBC = transactionBytes-1;

	//enable interrupts
	cr.bits.SME = 1;

	//start operation
	cr.bits.SCGO = 1;

	//read data from the given buffer to data registers
	loadDataToRegs(gNoneBlockingBuffer.data+gNoneBlockingOpSize,transactionBytes);

	//set flash write address
	_sr(gNoneBlockingStartAddress+gNoneBlockingOpSize,FLASH_ADDRESS_REG_ADDR);

	//advance number of written bytes
	gNoneBlockingOpSize+=transactionBytes;

	//perform writing
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);
}


//this is the interrupt service routine
_Interrupt1 void flashISR()
{
	if ( ! (_lr(0X151) & 0x2) )
	{
		_sr( 0x2, 0x150 );
		return;
	}
	//acknowledge the interrupt
	FlashStatusRegister sr = {0};
	sr.bits.cycleDone = 1;
	_sr(sr.data,FLASH_STATUS_REG_ADDR);

	FlashControlRegister cr = {0};
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);

	//no need to handle those commands since they block
	if (gCurrentCommand == BLOCKING_READ_DATA || gCurrentCommand == BLOCKING_PAGE_PROGRAM || gCurrentCommand == IDLE)
	{
		return;
	}

	//handle the interrupt
	switch (gCurrentCommand)
	{
	case NONE_BLOCKING_READ_DATA: //non-blocking read

		//load data from regs to buffer and check if whole read is done
		if (finishNoneBlockingRead())
		{
			gCurrentCommand = IDLE;
			gpFlashDataReciveCB(gNoneBlockingBuffer.data,gNoneBlockingBuffer.size);

		}
		//continue to the next transaction
		else
		{
			startNoneBlockingRead();
		}
		break;

	case NONE_BLOCKING_PAGE_PROGRAM: //non-blocking write

		//check if whole data been written
		if (gNoneBlockingOpSize == gNoneBlockingBuffer.size)
		{
			gCurrentCommand = IDLE;
			gpFlashRequestDoneCB();
		}
		//continue to the next transaction
		else
		{
			performNoneBlockingWrite();
		}
		break;
	case BLOCK_ERASE:
	case BULK_ERASE:
		//erase done
		gCurrentCommand = IDLE;
		gpFlashRequestDoneCB();
		break;
	default:
		DBG_ASSERT(false);

	}

	return;


}

result_t flash_init(	void (*flash_data_recieve_cb)(uint8_t const *buffer, uint32_t size),
						void (*flash_request_done_cb)(void))
{
	if (!flash_data_recieve_cb || !flash_request_done_cb)
	{
		return NULL_POINTER;
	}
	else
	{
		//store the cb
		gpFlashDataReciveCB = flash_data_recieve_cb;
		gpFlashRequestDoneCB = flash_request_done_cb;

		return OPERATION_SUCCESS;
	}
}			 

//return true if hardware is idle
bool flashIsIdle(void)
{
	FlashStatusRegister cr = {_lr(FLASH_STATUS_REG_ADDR)};

	return cr.bits.SCIP==0;
}

//return true of the whole flash is ready for the next command
bool flash_is_ready(void)
{
	//hardware idle and we are not in a middle of big operation
	return (gCurrentCommand==IDLE && flashIsIdle());
}

result_t flash_read_start(uint16_t start_address, uint16_t size)
{

	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);

	if (size==0 || size > MAX_REQUEST_BUFFER_SIZE)
	{
		return INVALID_ARGUMENTS;
	}

	//check if flash is ready
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}

	gCurrentCommand = NONE_BLOCKING_READ_DATA;
	_enable();

	//set the blocking globals with the transaction's data
	gNoneBlockingBuffer.size = 0;
	gNoneBlockingOpSize = size;
	gNoneBlockingStartAddress = start_address;

	startNoneBlockingRead();

	return OPERATION_SUCCESS;

}

//load size bytes from FDATA regs to given buffer
void loadDataFromRegs(uint8_t buffer[],const uint16_t size)
{

	uint32_t i=0,j,regIndex = 0,regData;

	uint8_t* pRegData = (uint8_t*)&regData;

	while(i < size)
	{
		DBG_ASSERT(regIndex < NUM_OF_DATA_REG);

		regData = _lr(FLASH_FDATA_BASE_ADDR+regIndex);

		//every register is 4 bytes (32 bits), break it to 4 bytes and copy each byte
		//to our buffer
		for(j = 0 ; j < 4 && i < size ; ++i,++j)
		{
			buffer[i] = pRegData[j];
		}

		regIndex++;
	}
}

result_t flash_read(uint16_t start_address, uint16_t size, uint8_t buffer[])
{
	uint16_t readBytes = 0;
	uint16_t transactionBytes;
	FlashControlRegister cr = {0};

	DBG_ASSERT(start_address + size*8 >= FLASH_CAPACITY);

	if (!buffer)
	{
		return NULL_POINTER;
	}

	if (size==0 || size > MAX_REQUEST_BUFFER_SIZE )
	{
		return  INVALID_ARGUMENTS;
	}

	//check if flash is ready
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	//change to the current command
	gCurrentCommand = BLOCKING_READ_DATA;
	_enable();

	cr.bits.CMD = READ_DATA;

	//
	while(readBytes < size)
	{
		//locate the maximal number of bytes that can be loaded from
		//the flash in one request
		transactionBytes = MIN(size-readBytes,DATA_REG_BYTES);

		cr.bits.FDBC = transactionBytes-1;

		//no need for interrupts since this is blocking function
		cr.bits.SME = 0;

		cr.bits.SCGO = 1;

		//set the address to read from
		_sr(start_address+readBytes,FLASH_ADDRESS_REG_ADDR);
		_sr(cr.data,FLASH_CONTROL_REG_ADDR);

		//wait for the flash hardware to be idle
		wait_for_flash_to_be_idle;

		//load the date from the flash regs
		loadDataFromRegs(buffer+readBytes,transactionBytes);

		//update the number of bytes that was read
		readBytes+=transactionBytes;
	}
	gCurrentCommand = IDLE;

	return OPERATION_SUCCESS;



}

result_t flash_write_start(uint16_t start_address, uint16_t size, const uint8_t buffer[])
{
	uint16_t i;
	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);

	if (!buffer)
	{
		return NULL_POINTER;
	}

	if (size==0 || size > MAX_REQUEST_BUFFER_SIZE)
	{
		return  INVALID_ARGUMENTS;
	}

	//check if flash is ready
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}

	//change to the current command
	gCurrentCommand = NONE_BLOCKING_PAGE_PROGRAM;
	_enable();

	//copy data to internal buffer
	for(i = 0 ; i < size ; ++i)
	{
		gNoneBlockingBuffer.data[i] = buffer[i];
	}

	//set the non blocking globals with the transaction data
	gNoneBlockingBuffer.size = size;
	gNoneBlockingOpSize = 0;
	gNoneBlockingStartAddress = start_address;

	performNoneBlockingWrite();

	return OPERATION_SUCCESS;
}

/*
 *  load size bytes from given buffer to FDATA regs
 */
void loadDataToRegs(const uint8_t buffer[],const uint16_t size)
{
	uint16_t i,regIndex;

	for(i = 0 , regIndex = 0 ; i < size ; i+=4 , ++regIndex)
	{
		//store 4 bytes from our buffer into each FDATA 32bits registers (4 bytes each)
		_sr((*(uint32_t*)(buffer+i)),FLASH_FDATA_BASE_ADDR+regIndex);
	}
}

result_t flash_write(uint16_t start_address, uint16_t size, const uint8_t buffer[])
{

	uint16_t writtenBytes = 0;
	uint16_t transactionBytes;
	FlashControlRegister cr = {0};

	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);

	if (!buffer)
	{
		return NULL_POINTER;
	}

	if (size==0 || size > MAX_REQUEST_BUFFER_SIZE)
	{
		return  INVALID_ARGUMENTS;
	}

	//check if flash is ready
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}

	//change to the current command
	gCurrentCommand = BLOCKING_PAGE_PROGRAM;
	_enable();

	cr.bits.CMD = PAGE_PROGRAM;

	while(writtenBytes < size)
	{
		//calculate the size of the current transaction
		transactionBytes = MIN(size-writtenBytes,DATA_REG_BYTES);

		//set control reg
		cr.bits.FDBC = transactionBytes-1;

		//no need for interrupts since this is blocking function
		cr.bits.SME = 0;

		cr.bits.SCGO = 1;

		//read data from the given buffer to data registers
		loadDataToRegs(buffer+writtenBytes,transactionBytes);

		//set flash write address
		_sr(start_address+writtenBytes,FLASH_ADDRESS_REG_ADDR);
		//perform writing
		_sr(cr.data,FLASH_CONTROL_REG_ADDR);

		//update the number of bytes that was written
		writtenBytes+=transactionBytes;

		//wait for the flash hardware to be idle
		wait_for_flash_to_be_idle;
	}

	gCurrentCommand = IDLE;

	return OPERATION_SUCCESS;
}


result_t flash_bulk_erase_start(void)
{
	FlashControlRegister cr = {0};

	//check if flash is ready
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}

	//change to the current command
	gCurrentCommand = BULK_ERASE;
	_enable();

	//prepare control reg
	cr.bits.CMD = BULK_ERASE;
	cr.bits.SME = 1;
	cr.bits.SCGO = 1;

	_sr(cr.data,FLASH_CONTROL_REG_ADDR);

	return OPERATION_SUCCESS;

}


result_t flash_block_erase_start(uint16_t start_address)
{

	DBG_ASSERT(start_address < FLASH_CAPACITY);

	FlashControlRegister cr = {0};

	//check if flash is ready
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}

	//change to the current command
	gCurrentCommand = BLOCK_ERASE;
	_enable();

	//prepare control reg
	cr.bits.CMD = BLOCK_ERASE;
	cr.bits.SME = 1;
	cr.bits.SCGO = 1;

	//set the address
	_sr(start_address,FLASH_ADDRESS_REG_ADDR);

	_sr(cr.data,FLASH_CONTROL_REG_ADDR);

	return OPERATION_SUCCESS;
}

