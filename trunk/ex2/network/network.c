#include "network.h"

#define NETWORK_BASE_ADDR 0x200000
#define BUFFER_EMPTY(b) gpNetwork->N##b##XFH==gpNetwork->N##b##XFT
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
			union
			{
				uint8_t data                        :3;
				struct
				{
					uint8_t normal                  :1;
					uint8_t SmscConnectivity    	:1;
					uint8_t internalLoopback        :1;
				}bits;
			}NOM;
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

/*
typedef struct
	{
		uint8_t NBSY							:1;
		uint8_t enableTxInterrupt               :1;
		uint8_t enableRxInterrupt               :1;
		uint8_t enablePacketDroppedInterrupt    :1;
		uint8_t enableTransmitErrorInterrupt    :1;
		uint32_t reserved                       :24;
		union
		{
			uint8_t data                        :3;
			struct
			{
				uint8_t normal                  :1;
				uint8_t SmscConnectivity    	:1;
				uint8_t internalLoopback        :1;
			}bits;
		}NOM;
	}NCTRLReg;

	typedef struct
		{
		uint32_t NTIP            :1;
		uint32_t reserved1       :1;
		uint32_t NRIP            :1;
		uint32_t NROR            :1;
		uint32_t reserved2       :4;
			union
			{
			uint32_t data		:8;
				struct
				{
				uint32_t RxComplete              :1;
				uint32_t TxComplete              :1;
				uint32_t RxBufferTooSmall        :1;
				uint32_t CircularBufferFull      :1;
				uint32_t TxBadDescriptor         :1;
				uint32_t TxNetworkError          :1;
				uint32_t reserved                :2;
				}bits;
			}NIRE;
			uint32_t reserved       :16;
		}NSTATReg;

		typedef struct
				{
				uint32_t NTIP            :1;
				uint32_t reserved1       :1;
				uint32_t NRIP            :1;
				uint32_t NROR            :1;
				uint32_t reserved2       :4;
					union
					{
					uint32_t data		:8;
						struct
						{
						uint32_t RxComplete              :1;
						uint32_t TxComplete              :1;
						uint32_t RxBufferTooSmall        :1;
						uint32_t CircularBufferFulll     :1;
						uint32_t TxBadDescriptor         :1;
						uint32_t TxNetworkError          :1;
						uint32_t reserved                :2;
						}bits;
					}NIRE;
					uint32_t reserved       :16;
				}TestStruct;


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

	NCTRLReg NCTRL;

	NSTATReg NSTAT;


}volatile NetworkRegs;



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

}volatile NetworkRegs1;

NetworkRegsOld gTest;
*/

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

	union
	{
		uint32_t data;
		struct
		{
			uint8_t NBSY							:1;
			uint8_t enableTxInterrupt               :1;
			uint8_t enableRxInterrupt               :1;
			uint8_t enablePacketDroppedInterrupt    :1;
			uint8_t enableTransmitErrorInterrupt    :1;
			uint32_t reserved                       :24;
			union
			{
				uint8_t data						:3;
				struct
				{
					uint8_t normal                  :1;
					uint8_t SmscConnectivity    	:1;
					uint8_t internalLoopback        :1;
				}bits;
			}NOM;
		}bits;
	}NCTRL;



	union
	{
		uint32_t data;
		struct
		{
			uint8_t NTIP            :1;
			uint8_t reserved1       :1;
			uint8_t NRIP            :1;
			uint8_t NROR            :1;
			uint8_t reserved2       :4;
			union
			{
				uint8_t data;
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
		}bits;

	}NSTAT;


}volatile NetworkRegsNew;
#pragma pack()


void test()
{
	uint32_t a;
	a = sizeof(NetworkRegsNew);
	a = a * 10;
//	//NetworkRegs tt;
//	a = sizeof(NetworkRegs1);
//
//	b = sizeof(NCTRLReg);
//	c = sizeof(NSTATReg);
//	d = sizeof(NetworkRegsOld);
//	d = a+b+c;
}

//cost the NETWORK_BASE_ADDR to the NetworkRegs struct
NetworkRegs* gpNetwork = (NetworkRegs*)(NETWORK_BASE_ADDR);

network_call_back_t gNetworkCallBacks;

//uint32_t gNetworkData[NETWORK_MAXIMUM_TRANSMISSION_UNIT];


