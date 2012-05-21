#include "flash.h"
#include "common_defs.h"


#define FLASH_CAPACITY (64*1024)
#define FLASH_BLOCK_SIZE (4*1024)

#define NUM_OF_DATA_REG 16
#define FLASH_BASE_ADDR 0x150



#define FLASH_STATUS_REG_ADDR (FLASH_BASE_ADDR+0x0)
#define FLASH_CONTROL_REG_ADDR (FLASH_BASE_ADDR+0x1)
#define FLASH_ADDRESS_REG_ADDR (FLASH_BASE_ADDR+0x2)
#define FLASH_FDATA_BASE_ADDR (FLASH_BASE_ADDR+0x3)

typedef struct
{
	uint32_t size;
	uint8_t data[MAX_REQUEST_BUFFER_SIZE];
}FlashBuffer;

FlashBuffer gReadBuffer;
FlashBuffer gWriteBuffer;

union
{
	uint32_t data;
	struct
	{
		uint32_t SCIP				:1;
		const uint32_t cycleDone	:1;
		const uint32_t reserved		:30;
	}bits;
}FlashStatusRegister;

union
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

typedef enum
{
	IDLE			=	0x00,
	READ_DATA 		= 	0x03,
	PAGE_PROGRAM 	= 	0x05,
	BLOCK_ERASE 	= 	0xD8,
	BULK_ERASE 		= 	0xC7,
}SPICommand


void (*gpFlashDataReciveCB)(uint8_t const *,uint32_t);
void (*gpFlashRequestDoneCB)(void);

SPICommand gCurrentCommand;

uint32_t gFDATA[NUM_OF_DATA_REG];


_Interrupt1 flashISR()
{
	//acknowledge the interrupt
	
	//handle the interrupt
	swith (gCurrentCommand)
	{
		case READ_DATA:
			break;
		case PAGE_PROGRAM:
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
		
		gFlashInUse = false;
		//???
		gReadBuffer.size = 0;
		gWriteBuffer.size = 0;
		
		return OPERATION_SUCCESS;
	}
}			 

bool flash_is_ready(void)
{
	FlashControlRegister cr = _lr(FLASH_STATUS_REG_ADDR);
	return (gCurrentCommand==IDLE && cr.bits.SCIP==0);
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

	FlashControlRegister cr;
	
	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);
	
	if (size > MAX_REQUEST_BUFFER_SIZE)
	{
		return INVALID_ARGUMENTS;
	}
	
	if (!flash_is_ready())
	{
		return NOT_READY;
	}
	
	gCurrentCommand = READ_DATA;
	
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
	
	DBG_ASSERT(start_address + size*8 >= FLASH_CAPACITY);
	
	if (!buffer)
	{
		return NULL_POINTER;
	}
	
	if (size > MAX_REQUEST_BUFFER_SIZE )
	{
		return  INVALID_ARGUMENTS;
	}
	
	if (!flash_is_ready())
	{
		return NOT_READY;
	}
	
	//no need to set current command, since this is single thread program and this is blocking function
	
	
	
	
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
	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);
	
	if (!buffer)
	{
		return NULL_POINTER;
	}
	
	if (size > MAX_REQUEST_BUFFER_SIZE)
	{
		return  INVALID_ARGUMENTS;
	}
	
	if (!flash_is_ready())
	{
		return NOT_READY;
	}
	
	gCurrentCommand = PAGE_PROGRAM;
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
	DBG_ASSERT(start_address + size*8 < FLASH_CAPACITY);
	
	if (!buffer)
	{
		return NULL_POINTER;
	}
	
	if (size > MAX_REQUEST_BUFFER_SIZE)
	{
		return  INVALID_ARGUMENTS;
	}
	
	if (!flash_is_ready())
	{
		return NOT_READY;
	}
	
	//no need to set current command, since this is single thread program and this is blocking function
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
	
	if (!flash_is_ready())
	{
		return NOT_READY;
	}
	
	gCurrentCommand = BULK_ERASE;
	
	cr.CMD = BULK_ERASE;
	cr.SME = 1;
	cr.SCGO = 1;
	
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
	
	if (!flash_is_ready())
	{
		return NOT_READY;
	}
	
	gCurrentCommand = BLOCK_ERASE;
	
	cr.CMD = BLOCK_ERASE;
	cr.SME = 1;
	cr.SCGO = 1;

	//set the address
	_sr(start_address,FLASH_ADDRESS_REG_ADDR);
		
	_sr(cr.data,FLASH_CONTROL_REG_ADDR);
	
	return OPERATION_SUCCESS;
}

