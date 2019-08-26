#ifndef _SUPPORT_API_HPP_
#define _SUPPORT_API_HPP_

#include <iostream>
#include <stdio.h>
//~ #include <assert.h>
#include <time.h>
#include <string.h>
#include <openthread/thread_ftd.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/time.h>
#include <common/timer.hpp>
#define ALL_LOGS_DEBUG 0
#define CHILD_JOIN_COUNT_CHECK 1 
#define K_DEFAULT_TOKEN_LENGTH 2                          ///< Default token length

using namespace ot;

void handleNetifStateChanged(uint32_t aFlags, void *aContext);
void handleNeighborTableChangedCallback(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aEntryInfo);

int getChildCount(void);
int getRouterCount(void);
otDeviceRole getDeviceRole(void);

#endif // _SUPPORT_API_HPP_

