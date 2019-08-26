
#include "CoapServer.hpp"
#include "SupportAPI.hpp"

using ot::Encoding::BigEndian::HostSwap16;

#if OPENTHREAD_FTD

CoapServer::CoapServer(otInstance *pInstance)
{
	#if ALL_LOGS_DEBUG
	otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_API, "CoapServer Constructor\r\n");
	#endif
	m_Instance = pInstance;
	m_ServerURI[0]='\0';
	m_Port = OT_DEFAULT_COAP_PORT;
	memset(&mResource, 0x00, sizeof(mResource));
	m_sequence = 0;
}

CoapServer::~CoapServer()
{
	#if ALL_LOGS_DEBUG
	otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_API, "CoapServer Destructor\r\n");
	#endif
}

otError CoapServer::startCoapServer(unsigned int iPortTemp)
{
	otError error = OT_ERROR_NONE;
	m_Port = iPortTemp;

	// Coap start API with an instance and local UDP port number to bind.
	error = otCoapStart(m_Instance, m_Port);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Error in otCoapStart %d\r\n",error);
	}
	#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Success in otCoapStart %d \n\r", error);	
	}
	#endif

	return error;
}

otError CoapServer::stopCoapServer(void)
{
	otError error = OT_ERROR_NONE;

	// Coap Stop API with an instance.
	error = otCoapStop(m_Instance);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Error in otCoapStop %d\r\n",error);
	}
	#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Success in otCoapStop %d \n\r", error);	
	}
	#endif

	return error;
}

void CoapServer::serverRegisterHandler(void *pContext, otMessage *pMessage,
		const otMessageInfo *pMessageInfo)
{
	#if ALL_LOGS_DEBUG
	otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,"In serverRegisterHandler\n\r");
	#endif
	
	// Since static function cannot call non-static functions, we are static casting it.
	// Using static_cast of pContext pointer to CoapServer*
	static_cast<CoapServer *>(pContext)->serverResponseHandler(pMessage, pMessageInfo);

	OT_UNUSED_VARIABLE(pContext);
	OT_UNUSED_VARIABLE(pMessage);
	OT_UNUSED_VARIABLE(pMessageInfo);
}

void CoapServer::serverResponseHandler(otMessage *pMessage, const otMessageInfo *pMessageInfo)
{
	otError error = OT_ERROR_NONE;
	otMessage *pResponseMessage = NULL;
	otCoapCode   responseCode = OT_COAP_CODE_EMPTY;
	otCoapType   coapType               = otCoapMessageGetType(pMessage);
	otCoapCode   coapCode               = otCoapMessageGetCode(pMessage);
	
	struct timeval  tv1;
	uint64_t sendData[6] = {0};
	uint64_t rxPayloadData[4] = {0};
	GeneratePayload(NULL, 0, &tv1, false);
	uint16_t payload_size = 0;
	
	char operation[7] = {'\0'};  
	switch (coapCode)
	{
		case OT_COAP_CODE_GET:
			strcpy(operation, "get");
			break;

		case OT_COAP_CODE_DELETE:
			strcpy(operation, "delete");
			break;

		case OT_COAP_CODE_PUT:
			strcpy(operation, "put");
			break;

		case OT_COAP_CODE_POST:
			strcpy(operation, "post");
			break;

		default:
			ExitNow(error = OT_ERROR_PARSE);
	}   

	GetCoapPayload(pMessage, rxPayloadData, 4, payload_size);
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"S_rxCoapReq,op=%s,sec=%ld,%ld,PLSize=%d,PL=%lld,%lld,%lld,%lld,,,from=[%x:%x:%x:%x:%x:%x:%x:%x]\n\r", 
			operation, tv1.tv_sec, tv1.tv_usec, payload_size, 
			rxPayloadData[0], rxPayloadData[1], rxPayloadData[2],rxPayloadData[3],
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[0]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[1]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[2]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[3]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[4]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[5]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[6]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[7]));    
	}
	
	if ((coapType == OT_COAP_TYPE_CONFIRMABLE) ||
		(coapCode == OT_COAP_CODE_GET))
		{
		if (coapCode == OT_COAP_CODE_GET)
		{
			responseCode = OT_COAP_CODE_CONTENT;
		}
		else
		{
			responseCode = OT_COAP_CODE_VALID;
		}

		pResponseMessage = otCoapNewMessage(m_Instance, NULL);
		VerifyOrExit(pResponseMessage != NULL, error = OT_ERROR_NO_BUFS);

		otCoapMessageInit(pResponseMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode);
		otCoapMessageSetMessageId(pResponseMessage, otCoapMessageGetMessageId(pMessage));
		SuccessOrExit(error = otCoapMessageSetToken(pResponseMessage, otCoapMessageGetToken(pMessage),
													otCoapMessageGetTokenLength(pMessage)));

		if (otCoapMessageGetCode(pMessage) == OT_COAP_CODE_GET)
		{
			otCoapMessageSetPayloadMarker(pResponseMessage);
			sendData[0] = rxPayloadData[0];
			sendData[1] = rxPayloadData[1];
			sendData[2] = rxPayloadData[2];
			 
			GeneratePayload(&sendData[3], 3, &tv1, true);
			if(6 < (payload_size/(sizeof(uint64_t))))
			{
				uint64_t sendResponsePayloadSize[payload_size/sizeof(uint64_t)] = {0};
				sendResponsePayloadSize[0] = sendData[0];
				sendResponsePayloadSize[1] = sendData[1];
				sendResponsePayloadSize[2] = sendData[2];
				sendResponsePayloadSize[3] = sendData[3];
				sendResponsePayloadSize[4] = sendData[4];
				sendResponsePayloadSize[5] = sendData[5];
				SuccessOrExit(error = otMessageAppend(pResponseMessage, 
									sendResponsePayloadSize, 
									sizeof(sendResponsePayloadSize)));
			}
			else
			{
				SuccessOrExit(error = otMessageAppend(pResponseMessage, sendData, sizeof(sendData)));		
			}
		}

		SuccessOrExit(error = otCoapSendResponse(m_Instance, pResponseMessage, pMessageInfo));
	}    
