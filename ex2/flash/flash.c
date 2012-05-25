#include "flash.h"
#include "../common_defs.h"


#define FLASH_CAPACITY (64*1024)
#define FLASH_BLOCK_SIZE (4*1024)

#define NUM_OF_DATA_REG 16
#define FLASH_BASE_ADDR 0x150
#define DATA_REG_BYTES NUM_OF_DATA_REG*sizeof(uint32_t)


#define FLASH_STATUS_REG_ADDR (FLASH_BASE_ADDR+0x0)
#define FLASH_CONTROL_REG_ADDR (FLASH_BASE_ADDR+0x1)
#define FLASH_ADDRESS_REG_ADDR (FLASH_BASE_ADDR+0x2)
#define FLASH_FDATA_BASE_ADDR (FLASH_BASE_ADDR+0x3)

#define wait_for_flash_to_be_idle while(!flashIsIdle())

#define MIN(a,b) ((a)>(b)?(b):(a))

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
		const uint32_t SCIP		:1;
        uint32_t cycleDone	:1;
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
	IDLE			=	0x00,
	READ_DATA 		= 	0x03,
	PAGE_PROGRAM 	= 	0x05,
	BLOCK_ERASE 	= 	0xD8,
	BULK_ERASE 		= 	0xC7,
}SPICommand;


void (*gpFlashDataReciveCB)(uint8_t const *,uint32_t);
void (*gpFlashRequestDoneCB)(void);



SPICommand gCurrentCommand;

FlashBuffer gNonBlockingBuffer;
uint16_t gNonBlockingOpSize;
uint16_t gNonBlockingStartAddress;

//FWD declaration
void loadDataFromRegs(uint8_t buffer[],const uint16_t size);
void loadDataToRegs(const uint8_t buffer[],const uint16_t size);


void startNonBlockingRead()
{

	uint16_t transactionBytes = MIN(gNonBlockingOpSize-gNonBlockingBuffer.size,DATA_REG_BYTES);
	FlashControlRegister cr;

	cr.bits.CMD = READ_DATA;

	cr.bits.FDBC = transactionBytes-1;

	cr.bits.SME = 1;

	cr.bits.SCGO = 1;

	_sr(gNonBlockingStartAddress+gNonBlockingBuffer.size,FLASH_ADDRESS_REG_ADDR);
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);

	
}

void performNonBlockingWrite()
{
	uint16_t transactionBytes;
	FlashControlRegister cr;

	//calculate transaction size in bytes
	transactionBytes = MIN(gNonBlockingBuffer.size-gNonBlockingOpSize,DATA_REG_BYTES);

	//set command type
	cr.bits.CMD = PAGE_PROGRAM;

	//set transaction size
	cr.bits.FDBC = transactionBytes-1;

	//enable interrupts
	cr.bits.SME = 1;

	//start operation
	cr.bits.SCGO = 1;

	//read data from the given buffer to data registers
	loadDataToRegs(gNonBlockingBuffer.data+gNonBlockingOpSize,transactionBytes);

	//set flash write address
	_sr(gNonBlockingStartAddress+gNonBlockingOpSize,FLASH_ADDRESS_REG_ADDR);

	//advance number of written bytes
	gNonBlockingOpSize+=transactionBytes;

	//perform writing
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);
}


bool finishNonBlockingRead()
{
	uint16_t transactionBytes = MIN(gNonBlockingOpSize-gNonBlockingBuffer.size,DATA_REG_BYTES);

	loadDataFromRegs(gNonBlockingBuffer.data + gNonBlockingBuffer.size,transactionBytes);

	gNonBlockingBuffer.size+=transactionBytes;

	return gNonBlockingBuffer.size==gNonBlockingOpSize;
}

