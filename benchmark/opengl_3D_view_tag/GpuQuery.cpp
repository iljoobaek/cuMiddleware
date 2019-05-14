#include <stdio.h>
//#include <nvml.h>

#include <iostream>
#include <fstream>

#include "GpuQuery.h"

GpuQuery::GpuQuery()
{
}

GpuQuery::~GpuQuery()
{
}

int GpuQuery::InitializeNVML()
{
    mQueryReturn = nvmlInit();

    if (NVML_SUCCESS != mQueryReturn) return 0;

	return 1;
}

int GpuQuery::GetDeviceCount()
{
    mQueryReturn = nvmlDeviceGetCount(&mDeviceCount);

    if (NVML_SUCCESS != mQueryReturn)	return 0;

	return 1;
}

int GpuQuery::InitializeQuery()
{
	if (InitializeNVML() == 0)
	{
		PrintErrorString("[NVML INIT] has failed");
		return 0;
	}

	if (GetDeviceCount() == 0)
	{
		PrintErrorString("[NVML DEVICE COUNT] has failed");
		return 0;
	}

	return 1;
}

int GpuQuery::ShutdownNVML()
{
    mQueryReturn = nvmlShutdown();

    if (NVML_SUCCESS != mQueryReturn) return 0;

    return 1;
}


int GpuQuery::GetDevices()
{
	int i;

	DeviceInfo tDevInfo;

	for (i = 0; i < mDeviceCount; i++)
	{
		mQueryReturn = nvmlDeviceGetHandleByIndex(i, &(tDevInfo.Device));
		tDevInfo.Status = mQueryReturn;
		mDevices.push_back(tDevInfo);
	}
}

int GpuQuery::GetDeviceNames()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if (mDevices.at(i).Status == NVML_SUCCESS)
	        mQueryReturn = nvmlDeviceGetName(mDevices.at(i).Device, mDevices.at(i).Name, NVML_DEVICE_NAME_BUFFER_SIZE);
	}
}

int GpuQuery::GetPCIInfos()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if (mDevices.at(i).Status == NVML_SUCCESS)
		{
	        mQueryReturn = nvmlDeviceGetPciInfo(mDevices.at(i).Device, &(mDevices.at(i).PCI));
		}
	}
}

int GpuQuery::GetAccountingModes()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if (mDevices.at(i).Status == NVML_SUCCESS)
		{
	        mQueryReturn = nvmlDeviceGetAccountingMode(mDevices.at(i).Device, &(mDevices.at(i).AccountingMode));
		}
	}
}

int GpuQuery::EnableAccountingModes(nvmlEnableState_t paramEnable)
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if ( (mDevices.at(i).Status == NVML_SUCCESS) && (mDevices.at(i).AccountingMode != paramEnable) )
		{
    	    mQueryReturn = nvmlDeviceSetAccountingMode(mDevices.at(i).Device, paramEnable);
			PrintErrorString("Enable Account Mode");
		}
	}
}

int GpuQuery::ClearAccountingPids()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if ( (mDevices.at(i).Status == NVML_SUCCESS) && (mDevices.at(i).AccountingMode == NVML_FEATURE_ENABLED) )
		{
    	    mQueryReturn = nvmlDeviceClearAccountingPids(mDevices.at(i).Device);
			PrintErrorString("Clear Accounting Process IDs already terminated");
		}
	}
}

int GpuQuery::GetAccountingPids()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if ( (mDevices.at(i).Status == NVML_SUCCESS) && (mDevices.at(i).AccountingMode == NVML_FEATURE_ENABLED) )
		{
    	    mQueryReturn = nvmlDeviceGetAccountingPids(mDevices.at(i).Device, &(mDevices.at(i).ProcessCount), &(mDevices.at(i).ProcessList[0]));
			PrintErrorString("Get Accounting Process IDs");
		}
	}
}

int GpuQuery::GetAccountingStat(int paramIndex)
{
	int i;
	int tCount;
	unsigned int tPID;
	nvmlDevice_t tDevice;
	unsigned int tBufferSize;

	tCount = mDevices.at(paramIndex).ProcessCount;
	tDevice = mDevices.at(paramIndex).Device;

	mQueryReturn = nvmlDeviceGetAccountingBufferSize(tDevice, &tBufferSize);

	cout << "Buffer Size for Device: " << tBufferSize << endl;

	if (tBufferSize < tCount)
		tCount = tBufferSize;

	tCount = 1;

	for (i = 0; i < tCount; i++)
	{
		tPID = mDevices.at(i).ProcessList[i];
   	    mQueryReturn = nvmlDeviceGetAccountingStats(tDevice, tPID, &(mDevices.at(paramIndex).AccountingStatList[i]));
//		PrintErrorString("Get Accounting Process IDs");
	}
}

