#include "network.h"

#define NETWORK_BASE_ADDR 0x200000

//pack the struct with no internal spaces
#pragma pack(1)

typedef struct
{
	desc_t* NTXBP;
	uint32_t NTXBL;
	uint32_t NTXFH;
	uint32_t NTXFT;

	desc_t* NRXBP;
	uint32_t NRXBL;
	uint32_t NRXFH;
	uint32_t NRXFT;

	struct
	{
		uint8_t NBSY							:1;
		uint8_t enableTxInterrupt               :1;
		uint8_t enableRxInterrupt               :1;
		uint8_t enablePacketDroppedInterrupt    :1;
		uint8_t enableTransmitErrorInterrupt    :1;
		uint32_t reserved                       :24;
		uint8_t NOM                             :3;
	}NCTRL;

	struct
	{
		uint8_t NTIP            :1;
		uint8_t reserved1       :1;
		uint8_t NRIP            :1;
		uint8_t NROR            :1;
		uint8_t reserved2       :4;
		union
		{
			uint8_t data		:8;
			struct
			{
				uint8_t RxComplete              :1;
				uint8_t TxComplete              :1;
				uint8_t RxBufferTooSmall        :1;
				uint8_t CircularBufferFull      :1;
				uint8_t TxBadDescriptor         :1;
				uint8_t TxNetworkError          :1;
				uint8_t reserved                :2;
			}bits;
		}NIRE;
		uint16_t reserved       :16;
	}NSTAT;
}volatile NetworkRegs;


#pragma pack()

//cost the NETWORK_BASE_ADDR to the NetworkRegs struct
NetworkRegs* gpNetwork = (NetworkRegs*)(NETWORK_BASE_ADDR);

network_call_back_t gNetworkCallBacks = {0};

//check that all cb pointers valid
bool isValidCallbaks(network_call_back_t cb)
{
	return (cb.dropped_cb && cb.recieved_cb && cb.transmit_error_cb && cb.transmitted_cb);
}

result_t network_init(const network_init_params_t *init_params)
{
	//check that all pointers not null
	if(	!init_params || !init_params->transmit_buffer || !init_params->recieve_buffer ||
		!isValidCallbaks(init_params->list_call_backs))
	{
		return NULL_POINTER;
	}

	//check minimal buffer size
	if(init_params->size_t_buffer<2 || init_params->size_r_buffer<2)
	{
		return INVALID_ARGUMENTS;
	}

	//set transmit parameters
	gNetworkCallBacks = init_params->list_call_backs;
	gpNetwork->NTXBP = init_params->transmit_buffer;
	gpNetwork->NTXBL = init_params->size_t_buffer;
	gpNetwork->NTXFH = 0;
	gpNetwork->NTXFT = 0;

	//set recieve parameters
	gpNetwork->NRXBP = init_params->recieve_buffer;
	gpNetwork->NRXBL = init_params->size_r_buffer;
	gpNetwork->NRXFH = 0;
	gpNetwork->NRXFT = 0;

	//enable interrupts
	gpNetwork->NCTRL.enableTxInterrupt = 1;
	gpNetwork->NCTRL.enableRxInterrupt = 1;
	gpNetwork->NCTRL.enablePacketDroppedInterrupt = 1;
	gpNetwork->NCTRL.enableTransmitErrorInterrupt = 1;
	gpNetwork->NCTRL.NBSY = 0;

	return OPERATION_SUCCESS;
}

result_t network_set_operating_mode(network_operating_mode_t new_mode)
{
	if(new_mode!=NETWORK_OPERATING_MODE_INTERNAL_LOOPBACK &&
			new_mode!=NETWORK_OPERATING_MODE_NORMAL &&
			new_mode!=NETWORK_OPERATING_MODE_SMSC &&
			new_mode!=NETWORK_OPERATING_MODE_RESERVE)
	{
		return INVALID_ARGUMENTS;
	}

	gpNetwork->NCTRL.NOM = new_mode;

	return OPERATION_SUCCESS;
}

