#ifndef __GPU_QUERY_H__
#define __GPU_QUERY_H__

#include <string>
#include <vector>

using namespace std;

#include <nvml.h>

enum NV_REQUEST
{
	REQ_GPU_COUNT,			// Physical GPU Count
	REQ_GPU_UCOUNT,			// Used GPU Count
	REQ_DRV_VERSION,		// Driver Version
	REQ_DVI_NAME,			// Device Name
	REQ_DVI_UUID,			// UUID
	REQ_DVI_PCIID,			// PCI ID
	REQ_DVI_SERIAL,			// Device Serial
	REQ_DVI_TEMP,			// Device Temperature
	REQ_DVI_UTILRATE,		// Device Utilization Rate
	REQ_DVI_FANSPEED,		// Device Fan Speed
	REQ_DVI_PERFSTATE,		// Performance State
	REQ_CLK_GRAPHICS,		// Clock Graphics
	REQ_CLK_SM,				// Clock SM
	REQ_CLK_MEM,			// Clock MEM
	REQ_CLK_MAX_GRAPHICS,	// Max Clock Graphics
	REQ_CLK_MAX_SM,			// Max Clock SM
	REQ_CLK_MAX_MEM,		// Max Clock MEM
	REQ_PWR_USAGE,			// Power Usage Report
	REQ_PWR_MODE,			// Power Management Mode
	REQ_PWR_LIMIT,			// Power Management Limit
	REQ_ECC_MODE,			// ECC Mode
	REQ_ECC_DBERR,			// ECC db Error
	REQ_ECC_SBERR,			// ECC sb Error
	REQ_MEM_TOTAL,			// Memory Total
	REQ_MEM_USED,			// Memory Used
	REQ_MEM_UTILRATE,		// Memory Utilization Rate
	REQ_BAR1_MEM,			// Bar1 Memory Usage
	REQ_BAR1_MAX_MEM,		// Bar1 Memory Max Usage
	REQ_TMP_SHUTDOWN,		// Shutdown Temperature
	REQ_TMP_SLOWDOWN,		// Slowdown Temperature
	REQ_ENC_UTIL,			// Encoder Util
	REQ_DEC_UTIL,			// Decoder Util
	REQ_PWR_VIOLATION,		// Power Violation
	REQ_GPU_PROCESS			// Processes
};

#define MAX_PROCESS_COUNT 1024

typedef struct _DeviceInfo
{
	nvmlReturn_t		Status;
	nvmlDevice_t		Device;
	char            	Name[NVML_DEVICE_NAME_BUFFER_SIZE];
	nvmlPciInfo_t		PCI;
	nvmlEnableState_t	AccountingMode;
	unsigned int		ProcessCount;
	unsigned int 		ProcessList[MAX_PROCESS_COUNT];
	nvmlAccountingStats_t	AccountingStatList[MAX_PROCESS_COUNT];
	nvmlComputeMode_t 	ComputeMode;
} DeviceInfo, * pDeviceInfo;

typedef vector<DeviceInfo>	DeviceList;
typedef vector<DeviceInfo>::iterator DeviceItr;

class GpuQuery
{
private:
	nvmlReturn_t		mQueryReturn;
	
	DeviceList			mDevices;

	unsigned int		mDeviceCount;

	string				mStrReport;

public:
	GpuQuery();
	~GpuQuery();

	// For initialization
	int InitializeNVML();
	int GetDeviceCount();
	int InitializeQuery();

	// Shutdown
	int ShutdownNVML();

	// For Device Queries
	int GetDevices();
	int GetDeviceNames();
	int GetPCIInfos();
	int GetAccountingModes();

	int EnableAccountingModes(nvmlEnableState_t paramEnable);
	int ClearAccountingPids();
	int GetAccountingPids();
	int GetAccountingStat(int paramIndex);
	int GetAccountingStats();

	float GetMaxMemoryBytes(int paramIndex);
	float GetMemUtilization(int paramIndex);
	float GetGpuUtilization(int paramIndex);

	// Show Something
	void PrintAccountingInfo(int paramIndex);
	void PrintDeviceInfo(int paramIndex);
	void PrintAllDeviceInfo();
	void PrintErrorString(const char * paramNote);

	// Helper Helper Helper
	// Please Please Help me! -- Beatles Help!
	const char * ComputeModeToString(nvmlComputeMode_t paramMode);
};

#endif // __GPU_QUERY_H__
