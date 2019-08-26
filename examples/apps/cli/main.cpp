/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

//~ #include <assert.h>
#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
//~ #include <openthread/timer.hpp>
#include <openthread/platform/logging.h>

#include "openthread-system.h"
#include "CoapServer.hpp"
#include "CoapClient.hpp"
#include "SupportAPI.hpp"
#include <cstdlib>					// atoi

/* Added headers for monitoring Thread Role Change */
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <openthread/instance.h>
#include <openthread/dataset.h>
#include <openthread/coap.h>
#include <openthread/thread_ftd.h>
#include <openthread/thread.h>

// Added
#include "platform-posix.h"
#include <openthread/random_noncrypto.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "utils/code_utils.h"
#define TRUE 1
#if OPENTHREAD_EXAMPLES_POSIX
#include <setjmp.h>
#include <unistd.h>
jmp_buf gResetJump;

void __gcov_flush();
#endif

/* Extern Function Header Declarations */
void InitThreadNetwork(int argc, char *argv[]);

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
void *otPlatCAlloc(size_t aNum, size_t aSize)
{
	return calloc(aNum, aSize);
}

void otPlatFree(void *aPtr)
{
	free(aPtr);
}
#endif

static bool g_coapStatus = false;
static bool g_coapReady = false;

#if OPENTHREAD_FTD
static int g_childBroadcastNotifyCount = 0;
unsigned int iTxpower = 5;   // IEEE 802.15.4 Txpower
#endif

#if OPENTHREAD_MTD
unsigned int iTxpower = 3;   // IEEE 802.15.4 Txpower
#endif

void otTaskletsSignalPending(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
}

/*
 * Provide, if required an "otPlatLog()" function
 */
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
	OT_UNUSED_VARIABLE(aLogLevel);
	OT_UNUSED_VARIABLE(aLogRegion);
	OT_UNUSED_VARIABLE(aFormat);

	va_list ap;
	va_start(ap, aFormat);
	otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
	va_end(ap);
}
#endif
#define VerifyOrExit(aCondition, ...) \
	do                                \
{                                 \
	if (!(aCondition))            \
	{                             \
		__VA_ARGS__;              \
		goto exit;                \
	}                             \
} while (false)


#define ExitNow(...) \
	do               \
{                \
	__VA_ARGS__; \
	goto exit;   \
} while (false)


int Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength)
{
	size_t      hexLength = strlen(aHex);
	const char *hexEnd    = aHex + hexLength;
	uint8_t *   cur       = aBin;
	uint8_t     numChars  = hexLength & 1;
	uint8_t     byte      = 0;
	int         rval;

	VerifyOrExit((hexLength + 1) / 2 <= aBinLength, rval = -1);

	while (aHex < hexEnd)
	{
		if ('A' <= *aHex && *aHex <= 'F')
		{
			byte |= 10 + (*aHex - 'A');
		}
		else if ('a' <= *aHex && *aHex <= 'f')
		{
			byte |= 10 + (*aHex - 'a');
		}
		else if ('0' <= *aHex && *aHex <= '9')
		{
			byte |= *aHex - '0';
		}
		else
		{
			ExitNow(rval = -1);
		}

		aHex++;
		numChars++;

		if (numChars >= 2)
		{
			numChars = 0;
			*cur++   = byte;
			byte     = 0;
		}
		else
		{
			byte <<= 4;
		}
	}

	rval = static_cast<int>(cur - aBin);

exit:
	return rval;
}

/************************************************************************
 * @brief  : Function InitThreadNetwork is used to Initialize otInstance,
 *           Initialize channel,panid, ifconfig, Joiner Key and thread
 * @param  : command line arguments to be used to initialize OT Sys Init.
 *           The node offset given at command line would open a port
 *           at port 9000 + offset value.
 * @return : void
 *************************************************************************/