result_t network_send_packet_start(const uint8_t buffer[], uint32_t size, uint32_t length)
{

	if(!buffer)
	{
		return NULL_POINTER;
	}

	if(length > NETWORK_MAXIMUM_TRANSMISSION_UNIT || size < length){
		return INVALID_ARGUMENTS;
	}

	if(length == 0)
	{
		return OPERATION_SUCCESS;
	}

	//if the head is 1 cell behind the tail - the buffer is full
	if( (gpNetwork->NTXFH +1)%gpNetwork->NTXBL == gpNetwork->NTXFT )
	{
		return NETWORK_TRANSMIT_BUFFER_FULL;
	}

	//set next packet to send at the buffer
	desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFH;

	pPacket->pBuffer = (uint32_t)buffer;
	pPacket->buff_size = size;
	pPacket->reserved = length;

	//advance head pointer
	gpNetwork->NTXFH = (gpNetwork->NTXFH+1)%(gpNetwork->NTXBL);

	return OPERATION_SUCCESS;
}


void networkISR()
{
	//Important note: the order of those events important since in Loopback mode
	//if we handle Rx before Tx, Rx will be asserted again, need to Ack Tx
	//before Ack Rx
	if(gpNetwork->NSTAT.NIRE.bits.TxComplete)
	{
		//locate the transmitted packet
		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFT;

		//advance tail cyclically by one
		gpNetwork->NTXFT = (gpNetwork->NTXFT+1)%gpNetwork->NTXBL;

		//Acknowledging the interrupt
		gpNetwork->NSTAT.NIRE.bits.TxComplete = 1;

		//call cb with the transmitted packet
		gNetworkCallBacks.transmitted_cb((uint8_t*)pPacket->pBuffer,pPacket->buff_size);

	}
	else if(gpNetwork->NSTAT.NIRE.bits.RxComplete)
	{
		//locate the received packet
		desc_t* pPacket = gpNetwork->NRXBP+gpNetwork->NRXFT;

		//increase the head pointer
		gpNetwork->NRXFT = (gpNetwork->NRXFT + 1)%gpNetwork->NRXBL;

		//Acknowledging the interrupt
		gpNetwork->NSTAT.NIRE.bits.RxComplete = 1;

        //call cb with the received packet
        gNetworkCallBacks.recieved_cb((uint8_t*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFF);


	}
	else if(gpNetwork->NSTAT.NIRE.bits.RxBufferTooSmall)
	{
		//Ack the interrupt
		gpNetwork->NSTAT.NIRE.bits.RxBufferTooSmall = 1;
		//call the cb
		gNetworkCallBacks.dropped_cb(RX_BUFFER_TOO_SMALL);

	}
	else if(gpNetwork->NSTAT.NIRE.bits.CircularBufferFull)
	{
		//Ack the interrupt
		gpNetwork->NSTAT.NIRE.bits.CircularBufferFull = 1;
		//call the cb
		gNetworkCallBacks.dropped_cb(CIRCULAR_BUFFER_FULL);
	}
	else if(gpNetwork->NSTAT.NIRE.bits.TxBadDescriptor)
	{
		//locate the bad packet
		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFT;

		//advance tail cyclically by one
		gpNetwork->NTXFT = (gpNetwork->NTXFT+1)%gpNetwork->NTXBL;

		//Ack the interrupt
		gpNetwork->NSTAT.NIRE.bits.TxBadDescriptor = 1;

		//call cb with the packet
		gNetworkCallBacks.transmit_error_cb(BAD_DESCRIPTOR,(uint8_t*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);
	}
	else if(gpNetwork->NSTAT.NIRE.bits.TxNetworkError)
	{
		//locate the packet
		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFT;

		//advance tail cyclically by one
		gpNetwork->NTXFT = (gpNetwork->NTXFT+1)%gpNetwork->NTXBL;

		//Ack the interrupt
		gpNetwork->NSTAT.NIRE.bits.TxNetworkError = 1;

		//call cb with the packet
		gNetworkCallBacks.transmit_error_cb(NETWORK_ERROR,(uint8_t*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);
	}
	else
	{
		DBG_ASSERT(false);
	}
}