_Interrupt1 void flashISR()
{
	//acknowledge the interrupt
    FlashStatusRegister sr = {0};
    sr.bits.cycleDone = 1;
    _sr(sr.data,FLASH_STATUS_REG_ADDR);
	
	//handle the interrupt
	switch (gCurrentCommand)
	{
		case READ_DATA: //non-blocking read

			//load data from regs to buffer and check if whole read is done
			if (finishNonBlockingRead())
			{
				gCurrentCommand = IDLE;
				gpFlashDataReciveCB(gNonBlockingBuffer.data,gNonBlockingBuffer.size);

			}
			//continue to the next transaction
			else
			{
				startNonBlockingRead();
			}
			break;

		case PAGE_PROGRAM: //non-blocking write

			//check if whole data been written
			if (gNonBlockingOpSize == gNonBlockingBuffer.size)
			{
				gCurrentCommand = IDLE;
				gpFlashRequestDoneCB();
			}
			//continue to the next transaction
			else
			{
				performNonBlockingWrite();
			}
			break;
		case BLOCK_ERASE:
		case BULK_ERASE:
			gCurrentCommand = IDLE;
			gpFlashRequestDoneCB();
			break;
		default:
			DBG_ASSERT(false);
		
	}


}

result_t flash_init( void (*flash_data_recieve_cb)(uint8_t const *buffer, uint32_t size),
		     void (*flash_request_done_cb)(void))
{
	if (!flash_data_recieve_cb || !flash_request_done_cb)
	{
		return NULL_POINTER;
	}
	else
	{
		gpFlashDataReciveCB = flash_data_recieve_cb;
		gpFlashRequestDoneCB = flash_request_done_cb;
		
		return OPERATION_SUCCESS;
	}
}			 

bool flashIsIdle(void)
{
	FlashStatusRegister cr = {_lr(FLASH_STATUS_REG_ADDR)};

	return cr.bits.SCIP==0;
}

bool flash_is_ready(void)
{

	return (gCurrentCommand==IDLE && flashIsIdle());
}

/**********************************************************************
 *
 * Function:    flash_read_start
 *
 * Descriptor:  Read "size" bytes start from address "start_address".
 *              This function is non-blocking.
 *
 * Notes:       size = # bytes to read - must be > 0
 *
 * Return:      OPERATION_SUCCESS: Read request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_read_start(uint16_t start_address, uint16_t size)
{

	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);
	
	if (size > MAX_REQUEST_BUFFER_SIZE)
	{
		return INVALID_ARGUMENTS;
	}
	
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	
	gCurrentCommand = READ_DATA;
	_enable();
        
        gNonBlockingBuffer.size = 0;
        gNonBlockingOpSize = size;
        gNonBlockingStartAddress = start_address;

        startNonBlockingRead();

        return OPERATION_SUCCESS;
	
}

void loadDataFromRegs(uint8_t buffer[],const uint16_t size)
{

	uint32_t i=0,j,regIndex = 0,regData;

	uint8_t* pRegData = (uint8_t*)&regData;

	while(i < size)
	{
 		DBG_ASSERT(regIndex < NUM_OF_DATA_REG);

		regData = _lr(FLASH_FDATA_BASE_ADDR+regIndex);

		for(j = 0 ; j < 4 && i < size ; ++i,++j)
		{
			buffer[i] = pRegData[j];
		}

		regIndex++;
	}
}

/**********************************************************************
 *
 * Function:    flash_read
 *
 * Descriptor:  Same as flash_read_start except the data received in the supplied
 *              buffer.
 *              This function is blocking.

 * Notes:       size = # bytes to read - must be > 0
 *
 * Return:      OPERATION_SUCCESS: Read request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
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
	
	if (size > MAX_REQUEST_BUFFER_SIZE )
	{
		return  INVALID_ARGUMENTS;
	}
	
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	
	gCurrentCommand = READ_DATA;
	_enable();

	cr.bits.CMD = READ_DATA;

	//no need to set current command, since this is single thread program and this is blocking function
	while(readBytes < size)
	{
		transactionBytes = MIN(size-readBytes,DATA_REG_BYTES);

		cr.bits.FDBC = transactionBytes-1;

		//no need for interrupts since this is blocking function
		cr.bits.SME = 0;

		cr.bits.SCGO = 1;

		_sr(start_address+readBytes,FLASH_ADDRESS_REG_ADDR);
		_sr(cr.data,FLASH_CONTROL_REG_ADDR);

		wait_for_flash_to_be_idle;

		loadDataFromRegs(buffer+readBytes,transactionBytes);

		readBytes+=transactionBytes;

	}

	return OPERATION_SUCCESS;
	
	
	
}

/**********************************************************************
 *
 * Function:    flash_write_start
 *
 * Descriptor:  Write "size" bytes start from address "start_address".
 *              This function is non-blocking.
 *
 * Notes:       size = # bytes to write - must be > 0.
 *              The given buffer can be used immediately when the function returned.
 *
 * Return:      OPERATION_SUCCESS: Write request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_write_start(uint16_t start_address, uint16_t size, const uint8_t buffer[])
{
    uint16_t i;
	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);
	
	if (!buffer)
	{
		return NULL_POINTER;
	}
	
	if (size > MAX_REQUEST_BUFFER_SIZE)
	{
		return  INVALID_ARGUMENTS;
	}
	
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	
	gCurrentCommand = PAGE_PROGRAM;
	_enable();

        for(i = 0 ; i < size ; ++i)
        {
            gNonBlockingBuffer.data[i] = buffer[i];
        }
        gNonBlockingBuffer.size = size;
        gNonBlockingOpSize = 0;
        gNonBlockingStartAddress = start_address;

        performNonBlockingWrite();

        return OPERATION_SUCCESS;
}

/*
 *  load data from
 */