void InitThreadNetwork(int argc, char *argv[])
{
	otInstance *pInstance = NULL; // Pointer to the OT instance
	unsigned int iChannel = 26;   // IEEE 802.15.4 Channel
	unsigned int iPanid = 0xABCD; // IEEE 802.15.4 Pan ID
	otOperationalDataset aDataset;
	otError error = OT_ERROR_NONE;

#if OPENTHREAD_EXAMPLES_POSIX
	if (setjmp(gResetJump))
	{
		alarm(0);
#if OPENTHREAD_ENABLE_COVERAGE
		__gcov_flush();
#endif
		execvp(argv[0], argv);
	}
#endif

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
	size_t   otInstanceBufferLength = 0;
	uint8_t *otInstanceBuffer       = NULL;
#endif

pseudo_reset:

	//Initialize OT using system Init
	otSysInit(argc, argv);

	// Initialize otInstance & verify it's valid
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
	// Call to query the buffer size
	(void)otInstanceInit(NULL, &otInstanceBufferLength);

	// Call to allocate the buffer
	otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
	assert(otInstanceBuffer);

	// Initialize OpenThread with the buffer
	pInstance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
	pInstance = otInstanceInitSingle();
#endif
	assert(pInstance);

	// Initialize Cli Uart
	otCliUartInit(pInstance);

#if OPENTHREAD_FTD
	/* Register neighbor state change handler */
	otThreadRegisterNeighborTableCallback(pInstance, handleNeighborTableChangedCallback);
	otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"Registered for neighbor state change handler.\r\n");
#else
	/* Register Thread state change handler */
	otSetStateChangedCallback(pInstance, handleNetifStateChanged, pInstance);
#endif

#if OPENTHREAD_FTD
	otDatasetSetActive(pInstance, &aDataset);

	/* Set the router selection jitter to override the 2 minute default.
	   CLI cmd >routerselectionjitter 20
Warning: For demo purposes only - not to be used in a real product */
//	uint8_t jitterValue = 225;
//	otThreadSetRouterSelectionJitter(pInstance, jitterValue);
#else
	OT_UNUSED_VARIABLE(pInstance);
	OT_UNUSED_VARIABLE(aDataset);
#endif

	// Set channel ID
	error = otLinkSetChannel(pInstance, static_cast<uint8_t>(iChannel));
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in setting Channel -  %d\r\n",error);
		exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting Channel - %d\r\n",iChannel);	
	}
#endif

	//Set txpower
#if OPENTHREAD_FTD
	error = otPlatRadioSetTransmitPower(pInstance, static_cast<int8_t>(iTxpower));
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in setting Txpower -  %d\r\n",error);
		exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting Txpower - %d\r\n",iTxpower);
	}
#endif
#endif

#if OPENTHREAD_MTD
	error = otPlatRadioSetTransmitPower(pInstance, static_cast<int8_t>(iTxpower));
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in setting Txpower -  %d\r\n",error);
		exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting Txpower - %d\r\n",iTxpower);
	}
#endif
#endif

	// Set PANID
	error = otLinkSetPanId(pInstance, (otPanId)iPanid);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in setting Pan ID - %d\r\n",error);
		exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting Pan ID - %x\r\n",iPanid);	
	}
#endif

	// Set Network Name
	char networkName[15] = "Test_Network";
	error = otThreadSetNetworkName(pInstance, networkName);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in setting Network Name - %d\r\n",error);
		exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting Network Name - %s\r\n",networkName);	
	}
#endif

	char masterkey[33] = "00112233445566778899aabbccddeeff";
	otMasterKey key;
	int masterKeyLen = Hex2Bin(masterkey, key.m8, sizeof(key.m8));
	error = otThreadSetMasterKey(pInstance, &key);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in otThreadSetMasterKey %d\r\n",error);
		OT_UNUSED_VARIABLE(masterKeyLen);
		//~ exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in otThreadSetMasterKey %d \n\r", masterKeyLen);	
	}
