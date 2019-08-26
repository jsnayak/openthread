
#include "CoapClient.hpp"                // Header file for client Coap APIs 

using ot::Encoding::BigEndian::HostSwap16;

CoapClientInstance g_getCoapClientInstance;

CoapClient::CoapClient(otInstance *pInstanceTemp):
	m_OtInstance(pInstanceTemp),
	m_Serverport(OT_DEFAULT_COAP_PORT),
	m_sequence(0),
	m_ServerConnect(false),
	m_TransmitCount(25),
	m_MessageTimer(0),
	mCoapSendTimer(*static_cast<Instance *>(pInstanceTemp), &CoapClient::HandleCoapTimer, this)	
{
	#if ALL_LOGS_DEBUG
	otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_API, "Coap Secure Client Constructor\n");
	#endif
	
	memset(m_DestinationResource, 0x00, sizeof(m_DestinationResource));
	memset(&m_coapDestinationIp, 0x00, sizeof(m_coapDestinationIp));
	
}

CoapClient::~CoapClient()
{
	#if ALL_LOGS_DEBUG
	otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_API, "Coap Secure Client Destructor\n");
	#endif
}

otError CoapClient::startCoapClient(unsigned short iSourcePortTemp, char *destinationResource)
{
	otError error = OT_ERROR_NONE;

	m_Serverport = iSourcePortTemp;
	memcpy(m_DestinationResource, destinationResource, sizeof(m_DestinationResource));
	g_getCoapClientInstance.SetCoapClientInstance(this);
	// Coap Secure start with port as 5684
	error = otCoapStart(m_OtInstance, m_Serverport);
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

otError CoapClient::stopCoapClient()
{
	// Coap Secure Stop
	otError error = otCoapStop(m_OtInstance);
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

void CoapClient::ResponseHandler(void *pContext, otMessage *pMessage,
	const otMessageInfo *pMessageInfo,otError error)
{
	static_cast<CoapClient *>(pContext)->ClientResponseHandler(
			pMessage, pMessageInfo, error);
	OT_UNUSED_VARIABLE(pContext);
}

void CoapClient::ClientResponseHandler(otMessage *pMessage,
				const otMessageInfo *pMessageInfo, otError error)
{
	struct timeval  tv1;
	uint16_t payload_size = 0;
	GeneratePayload(NULL, 0, &tv1, false);
	if (error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,"C_rxCoapResp,op=,sec=%ld,%ld,error=%d,error=%s\r\n", 
					tv1.tv_sec, tv1.tv_usec, error, otThreadErrorToString(error));
	}
	else
	{        
		otCoapCode   coapCode               = otCoapMessageGetCode(pMessage);
		uint64_t 	rxPayloadData[6] = {0};
		char 		 operation[7] 			= {'\0'};
		switch (coapCode)
		{
			case OT_COAP_CODE_GET:
			case OT_COAP_CODE_CONTENT:
			GetCoapPayload(pMessage, rxPayloadData, 6, payload_size);
			strcpy(operation, "get");
			otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"C_rxCoapResp,op=%s,sec=%ld,%ld,PLSize=%d,PL=%lld,%lld,%lld,%lld,%lld,%lld,from=[%x:%x:%x:%x:%x:%x:%x:%x]\n\r", 
			operation, tv1.tv_sec, tv1.tv_usec, payload_size, 
			rxPayloadData[0], rxPayloadData[1], rxPayloadData[2],
			rxPayloadData[3], rxPayloadData[4], rxPayloadData[5],
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[0]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[1]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[2]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[3]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[4]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[5]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[6]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[7])); 				
			break;

			case OT_COAP_CODE_DELETE:
			strcpy(operation, "delete");
			break;
			
			case OT_COAP_CODE_VALID:
			case OT_COAP_CODE_PUT:
			GetCoapPayload(pMessage, rxPayloadData, 3, payload_size);
			strcpy(operation, "put");
			otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"C_rxCoapResp,op=%s,sec=%ld,%ld,PLSize=%d,PL=%lld,%lld,%lld,,,,,from=[%x:%x:%x:%x:%x:%x:%x:%x]\n\r", 
			operation, tv1.tv_sec, tv1.tv_usec, payload_size, 
			rxPayloadData[0], rxPayloadData[1], rxPayloadData[2],
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[0]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[1]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[2]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[3]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[4]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[5]), HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[6]),
			HostSwap16(pMessageInfo->mPeerAddr.mFields.m16[7])); 
			break;

			case OT_COAP_CODE_POST:
			strcpy(operation, "post");
			break;

			default:
			//~ //otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,"Undefined\r\n");
			break;
		}        
	}

	OT_UNUSED_VARIABLE(pMessageInfo);
	OT_UNUSED_VARIABLE(pMessage);
}

