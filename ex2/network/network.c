#include "network.h"

#define NETWORK_BASE_ADDR 0x200000
#define BUFFER_EMPTY(b) gpNetwork->N##b##XFH==gpNetwork->N##b##XFT
//pack the struct with no internal spaces
#pragma pack(1)
typedef struct
		{
	desc_t* NTXBP;
	uint32_t NTXBL;
	desc_t* NTXFH;
	desc_t* NTXFT;

	desc_t* NRXBP;
	uint32_t NRXBL;
	desc_t* NRXFH;
	desc_t* NRXFT;

	struct
	{
		//		uint32_t data;
		//		struct
		//		{
		uint8_t NBSY							:1;
		uint8_t enableTxInterrupt				:1;
		uint8_t enableRxInterrupt				:1;
		uint8_t enablePacketDroppedInterrupt	:1;
		uint8_t enableTransmitErrorInterrupt	:1;
		uint32_t reserved						:24;
		union
		{
			uint8_t data						:3;
			struct
			{
				uint8_t normal      		:1;
				uint8_t SmscConnectivity    :1;
				uint8_t internalLoopback   	:1;
			}bits;
		}NOM;
		//		}
	}NCTRL;

	struct
	{
		//		uint32_t data;
		//		struct
		//		{
		uint8_t NTIP		:1;
		uint8_t reserved1	:1;
		uint8_t NRIP		:1;
		uint8_t NROR		:1;
		uint8_t reserved2	:4;
		union
		{
			uint8_t data;
			struct
			{
				uint8_t RxComplete      	:1;
				uint8_t TxComplete      	:1;
				uint8_t RxBufferTooSmall   	:1;
				uint8_t CircularBufferFulll	:1;
				uint8_t TxBadDescriptor  	:1;
				uint8_t TxNetworkError     	:1;
				uint8_t reserved           	:2;
			}bits;
		}NIRE;
		uint16_t reserved	:16;
		//		}
	}NSTAT;
		} volatile NetworkRegs;
