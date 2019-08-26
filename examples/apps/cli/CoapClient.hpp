#ifndef _COAP_CLIENT_HPP_
#define _COAP_CLIENT_HPP_


#include <cstddef>
#include <iostream>                        // For CPP IO Stream
#include <openthread-core-config.h>        // For compile-time OpenThread configuration constants
#include <openthread/config.h>             // For defined configurations headers
#include <openthread/diag.h>               // For initialize diagnostics module
#include <openthread/tasklet.h>            // For Tasklets API
#include <openthread/platform/logging.h>   // For otPlatLog Abstractions
#include <openthread-system.h>             // For platform-specific functions needed by OpenThread apps
#include <openthread/platform/uart.h>      // Added for UART functionality 
#include <openthread/coap.h>               // Added for coap APIs
#include "common/instance.hpp"             // Added for instances
#include <openthread/ip6.h>                // Added for IPv6 functionalities
#include <setjmp.h>                        // For save calling environment for longjmp
#include <unistd.h>                        // Added for symbolic constants and types
#include <ctype.h>                         // Header for transforming characters
#include <string.h>                        // Added for string functions 
//#include "common/timer.hpp"				   // Added for TimerMilli
#include <timer.hpp>
#include <sys/time.h>
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "SupportAPI.hpp"

//Added for Radio

#include "platform-posix.h"


#include <openthread/random_noncrypto.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

using ot::Timer;
using ot::TimerMilli;
using ot::Instance;

class CoapClient
{
	private:
		otInstance 				*m_OtInstance;
		unsigned short 			 m_Serverport;
		uint64_t 				 m_sequence;
		bool 					 m_ServerConnect;
		uint64_t 				 m_TransmitCount;
		uint64_t 				 m_MessageTimer;
		TimerMilli               		 mCoapSendTimer;
		char 					 m_DestinationResource[12];
		otIp6Address 			 m_coapDestinationIp;


		//static Function to get called when response comes from server
		static void ResponseHandler(void *pContext,
				otMessage *pMessage,const otMessageInfo *pMessageInfo,otError error);

		//static Function to get called when request comes from server
		static void HandleDefaultRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
		
		//static Function to handle timer

		static void HandleCoapTimer(Timer &aTimer);

		// Function to handle response from server on client side
		void ClientResponseHandler(otMessage *pMessage,
				const otMessageInfo *pMessageInfo, otError error);

		// Function to handle request from server on client side
		void ProcessDefaultRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo);
		
		//Payload handlers - create and parse payload
		void GetCoapPayload(otMessage *aMessage, uint64_t* rxPayloadData, uint16_t arraySize, uint16_t& payload_size);
		void GeneratePayload(uint64_t* sendData, uint16_t arraySize, struct timeval* timeVal, bool pack);
		void GeneratePayload_mac(uint64_t* sendData1, uint64_t arraySize);

		//Send CoAP Message
		void SendMessage(otCoapCode coapCode);
		void SendMessageMac(otCoapCode coapCode);
		
		//Process the timer
		void ProcessCoapTimer(void);
		
	public:
		// Start the timer
		void StartTimer(void);

		// stop the timer
		void StopTimer(void);

		//check the timer status and triggering the timer
		bool CheckTimer();

		// Constructor for CoapClient
		CoapClient(otInstance *pInstanceTemp);

		// start coap service, using input address for destination
		otError startCoapClient(unsigned short iSourcePortTemp, char *destinationResource);

		// stop coap service
		otError stopCoapClient();

		// Use destination address in client
		void PutData(float fData,char *uri);
		
		//Send DeviceInfo 
		void SendDeviceInfo(otInstance *pInstance,char *uri);
		
		// Set default handler when there is no URI
		void SetDefaultHandler(void);

		//Destructor
		~CoapClient();
};



class CoapClientInstance
{
public:
    CoapClientInstance(void)
    {
		mCoapClient = NULL;
    }
	void SetCoapClientInstance(CoapClient* aCoapClient) { mCoapClient = aCoapClient; }
    CoapClient* GetCoapClientInstance(void) { return mCoapClient; }

private:
    CoapClient* mCoapClient;
};

#endif // _COAP_CLIENT_HPP_