void CoapClient::GeneratePayload(uint64_t* sendData, uint16_t arraySize, struct timeval* timeVal, bool pack)
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

void CoapClient::GeneratePayload_mac(uint64_t* sendData1, uint64_t arraySize)
{

        otRadioFrame Ccavalue;
        memset(&Ccavalue,0,sizeof(Ccavalue));
        Ccavalue = *(otPlatRadioGetTransmitBuffer(m_OtInstance));
        uint8_t value = Ccavalue.mInfo.mTxInfo.mMaxCsmaBackoffs;
        otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "The Vlaue Of CCA BACKOFF =%ld\r\n",value);

	otMacCounters mac_c;
	memset(&mac_c,0,sizeof(mac_c));
	mac_c = *(otLinkGetCounters(m_OtInstance));
	if(NULL == sendData1) 
	{
		return;
	}
	else
	{
		if(arraySize < 6)
		{
			sendData1[0] = value;
			sendData1[1] = mac_c.mTxRetry;
			sendData1[2] = mac_c.mTxErrCca;
			sendData1[3] = mac_c.mTxAcked;
		}
	}
}
/*
void RadioPayload(uint64_t*  sendData2, uint64_t arraySize)
{
	otRadioFrame Ccavalue;
        memset(&Ccavalue,0,sizeof(Ccavalue));
        Ccavalue = *(otPlatRadioGetTransmitBuffer(m_OtInstance));
        uint8_t value = Ccavalue.mInfo.mTxInfo.mMaxCsmaBackoffs;
        otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "The Vlaue Of CCA BACKOFF =%ld\r\n",value);	
	
	if(NULL == sendData2) 
	{
		return;
	}
	else
	{
		if(arraySize > 0)
		{
			sendData1[0] = mac_c.mTxAck,0,0,64785,ed;
		}
	}




}*/
void CoapClient::GetCoapPayload(otMessage *aMessage, uint64_t* rxPayloadData, uint16_t arraySize, uint16_t& payload_size)
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

void CoapClient::SetDefaultHandler(void)
{
	otCoapSetDefaultHandler(m_OtInstance, &CoapClient::HandleDefaultRequest, this);
}


void CoapClient::HandleDefaultRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
	static_cast<CoapClient *>(aContext)->ProcessDefaultRequest(aMessage, aMessageInfo);
}

void CoapClient::ProcessDefaultRequest(otMessage *pMessage, const otMessageInfo *pMessageInfo)
{
	//~ otError error = OT_ERROR_NONE;
	otCoapType   coapType               = otCoapMessageGetType(pMessage);
	otCoapCode   coapCode               = otCoapMessageGetCode(pMessage);	
	
    char  buf;
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length       = otMessageGetLength(pMessage) - otMessageGetOffset(pMessage);
    uint16_t payload_size = length;
	char payload[payload_size] = {'\0'};
    uint16_t index = 0;
    
    if (length > 0)
    {
        while (length > 0)
        {
            bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
            otMessageRead(pMessage, otMessageGetOffset(pMessage) + bytesPrinted, &buf, bytesToPrint);

            if(index < payload_size)
            {
                payload[index] = buf;
                ++index;
            }
            length -= bytesToPrint;
            bytesPrinted += bytesToPrint;
        }
    }	
	
	char expectedPayload[7] = "00TEST";

	if((!m_ServerConnect) && 
	   ((OT_COAP_CODE_PUT == coapCode) || (OT_COAP_CODE_GET == coapCode)) && 
	   !(strcmp(payload,expectedPayload)))
	{
		m_ServerConnect = true;
		memcpy(&m_coapDestinationIp, &(pMessageInfo->mPeerAddr), sizeof(m_coapDestinationIp)); 
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"[%x:%x:%x:%x:%x:%x:%x:%x] -- %s\n\r", 
			HostSwap16(m_coapDestinationIp.mFields.m16[0]),
			HostSwap16(m_coapDestinationIp.mFields.m16[1]), HostSwap16(m_coapDestinationIp.mFields.m16[2]),
			HostSwap16(m_coapDestinationIp.mFields.m16[3]), HostSwap16(m_coapDestinationIp.mFields.m16[4]),
			HostSwap16(m_coapDestinationIp.mFields.m16[5]), HostSwap16(m_coapDestinationIp.mFields.m16[6]),
			HostSwap16(m_coapDestinationIp.mFields.m16[7]), m_DestinationResource);
			
		mCoapSendTimer.Start(CHILD_JOIN_COUNT_CHECK * 180 * 1000);
	}
	
	OT_UNUSED_VARIABLE(pMessage);
	OT_UNUSED_VARIABLE(pMessageInfo);
	OT_UNUSED_VARIABLE(coapType);
	OT_UNUSED_VARIABLE(coapCode);
}

