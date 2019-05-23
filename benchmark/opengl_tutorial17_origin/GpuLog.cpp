#include "GpuLog.hpp"

GpuLog::GpuLog()
{
}

GpuLog::~GpuLog()
{
	fileBytes.close();
	fileMemUtil.close();
	fileGpuUtil.close();
	fileTimeDiff.close();
}

void GpuLog::Initialize()
{
	fileBytes.open("max_mem_bytes.log");
	fileMemUtil.open("memory_utilization.log");
	fileGpuUtil.open("gpu_utilization.log");	
	fileTimeDiff.open("frame_timediff.log");
}

void GpuLog::WriteLogs(float paramValue, const char * paramTag, int paramFile)
{
	switch (paramFile)
	{
		case 1:	// maximum memory used (bytes)
			fileBytes << (int)paramValue << ", " << paramTag << endl;
			break;
		case 2: // memory utilization ratio
			fileMemUtil << paramValue <<  ", " << paramTag << endl;
			break;
		case 3: // GPU utilization ratio
			fileGpuUtil << paramValue <<  ", " << paramTag << endl;
			break;
		case 4: // Time Difference
			fileTimeDiff << (long)paramValue <<  ", " << paramTag << endl;
			break;
	}
}