#pragma pack();

		//cost the NETWORK_BASE_ADDR to the NetworkRegs struct
		NetworkRegs* gpNetwork = (NetworkRegs*)(NETWORK_BASE_ADDR);

		network_call_back_t gNetworkCallBacks;
		uint32_t gNetworkData[NETWORK_MAXIMUM_TRANSMISSION_UNIT];
		network_operating_mode_t gNetworkOperMode;
		network_init_params_t gNetworkParams;

		result_t network_init(const network_init_params_t *init_params)
		{
			if(!init_params || !init_params->transmit_buffer || !init_params->recieve_buffer)
			{
				return NULL_POINTER;
			}

			if(init_params->size_t_buffer<2 || init_params->size_r_buffer<2)
			{
				return INVALID_ARGUMENTS;
			}

			gNetworkParams = *init_params;
			gNetworkCallBacks = init_params->list_call_backs;
			gpNetwork->NTXBP = init_params->transmit_buffer;
			gpNetwork->NTXBL = init_params->size_t_buffer;
			gpNetwork->NTXFH = gpNetwork->NTXBP;
			gpNetwork->NTXFT = gpNetwork->NTXBP;

			gpNetwork->NRXBP = init_params->recieve_buffer;
			gpNetwork->NRXBL = init_params->size_r_buffer;
			gpNetwork->NRXFH = gpNetwork->NRXBP;
			gpNetwork->NRXFT = gpNetwork->NRXBP;

			gpNetwork->NCTRL.enableTxInterrupt = 1;
			gpNetwork->NCTRL.enableRxInterrupt = 1;
			gpNetwork->NCTRL.enablePacketDroppedInterrupt = 1;
			gpNetwork->NCTRL.enableTransmitErrorInterrupt = 1;
			gpNetwork->NCTRL.NBSY = 0;
			//			gpNetwork->NCTRL.NOM.data = NETWORK_OPERATING_MODE_NORMAL;
			return OPERATION_SUCCESS;
		}

		result_t network_set_operating_mode(network_operating_mode_t new_mode)
		{
			//      if()
			//      return INVALID_ARGUMENTS;

			gNetworkOperMode = new_mode;
			gpNetwork->NCTRL.NOM.data = (new_mode);
			return OPERATION_SUCCESS;
		}

		result_t network_send_packet_start(const uint8_t buffer[], uint32_t size, uint32_t length)
		{
			DBG_ASSERT(length<0);

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


			desc_t* headOffset = (desc_t*)((uint32_t)gpNetwork->NTXFH - (uint32_t)gpNetwork->NTXBP);
			desc_t* tailOffset  = (desc_t*)((uint32_t)gpNetwork->NTXFT - (uint32_t)gpNetwork->NTXBP);
			if( ((uint32_t)(headOffset + 1))%gpNetwork->NTXBL == (uint32_t)tailOffset)
			{
				return NETWORK_TRANSMIT_BUFFER_FULL;
			}

			gpNetwork->NTXFH->pBuffer = (uint32_t)buffer;

			//if Network Transmit is not in Progress then activate the transmit.
			if(!gpNetwork-> NSTAT.NTIP)
			{
				gpNetwork->NTXFH++;
			}


			return OPERATION_SUCCESS;
		}

		_Interrupt1 void networkISR()
		{
			if(gpNetwork->NSTAT.NIRE.bits.RxComplete)
			{
				//Acknowledging the interrupt
				gpNetwork->NSTAT.NIRE.bits.RxComplete = 0;

				desc_t* tailOffset  = (desc_t*)((uint32_t)gpNetwork->NRXFT - (uint32_t)gpNetwork->NRXBP);
				//uint32_t lastDesc1 = (uint32_t)(tailOffset + gpNetwork->NRXBL - 1)%gpNetwork->NRXBL;
				desc_t* lastDesc = (desc_t*)( ((uint32_t)(tailOffset + gpNetwork->NRXBL - 1))%gpNetwork->NRXBL);

				gNetworkCallBacks.recieved_cb((uint8_t*)lastDesc->pBuffer,lastDesc->buff_size,lastDesc->reserved);

			}
			else if(gpNetwork->NSTAT.NIRE.bits.TxComplete)

			{
				//Acknowledging the interrupt
				gpNetwork->NSTAT.NIRE.bits.TxComplete = 0;

				desc_t* tailOffset  = (desc_t*)((uint32_t)gpNetwork->NTXFT - (uint32_t)gpNetwork->NTXBP);
				desc_t* lastDesc = (desc_t*)((uint32_t)(tailOffset + gpNetwork->NTXBL - 1)%gpNetwork->NTXBL);

				gNetworkCallBacks.transmitted_cb((uint8_t*)lastDesc->pBuffer,lastDesc->buff_size);

				//?????????????????????//
				//if Network Transmit is not in Progress, and the Transmit buffer is not empty
				//then activate the next transmit.
				if(!gpNetwork-> NSTAT.NTIP && gpNetwork->NTXFH!=gpNetwork->NTXFT)
				{
					gpNetwork->NTXFH++;
				}
				//?????????????????????//

			}
			else if(gpNetwork->NSTAT.NIRE.bits.RxBufferTooSmall)
			{
				gpNetwork->NSTAT.NROR = 0;
				gNetworkCallBacks.dropped_cb(RX_BUFFER_TOO_SMALL);

			}
			else if(gpNetwork->NSTAT.NIRE.bits.CircularBufferFulll)
			{
				gpNetwork->NSTAT.NROR = 0;
				gNetworkCallBacks.dropped_cb(CIRCULAR_BUFFER_FULL);

			}
			else if(gpNetwork->NSTAT.NIRE.bits.TxBadDescriptor)
			{
				gNetworkCallBacks.transmit_error_cb();

			}
			else if(gpNetwork->NSTAT.NIRE.bits.TxNetworkError)
			{
				gNetworkCallBacks.transmit_error_cb();

			}
			else
			{
				DBG_ASSERT(false);
			}
		}