void loadDataToRegs(const uint8_t buffer[],const uint16_t size)
{
	uint16_t i,regIndex;

	for(i = 0 , regIndex = 0 ; i < size ; i+=4 , ++regIndex)
	{
		_sr((*(uint32_t*)(buffer+i)),FLASH_FDATA_BASE_ADDR+regIndex);
	}
}
/**********************************************************************
 *
 * Function:    flash_write
 *
 * Descriptor:  Same as flash_write_start.
 *              This function is blocking.
 *
 * Notes:       size = # bytes to write - must be > 0.
 *              The given buffer can be used immediately when the function returned.
 *
 * Return:      OPERATION_SUCCESS: Write request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
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
	
	if (size > MAX_REQUEST_BUFFER_SIZE)
	{
		return  INVALID_ARGUMENTS;
	}
	
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	gCurrentCommand = PAGE_PROGRAM;
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

		writtenBytes+=transactionBytes;

		wait_for_flash_to_be_idle;

	}

	gCurrentCommand = IDLE;

	return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_bulk_erase_start
 *
 * Descriptor:  Erase all the content that find on the flash device.
 *              This function is non-blocking.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS: The request done successfully.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_bulk_erase_start(void)
{
	FlashControlRegister cr = {0};
	
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	
	gCurrentCommand = BULK_ERASE;
	_enable();
	
	//prepare control reg
	cr.bits.CMD = BULK_ERASE;
	cr.bits.SME = 1;
	cr.bits.SCGO = 1;
	
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);
	
	return OPERATION_SUCCESS;

}


/**********************************************************************
 *
 * Function:    flash_block_erase_start
 *
 * Descriptor:  Erase the content that find on the block that find on address
 *              "start_address".
 *              This function is non-blocking.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS: The request done successfully.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_block_erase_start(uint16_t start_address)
{
	
	DBG_ASSERT(start_address < FLASH_CAPACITY);
	
	FlashControlRegister cr = {0};
	
	_disable();
	if (!flash_is_ready())
	{
		_enable();
		return NOT_READY;
	}
	
	//the ISR should clean this var
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