void CoapClient::SendMessage(otCoapCode coapCode)
{
	otCoapType coapType = OT_COAP_TYPE_NON_CONFIRMABLE;
	if(OT_COAP_CODE_GET == coapCode)
	{
		coapType = OT_COAP_TYPE_CONFIRMABLE;
	}
    otError       error   = OT_ERROR_NONE;

    otMessageInfo messageInfo;
    otMessage *   message 				= NULL;
    
    uint64_t sendData[3] = {0};
    uint16_t payload_transmit = sizeof(sendData);
    struct timeval  tv1;    
    
	message = otCoapNewMessage(m_OtInstance, NULL);
	if(NULL != message)
	{
		otCoapMessageInit(message, coapType, coapCode);
		otCoapMessageGenerateToken(message, K_DEFAULT_TOKEN_LENGTH/*ot::Coap::Message::kDefaultTokenLength*/);
		error = otCoapMessageAppendUriPathOptions(message, m_DestinationResource);
		if(OT_ERROR_NONE == error)
		{
			otCoapMessageSetPayloadMarker(message);
			GeneratePayload(sendData, 3, &tv1, true);
			error = otMessageAppend(message, sendData, payload_transmit);
			
			if(OT_ERROR_NONE == error)
			{
				memset(&messageInfo, 0, sizeof(messageInfo));
				messageInfo.mPeerAddr    = m_coapDestinationIp;
				messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
				if((coapType == OT_COAP_TYPE_CONFIRMABLE) || (coapCode == OT_COAP_CODE_GET))
				{
		//			error = otCoapSendRequest(m_OtInstance, message, &messageInfo, &CoapClient::ResponseHandler, this, true);
		//		}
		//		else
		//		{
					error = otCoapSendRequest(m_OtInstance, message, &messageInfo, NULL, NULL, true);
				}
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
				elser this packet, false otherwise. 
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
					"Error in otCoapMessageAppendUriPathOptions %d\r\n",error);
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


void CoapClient::SendMessageMac(otCoapCode coapCode)
{
	otCoapType coapType = OT_COAP_TYPE_NON_CONFIRMABLE;
	otError   error   = OT_ERROR_NONE;

    otMessageInfo messageInfo;
    otMessage *   message = NULL;
    
    uint64_t sendData1[4] = {0};
    uint16_t payload_transmit = sizeof(sendData1);
    
	message = otCoapNewMessage(m_OtInstance, NULL);
	if(NULL != message)
	{
		otCoapMessageInit(message, coapType, coapCode);
		otCoapMessageGenerateToken(message, K_DEFAULT_TOKEN_LENGTH/*ot::Coap::Message::kDefaultTokenLength*/);
		error = otCoapMessageAppendUriPathOptions(message, m_DestinationResource);
		if(OT_ERROR_NONE == error)
		{
			otCoapMessageSetPayloadMarker(message);
			GeneratePayload_mac(sendData1,4);
			error = otMessageAppend(message, sendData1, payload_transmit);
			
			if(OT_ERROR_NONE == error)
			{
				memset(&messageInfo, 0, sizeof(messageInfo));
				messageInfo.mPeerAddr    = m_coapDestinationIp;
				messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
				error = otCoapSendRequest(m_OtInstance, message, &messageInfo, NULL, NULL, true);
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
					"Error in otCoapMessageAppendUriPathOptions %d\r\n",error);
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

void CoapClient::HandleCoapTimer(Timer &aTimer)
{
	#if ALL_LOGS_DEBUG
    otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
					"HandleCoapTimer\n\r");
	#endif
	
	OT_UNUSED_VARIABLE(aTimer);
	
    CoapClient* client = g_getCoapClientInstance.GetCoapClientInstance();
    if(NULL != client)
    {
		client->ProcessCoapTimer();
	}
}

void CoapClient::ProcessCoapTimer(void)
{
	++m_MessageTimer;
	
	if(0 == (m_MessageTimer % 10) && (m_TransmitCount > 0))
	{
		--m_TransmitCount;
		this->SendMessage(OT_COAP_CODE_GET);
	}
	
	if(0 == (m_MessageTimer % 2))
	{
		this->SendMessageMac(OT_COAP_CODE_PUT);
	}
    otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,"ProcessCoapTimer  --  %s %d \n\r", m_DestinationResource, m_TransmitCount);	
	mCoapSendTimer.Start(60 * 1000);
//	mCoapSendTimer.Start(20 * 1000);
}


///// function for timer start//////

void CoapClient::StartTimer(void)
{
	mCoapSendTimer.Start( 180 * 1000);
}

///// function to stop timer////
void CoapClient::StopTimer(void)
{       
        mCoapSendTimer.Stop();
}  
///////function tocheck the status/////

bool  CoapClient::CheckTimer()
{       
   	bool check =  mCoapSendTimer.IsRunning();
   	return check;
}  
