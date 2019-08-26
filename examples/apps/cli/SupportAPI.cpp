#include "SupportAPI.hpp"

using namespace ot;
static int g_numChild = 0;
static int g_numRouters = 0;

static otDeviceRole g_changedRole = OT_DEVICE_ROLE_DISABLED;


void handleNeighborTableChangedCallback(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aEntryInfo)
{
	OT_UNUSED_VARIABLE(aEntryInfo);
	char buffer [80];
	time_t rawtime = time(0);
	struct tm * timeinfo = localtime (&rawtime); 
	strftime (buffer,80,"%F-%T",timeinfo);

	otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_API, "%s, Neighbor Table Change Detected ... \n", buffer);
	switch (aEvent)
	{
		case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:  ///< A child is being added.
			g_numChild++;
			otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Neighbor_Change, CHILD_ADDED,    TOT_CHILD = %d, TOT_ROUTERS = %d\r\n", buffer, g_numChild, g_numRouters);
			break;
		case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED: ///< A child is being removed
			if(g_numChild > 0)
			g_numChild--;
			otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Neighbor_Change, CHILD_REMOVED,  TOT_CHILD = %d, TOT_ROUTERS = %d\r\n", buffer, g_numChild, g_numRouters);
			break;
		case OT_NEIGHBOR_TABLE_EVENT_ROUTER_ADDED: ///< A router is being added.
			g_numRouters++;
			otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Neighbor_Change, ROUTER_ADDED,   TOT_CHILD = %d, TOT_ROUTERS = %d", buffer, g_numChild, g_numRouters);
			break;
		case OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED: ///< A router is being removed.
			if(g_numRouters > 0)
			g_numRouters--;
			otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Neighbor_Change, ROUTER_REMOVED, TOT_CHILD = %d, TOT_ROUTERS = %d\r\n", buffer, g_numChild, g_numRouters);
			break;
		default:
			break;
	}
}

void handleNetifStateChanged(uint32_t aFlags, void *aContext)
{
	if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
	{
		otDeviceRole changedRole = otThreadGetDeviceRole((otInstance *)aContext);
		char buffer [80];
		time_t rawtime = time(0);
		struct tm * timeinfo = localtime (&rawtime);
		strftime (buffer,80,"%F-%T",timeinfo);
		g_changedRole = changedRole;
		switch (changedRole)
		{
			case OT_DEVICE_ROLE_LEADER:
				otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Role_Change, LEADER\n", buffer);
				break;

			case OT_DEVICE_ROLE_ROUTER:
				otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Role_Change, ROUTER\n", buffer);
				break;

			case OT_DEVICE_ROLE_CHILD:
				otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Role_Change, CHILD\n", buffer);
				break;

			case OT_DEVICE_ROLE_DETACHED:
				otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Role_Change, DETACHED\n", buffer);
				break;

			case OT_DEVICE_ROLE_DISABLED:
				otPlatLog(OT_LOG_LEVEL_CRIT, OT_LOG_REGION_CLI, "%s, Role_Change, DISABLED\n", buffer);
				break;
		}
	}
}

int getChildCount(void)
{
	return g_numChild;
}

int getRouterCount(void)
{
	return g_numRouters;
}

otDeviceRole getDeviceRole(void)
{
	return g_changedRole;
}
///////////////////////////////////////////////////For timer functionalities///////////
/*
CTimer::CTimer(otInstance* pInstancesuedo)
{
	pInstancePtr = reinterpret_cast<Instance *>(pInstancesuedo);
	pTimerMilli = NULL;
}
CTimer::~CTimer()
{
	delete pTimerMilli;
}
void CTimer::createMilliTimer(void(*pHandler)(Timer &timer))
{
	pTimerMilli = new TimerMilli(*pInstancePtr,pHandler,(void *)this);
}
void CTimer::startMilliTimer(int iMillitime)
{
	pTimerMilli->Start(iMillitime);
}
void CTimer::stopMilliTimer()
{
	pTimerMilli->Stop();
}
*/