#endif

	char extpanid[17] = "1111111122222222";
	otExtendedPanId extPanId;
	int extPanIDLen = Hex2Bin(extpanid, extPanId.m8, sizeof(extPanId));
	error = otThreadSetExtendedPanId(pInstance, &extPanId); 
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in otThreadSetExtendedPanId - %d\r\n",error);
		OT_UNUSED_VARIABLE(extPanIDLen);
		//~ exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting otThreadSetExtendedPanId %d \n\r", extPanIDLen);	
	}
#endif  

	//Starting IPv6 Interface
	error = otIp6SetEnabled(pInstance, true);
	if(error != OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in activating IPv6 - %d\r\n",error);
		exit(-1);
	}
#if ALL_LOGS_DEBUG
	else
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Success in setting otIp6SetEnabled \n\r");	
	}
#endif

	// Starting thread network
	error = otThreadSetEnabled(pInstance, true);
	if(error!=OT_ERROR_NONE)
	{
		otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
				"Error in starting Thread Network - %d\r\n",error);
		exit(-1);
	}

#if OPENTHREAD_FTD
	CoapServer coapServer(pInstance);
#endif

	int childJoinCount = CHILD_JOIN_COUNT_CHECK;
	//~ #if ALL_LOGS_DEBUG
	otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
			"childJoinCount %d \n\r", childJoinCount);	
	//~ #endif

#if OPENTHREAD_MTD
	CoapClient coapClient(pInstance);
#endif

	while (!otSysPseudoResetWasRequested())
	{
		otTaskletsProcess(pInstance);
		otSysProcessDrivers(pInstance);
#if OPENTHREAD_FTD
		if((!g_coapStatus) && (getChildCount() >= 1))
		{
			error = coapServer.startCoapServer(OT_DEFAULT_COAP_PORT);
			if(error == OT_ERROR_NONE)
			{
				char resource[7] = "server";
				error = coapServer.addResource(resource);
				if(error == OT_ERROR_NONE)
				{
					g_coapStatus = true;
					g_coapReady = true;
					otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
							"CoAP Server Ready\r\n");		    
				}
				else
				{
					coapServer.stopCoapServer();
				}
			}
		}
		if((g_coapReady) && 
				(getChildCount() >= childJoinCount) && 
				(getChildCount() != g_childBroadcastNotifyCount))
		{
			//Broadcast Server Ready Message
			coapServer.serverReadyBroadcast();
			g_childBroadcastNotifyCount = getChildCount();
		}		
#endif

#if OPENTHREAD_MTD
		if((!g_coapStatus))
		{
			if ((OT_DEVICE_ROLE_CHILD == getDeviceRole()))
			{
				char URI[7] = "server";
				error = coapClient.startCoapClient(OT_DEFAULT_COAP_PORT, URI);
				if(error == OT_ERROR_NONE)
				{
					coapClient.SetDefaultHandler();
					g_coapStatus = true;
					g_coapReady = true;
					otPlatLog(OT_LOG_LEVEL_CRIT,OT_LOG_REGION_API,
							"CoAP Client Ready\r\n");
					// start timer
		 			coapClient.StartTimer();
			

				}
			}
		}
		else
		{
			if((OT_DEVICE_ROLE_CHILD != getDeviceRole()))
			{
				// stop timer
				coapClient.StopTimer();
			}
			else
			{
				//check timer status and starts if its is stopped
				int  status = coapClient.CheckTimer();
				if(status != TRUE)
				
				{
					//timer is inactive so starting timer 
					coapClient.StartTimer();
				}
			}

	     }
#endif
	
	}	
	otInstanceFinalize(pInstance);
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
	free(otInstanceBuffer);
#endif

	goto pseudo_reset;
}

int main(int argc, char *argv[])
{
	otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "Scalability Test Util v0.2\r\n");
	InitThreadNetwork(argc, argv);
	return 0;
}
