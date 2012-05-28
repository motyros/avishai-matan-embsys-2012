/*
 * network.h
 *
 *  Descriptor: API of the network device.
 *
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "../common_defs.h"

// The maximum unit's size that can be transmitted over the network (send & receive)
#define NETWORK_MAXIMUM_TRANSMISSION_UNIT 161

#pragma pack(1)
typedef struct
{
	uint32_t        pBuffer;
	uint8_t         buff_size;
	uint16_t        reserved;

} desc_t;
#pragma pack()

typedef enum
{
    NETWORK_OPERATING_MODE_RESERVE              = 0 ,
    NETWORK_OPERATING_MODE_NORMAL               = (0x1) << 0,
    NETWORK_OPERATING_MODE_SMSC                 = (0x1) << 1,
    NETWORK_OPERATING_MODE_INTERNAL_LOOPBACK    = (0x1) << 2,

} network_operating_mode_t;
// reasons for packet drop during receiving
typedef enum
{
    RX_BUFFER_TOO_SMALL ,
    CIRCULAR_BUFFER_FULL

} packet_dropped_reason_t;

// reasons for packet drop during transmission
typedef enum
{
    BAD_DESCRIPTOR ,
    NETWORK_ERROR

} transmit_error_reason_t;


//	------------------------------------------------------------------------------
//				Device call back
//
//      in case a pointer is not set to 0 (NULL) - the matching interrupt will be
//      enabled, therefore a buffer must be supplied.
//	------------------------------------------------------------------------------

/*
 * call back when a packet was transmitted, the buffer and it's size
 * are the same values that were given during the transmit request in order to allow
 * the host to manage the memory
 */
typedef void (*network_packet_transmitted_cb)(const uint8_t *buffer, uint32_t size);

/*
 * call back when a packet was received.
 */
typedef  void (*network_packet_received_cb)(uint8_t buffer[], uint32_t size, uint32_t length);

/*
 * call back when a packet was dropped during receiving.
 */
typedef  void (*network_packet_dropped_cb)(packet_dropped_reason_t);

/*
 * call back when a packet was dropped during transmission.
 */
typedef  void (*network_transmit_error_cb)(transmit_error_reason_t,
                                           uint8_t *buffer,
                                           uint32_t size,
                                           uint32_t length );


typedef struct
{
	network_packet_transmitted_cb	transmitted_cb;
	network_packet_received_cb		recieved_cb;
	network_packet_dropped_cb		dropped_cb;
	network_transmit_error_cb		transmit_error_cb;

} network_call_back_t;

//	----------------------------------------------------------------------------------->


// Initialization
typedef struct
{
	desc_t *transmit_buffer;              	/*< if no transmit buffer is supplied - this
	                                            pointer must be set to 0 (NULL) >*/

	uint32_t size_t_buffer;          	// the size of transmit_buffer

	desc_t *recieve_buffer;         	/*< contain the list of buffers and sizes
						    that should be place in receive buffer
						    the number of entries in this array must
						    be as the size of recieve_buffer 	>*/

	uint32_t size_r_buffer;                 // the size of recieve_buffer


	network_call_back_t list_call_backs;

} network_init_params_t;



/**********************************************************************
 *
 * Function:    network_init
 *
 * Descriptor:  Initialize the network device according to the given parameters.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Initialization done successfully.
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *              NULL_POINTER:           One of the arguments points to NULL,
 *                                      (this return value should be returned only in case
 *                                      a given call back != NULL and the supplied buffer
 *                                      points to NULL ).
 *
 ***********************************************************************/
result_t network_init(const network_init_params_t *init_params);


/**********************************************************************
 *
 * Function:    network_set_operating_mode
 *
 * Descriptor:  Set the operating mode of the device to new_mode
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Setting the new mode done successfully.
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *
 ***********************************************************************/
result_t network_set_operating_mode(network_operating_mode_t new_mode);


/**********************************************************************
 *
 * Function:    network_send_packet_start
 *
 * Descriptor:  Send "length" bytes from the buffer "buffer" whose size is
 *              "size" over the network.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      The request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL
 *              INVALID_ARGUMENTS:      One of the arguments is invalid.
 *              NETWORK_TRANSMIT_BUFFER_FULL: There is no available descriptor for this request
 *
 ***********************************************************************/
result_t network_send_packet_start(const uint8_t buffer[], uint32_t size, uint32_t length);


#endif /* NETWORK_H_ */
