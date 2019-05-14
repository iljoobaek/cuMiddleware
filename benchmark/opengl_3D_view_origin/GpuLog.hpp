#ifndef __GPU_LOG_HPP__
#define __GPU_LOG_HPP__

#include <iostream>
#include <fstream>
using namespace std;

class GpuLog
{
private:

	ofstream fileBytes;
	ofstream fileMemUtil;
	ofstream fileGpuUtil;
	ofstream fileTimeDiff;

public:
	
	GpuLog();
	~GpuLog();

	void Initialize();
	void WriteLogs(float paramValue, const char * paramTag, int paramFile);
};

#endif // __GPU_LOG_HPP__

