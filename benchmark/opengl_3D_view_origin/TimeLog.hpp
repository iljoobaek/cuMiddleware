#ifndef __TIME_LOG_HPP__
#define __TIME_LOG_HPP__

#include <time.h>

#define NANO_SEC		1000000000
#define MICRO_SEC		1000000
#define FRAME_ID_CHK	50

class TimeLog
{
private:

	struct timespec mTpStart;
	struct timespec mTpEnd;

	struct timespec mStart10;
	struct timespec mNow10;

	struct timespec mTimePrev;
	struct timespec mTimeCurr;
	struct timespec * mTimePrevPtr;
	struct timespec * mTimeCurrPtr;

	float	mDiffSec, mDiffNanoSec;

	int 	mFrameNumber;
	float 	mTimeSum, mTimeMax;

	int 	mFrameCount;
	int 	mFrameID;
	float 	mFPS;
	int		mNBFrames;

	float 	mTotalNanoSec;

	float 	mLastTime;
	float 	mCurrentTime;

public:
	
	TimeLog();
	~TimeLog();

	void Initialize(float paramTime);
	void CheckFrameStartTime();
	void CheckFrameEndTime();
	void CheckFrame10();
	void CheckNBFrame(float paramTime);
	float GetTimeDiff();

	void PrintFPS();
	void PrintNBFrame();
	void PrintResult();
};

#endif // __TIME_LOG_HPP__

