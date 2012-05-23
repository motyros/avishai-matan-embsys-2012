#include "network.h"

uint32_t gNetworkData[NETWORK_MAXIMUM_TRANSMISSION_UNIT];
network_operating_mode_t gNetworkOperMode;
network_init_params_t gNetworkParams;

result_t network_init(const network_init_params_t *init_params)
{
	if(!init_params)
	{
		return NULL_POINTER;
	}
//	if()
//	return INVALID_ARGUMENTS;

	gNetworkParams = *init_params;
	return OPERATION_SUCCESS;
}

result_t network_set_operating_mode(network_operating_mode_t new_mode)
{
	//	if()
	//	return INVALID_ARGUMENTS;

	gNetworkOperMode = new_mode;
	return OPERATION_SUCCESS;
}

result_t network_send_packet_start(const uint8_t buffer[], uint32_t size, uint32_t length)
{
	if(!buffer)
	{
		return NULL_POINTER;
	}

	if(length < 0 || length > NETWORK_MAXIMUM_TRANSMISSION_UNIT || size < length){
		return INVALID_ARGUMENTS;
	}

	if(length == 0)
	{
		return OPERATION_SUCCESS;
	}


//	if()
//	return NETWORK_TRANSMIT_BUFFER_FULL;

	return OPERATION_SUCCESS;
}