int GpuQuery::GetAccountingStats()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
	{
		if ( (mDevices.at(i).Status == NVML_SUCCESS) && (mDevices.at(i).AccountingMode == NVML_FEATURE_ENABLED) )
		{
			GetAccountingStat(i);
			PrintErrorString("Get Accounting Stats");
		}
	}
}

float GpuQuery::GetMaxMemoryBytes(int paramIndex)
{
	nvmlAccountingStats_t * tStat;

	tStat = &(mDevices.at(paramIndex).AccountingStatList[0]);

	return (float)(tStat->maxMemoryUsage);
}

float GpuQuery::GetMemUtilization(int paramIndex)
{
	nvmlAccountingStats_t * tStat;

	tStat = &(mDevices.at(paramIndex).AccountingStatList[0]);

	return tStat->memoryUtilization;
}

float GpuQuery::GetGpuUtilization(int paramIndex)
{
	nvmlAccountingStats_t * tStat;

	tStat = &(mDevices.at(paramIndex).AccountingStatList[0]);

	return tStat->gpuUtilization;
}

void GpuQuery::PrintAccountingInfo(int paramIndex)
{
	int i;
	int tCount;
	unsigned int * tPIDs;
	nvmlAccountingStats_t * tStat;

	tCount = mDevices.at(paramIndex).ProcessCount;
	tPIDs  = &(mDevices.at(paramIndex).ProcessList[0]);

	cout << "##### Accounting Process Information #####" << endl;
	cout << "----- Count: " << tCount << endl;
	
	if (tCount < 1)
	{
		cout << "No Process is running" << endl;
		return;
	}
	
	for (i = 0; i < tCount; i++)
	{
		tStat = &(mDevices.at(paramIndex).AccountingStatList[i]);

		cout << "Process No." << (i + 1) << ": " << tPIDs[i] << endl;
		cout << "---------- Is Running: " << tStat->isRunning << endl;
		cout << "--------------- Start: " << tStat->startTime << endl;
		cout << "---------------- Time: " << tStat->time << endl;
		cout << "----------- Mem Usage: " << tStat->maxMemoryUsage << " BYTES" << endl;
		cout << "----- Mem Utilization: " << tStat->memoryUtilization << " %" << endl;
		cout << "----- GPU Utilization: " << tStat->gpuUtilization << " %" << endl;
	}
}

void GpuQuery::PrintDeviceInfo(int paramIndex)
{
	if (mDevices.at(paramIndex).Status != NVML_SUCCESS)
		PrintErrorString("[NVML DEVICE] has failed");
	else
	{
		cout << "================= START =================" << endl;
		cout << "[NVML DEVICE] has done successfully" << endl;
		cout << "---------------- NAME: " << mDevices.at(paramIndex).Name << endl;
		cout << "----------------- PCI: " << mDevices.at(paramIndex).PCI.pciDeviceId << endl;
		if (mDevices.at(paramIndex).AccountingMode == NVML_FEATURE_ENABLED)
		{
			cout << "----- ACCOUNTING MODE: ENABLED" << endl;
			PrintAccountingInfo(paramIndex);
		}
		else
			cout << "----- ACCOUNTING MODE: DISABLED" << endl;


		cout << "================= E N D =================" << endl;
	}
}

void GpuQuery::PrintAllDeviceInfo()
{
	int i;

	for (i = 0; i < mDeviceCount; i++)
		PrintDeviceInfo(i);
}

void GpuQuery::PrintErrorString(const char * paramNote)
{
	if (NVML_SUCCESS != mQueryReturn)
		cout << paramNote << ": " << nvmlErrorString(mQueryReturn) << endl;
	else
		cout << paramNote << endl;
}

const char * GpuQuery::ComputeModeToString(nvmlComputeMode_t paramMode)
{
    switch (paramMode)
    {
        case NVML_COMPUTEMODE_DEFAULT:
            return "Default";
        case NVML_COMPUTEMODE_EXCLUSIVE_THREAD:
            return "Exclusive_Thread";
        case NVML_COMPUTEMODE_PROHIBITED:
            return "Prohibited";
        case NVML_COMPUTEMODE_EXCLUSIVE_PROCESS:
            return "Exclusive Process";
        default:
            return "Unknown";
    }
}