exit:

	if (error != OT_ERROR_NONE)
	{
		if (pResponseMessage != NULL)
		{
			otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API, "coap send response error %d: %s\r\n", error,
											   otThreadErrorToString(error));
			otMessageFree(pResponseMessage);
		}
	}
	else if (responseCode >= OT_COAP_CODE_RESPONSE_MIN)
	{
		#if ALL_LOGS_DEBUG
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API, "coap response sent\r\n");
		#endif
	}
}


otError CoapServer::addResource(const char* pResource)
{
	strcpy(m_ServerURI,pResource);


	// Setting Parameters for Resource object.
	// setting UriPath of resource
	mResource.mUriPath=m_ServerURI;

	// setting application context
	mResource.mContext=this;

	// setting server handler
	mResource.mHandler=&CoapServer::serverRegisterHandler;

	otError error = OT_ERROR_NONE;

	// This API  add a resource to the Coap Server
	error = otCoapAddResource(m_Instance, &mResource);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Error in otCoapAddResource %d\r\n",error);
	}
	#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Success in otCoapAddResource %d \n\r", error);	
		otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_API, "Resource name: %s\n",m_ServerURI);
	}
	#endif

	return error;
}


void CoapServer::GeneratePayload(uint64_t* sendData, uint16_t arraySize, struct timeval* timeVal, bool pack)
{
	struct timeval  tv1;
	if(pack)
	{
		if((NULL == sendData) || (NULL == timeVal))
		{
			return;
		}
		else
		{
			gettimeofday(&tv1, NULL);
			memcpy(timeVal, &tv1, sizeof(struct timeval));
			if(arraySize > 2)
			{
			sendData[0] = m_sequence++;
			sendData[1] = tv1.tv_sec;
			sendData[2] = tv1.tv_usec;
			}
		}	
	}
	else
	{
		if(NULL == timeVal)
		{
			return;
		}
		else
		{
			gettimeofday(&tv1, NULL);
			memcpy(timeVal, &tv1, sizeof(struct timeval));			
		}
	}
}

void CoapServer::GetCoapPayload(otMessage *aMessage, uint64_t* rxPayloadData, uint16_t arraySize, uint16_t& payload_size)
{
	uint64_t  buf;
	uint16_t bytesToPrint;
	uint16_t bytesPrinted = 0;
	uint16_t length       = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	payload_size = length;
	uint16_t index = 0;
	
	if (length > 0)
	{
		while (length > 0)
		{
			bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
			otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, &buf, bytesToPrint);
			if(index < arraySize)
			{
				rxPayloadData[index] = buf;
				++index;
			}
			length -= bytesToPrint;
			bytesPrinted += bytesToPrint;
		}
	}
}

void CoapServer::serverReadyBroadcast(void)
{
	otError       error   = OT_ERROR_NONE;
	otMessage *   message = NULL;
	otMessageInfo messageInfo;
	char payload[7] = "00TEST";
	otCoapType   coapType               = OT_COAP_TYPE_NON_CONFIRMABLE;
	otCoapCode   coapCode               = OT_COAP_CODE_PUT;
	otIp6Address coapDestinationIp;
	char coapDestinationAddr[8] = "ff02::1";
	otIp6AddressFromString(coapDestinationAddr, &coapDestinationIp);
	
	otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,"[%x:%x:%x:%x:%x:%x:%x:%x]",	
		HostSwap16(coapDestinationIp.mFields.m16[0]),
		HostSwap16(coapDestinationIp.mFields.m16[1]), HostSwap16(coapDestinationIp.mFields.m16[2]),
		HostSwap16(coapDestinationIp.mFields.m16[3]), HostSwap16(coapDestinationIp.mFields.m16[4]),
		HostSwap16(coapDestinationIp.mFields.m16[5]), HostSwap16(coapDestinationIp.mFields.m16[6]),
		HostSwap16(coapDestinationIp.mFields.m16[7])); 
			
	message = otCoapNewMessage(m_Instance, NULL);
	if(NULL != message)
	{
		otCoapMessageInit(message, coapType, coapCode);
		otCoapMessageGenerateToken(message, K_DEFAULT_TOKEN_LENGTH/*ot::Coap::Message::kDefaultTokenLength*/);
		otCoapMessageSetPayloadMarker(message);
		error = otMessageAppend(message, payload, strlen(payload));
		if(OT_ERROR_NONE == error)
		{
			memset(&messageInfo, 0, sizeof(messageInfo));
			messageInfo.mPeerAddr    = coapDestinationIp;
			messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
			error = otCoapSendRequest(m_Instance, message, &messageInfo, NULL, NULL, true);
			if(error != OT_ERROR_NONE)
			{
				otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
					"Error in otCoapSendRequest %d\r\n",error);
				if (message != NULL)
				{
					otMessageFree(message);
				}				
			}
			#if ALL_LOGS_DEBUG
			else
			{
				otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
					"Success in otCoapSendRequest %d \n\r", error);	
			}
			#endif
		}
		else
		{
			otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
					"Error in otMessageAppend %d\r\n",error);
			if (message != NULL)
			{
				otMessageFree(message);
			}		
		}
	}
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
					"Error in otCoapNewMessage %d\r\n",error);
	}
}

#endif //OPENTHREAD_FTD
