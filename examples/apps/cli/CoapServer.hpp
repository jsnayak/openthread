#ifndef _COAP_SERVER_HPP_
#define _COAP_SERVER_HPP_

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
#include <openthread/thread_ftd.h>		   // Added for otThreadGetChildInfoByIndex
#include "common/instance.hpp"             // Added for instances
#include <openthread/ip6.h>                // Added for IPv6 functionalities
#include <setjmp.h>                        // For save calling environment for longjmp
#include <unistd.h>                        // Added for symbolic constants and types
#include <ctype.h>                         // Header for transforming characters
#include <string.h>                        // Added for string functions 

#include <sys/time.h>
#include "common/encoding.hpp"
//#include "SupportAPI.hpp"

/************************************************************************
 * @brief  : Class for Coap Server App
 *************************************************************************/
 #if OPENTHREAD_FTD
class CoapServer
{
	private:
		// holds the resource
		otCoapResource mResource;   

		// UriPath of resource
		char m_ServerURI[10];
		otInstance *m_Instance;
		unsigned int m_Port;
		uint64_t m_sequence;

		// Function to send coap Response from server
		void serverResponseHandler(otMessage *pMessage, const otMessageInfo *pMessageInfo);

		// static function to get called when the request is received from client.
		static void serverRegisterHandler(void *pContext, 
				otMessage *pMessage, const otMessageInfo *pMessageInfo);
				
		void GeneratePayload(uint64_t* sendData, uint16_t arraySize, 
				struct timeval* timeVal, bool pack);
				
		void GetCoapPayload(otMessage *aMessage, uint64_t* rxPayloadData, 
				uint16_t arraySize, uint16_t& payload_size);
				
	public:
		// Constructor for CoapServer
		CoapServer(otInstance *pInstance);

		// start coap service
		otError startCoapServer(unsigned int iPort);

		// stop coap service
		otError stopCoapServer();

		// Add a resource using argument
		otError addResource(const char* pResource);    

		void serverReadyBroadcast(void);
		
		//Destructor for CoapServer
		~CoapServer();
};
#endif //OPENTHREAD_FTD
#endif //_COAP_SERVER_HPP_