result_t network_init(const network_init_params_t *init_params)
{
	test();
	if(!init_params || !init_params->transmit_buffer || !init_params->recieve_buffer)
	{
		return NULL_POINTER;
	}

	if(init_params->size_t_buffer<2 || init_params->size_r_buffer<2)
	{
		return INVALID_ARGUMENTS;
	}

	//                      gNetworkParams = *init_params;
	gNetworkCallBacks = init_params->list_call_backs;
	gpNetwork->NTXBP = init_params->transmit_buffer;
	gpNetwork->NTXBL = init_params->size_t_buffer;
	gpNetwork->NTXFH = 0;
	gpNetwork->NTXFT = 0;

	gpNetwork->NRXBP = init_params->recieve_buffer;
	gpNetwork->NRXBL = init_params->size_r_buffer;
	gpNetwork->NRXFH = 0;
	gpNetwork->NRXFT = 0;

	gpNetwork->NCTRL.enableTxInterrupt = 1;
	gpNetwork->NCTRL.enableRxInterrupt = 1;
	gpNetwork->NCTRL.enablePacketDroppedInterrupt = 1;
	gpNetwork->NCTRL.enableTransmitErrorInterrupt = 1;
	gpNetwork->NCTRL.NBSY = 0;

	return OPERATION_SUCCESS;
}

result_t network_set_operating_mode(network_operating_mode_t new_mode)
{
	//if()
	//return INVALID_ARGUMENTS;

	gpNetwork->NCTRL.NOM.data = (new_mode);

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


	//desc_t* headOffset = (desc_t*)((uint32_t)gpNetwork->NTXFH - (uint32_t)gpNetwork->NTXBP);
	//desc_t* tailOffset  = (desc_t*)((uint32_t)gpNetwork->NTXFT - (uint32_t)gpNetwork->NTXBP);

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

desc_t* getLastDesc(uint32_t tail, desc_t* bufferBase, uint32_t bufferSize)
{
	uint32_t tailOffset = (tail+bufferSize-1)%bufferSize;
	return gpNetwork->NRXBP+tailOffset;
	//desc_t* tailOffset = (desc_t*)(tail-bufferBase);
	//return  (desc_t*)((uint32_t)(tailOffset + bufferSize -1)%bufferSize);
}

_Interrupt1 void networkISR()
{
	if(gpNetwork->NSTAT.NIRE.bits.RxComplete)
	{
		//Acknowledging the interrupt
		gpNetwork->NSTAT.NIRE.bits.RxComplete = 1;

		//desc_t* tailOffset  = (desc_t*)((uint32_t)gpNetwork->NRXFT - (uint32_t)gpNetwork->NRXBP);
		//desc_t* lastDesc = (desc_t*)( ((uint32_t)(tailOffset + gpNetwork->NRXBL - 1))%gpNetwork->NRXBL);
		//desc_t* lastDesc = getLastDesc(gpNetwork->NRXFT,gpNetwork->NRXBP,gpNetwork->NRXBL);
		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFH;

		//increase the head pointer
		gpNetwork->NRXFH = (gpNetwork->NRXFH + 1)%gpNetwork->NRXBL;

		//call cb
		gNetworkCallBacks.recieved_cb((uint8_t*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);

	}
	else if(gpNetwork->NSTAT.NIRE.bits.TxComplete)
	{
		//Acknowledging the interrupt
		gpNetwork->NSTAT.NIRE.bits.TxComplete = 1;

		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFT;

		//advance tail cyclically by one
		gpNetwork->NTXFT = (gpNetwork->NTXFT+1)%gpNetwork->NTXBL;

		//call cb
		gNetworkCallBacks.transmitted_cb((uint8_t*)pPacket->pBuffer,pPacket->buff_size);

	}
	else if(gpNetwork->NSTAT.NIRE.bits.RxBufferTooSmall)
	{
		gpNetwork->NSTAT.NIRE.bits.RxBufferTooSmall = 1;
		gNetworkCallBacks.dropped_cb(RX_BUFFER_TOO_SMALL);

	}
	else if(gpNetwork->NSTAT.NIRE.bits.CircularBufferFull)
	{
		gpNetwork->NSTAT.NIRE.bits.CircularBufferFull = 1;
		gNetworkCallBacks.dropped_cb(CIRCULAR_BUFFER_FULL);
	}
	else if(gpNetwork->NSTAT.NIRE.bits.TxBadDescriptor)
	{
		gpNetwork->NSTAT.NIRE.bits.TxBadDescriptor = 1;
		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFT;

		//advance tail cyclically by one
		gpNetwork->NTXFT = (gpNetwork->NTXFT+1)%gpNetwork->NTXBL;

		//call cb
		gNetworkCallBacks.transmit_error_cb(BAD_DESCRIPTOR,(uint8_t*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);
	}
	else if(gpNetwork->NSTAT.NIRE.bits.TxNetworkError)
	{
		gpNetwork->NSTAT.NIRE.bits.TxNetworkError = 1;
		desc_t* pPacket = gpNetwork->NTXBP+gpNetwork->NTXFT;

		//call cb
		gNetworkCallBacks.transmit_error_cb(NETWORK_ERROR,(uint8_t*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);
	}
	else
	{
		DBG_ASSERT(false);
	}
}

