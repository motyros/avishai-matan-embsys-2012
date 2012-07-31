/*
 * Flash.h
 *
 * Descriptor:  API of the flash device.
 *
 *
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "./common_defs.h"

// Maximum size of data buffer that can be used.
#define MAX_REQUEST_BUFFER_SIZE 512


/**********************************************************************
 *
 * Function:    flash_init
 *
 * Descriptor:  Initialize the flash device.
 *
 * Parameters:  flash_data_recieve_cb: call back whenever asynchrony read operation done.
 *              flash_request_done_cb: call back whenever asynchrony operation done
 *                                     (besides read).
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Initialization successfully done.
 *              NULL_POINTER:           One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_init( void (*flash_data_recieve_cb)(uint8_t const *buffer, uint32_t size),
		     void (*flash_request_done_cb)(void));


/**********************************************************************
 *
 * Function:    flash_is_ready
 *
 * Descriptor:
 *
 * Notes:
 *
 * Return:      true in case the FW ready to accept a new request.
 *
 ***********************************************************************/
bool flash_is_ready(void);


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
result_t flash_read_start(uint16_t start_address, uint16_t size);


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
result_t flash_read(uint16_t start_address, uint16_t size, uint8_t buffer[]);


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
result_t flash_write_start(uint16_t start_address, uint16_t size, const uint8_t buffer[]);


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
result_t flash_write(uint16_t start_address, uint16_t size, const uint8_t buffer[]);


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
result_t flash_bulk_erase_start(void);


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
result_t flash_block_erase_start(uint16_t start_address);

#endif /* FLASH_H_ */
