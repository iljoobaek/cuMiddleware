#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
using namespace std;

#include "TimeLog.hpp"


TimeLog::TimeLog()
{
}

TimeLog::~TimeLog()
{
}

// To run this application with other application at the same time
void TimeLog::CheckBeginTest()
{
    while(1)
    {
        if (FILE *file = fopen("../run_benchmark.txt", "r")) {
            fclose(file);
            break;
        }
        else {
            //cout << "." << endl;
            usleep(100000);
        }
    }
    return;
}

// To run this application with other application at the same time
void TimeLog::CheckEndTest()
{

    if (FILE *file = fopen("../stop_benchmark.txt", "r")) {
        fclose(file);
        PrintResult();
        exit(0);
    }

    return;
}

void TimeLog::Initialize(float paramTime)
{
	mTimeSum = 0.0f;
	mTimeMax = 0.0f;

	mFrameID = 0;
	mNBFrames = 0;

	mFrameNumber = 0;

	mLastTime = paramTime;

    CheckFrameStartTime();

	mTimeCurrPtr = &mTimeCurr;
	mTimePrevPtr = &mTimePrev;
	clock_gettime(CLOCK_MONOTONIC, mTimePrevPtr);
}

void TimeLog::CheckFrameStartTime()
{
    clock_gettime(CLOCK_MONOTONIC, &mTpStart);
}

void TimeLog::CheckFrameEndTime()
{
    clock_gettime(CLOCK_MONOTONIC, &mTpEnd);

    mDiffSec = mTpEnd.tv_sec - mTpStart.tv_sec;
    mDiffNanoSec = mTpEnd.tv_nsec - mTpStart.tv_nsec;
    mTotalNanoSec = mDiffSec * NANO_SEC + mDiffNanoSec;

    if(mFrameID > FRAME_ID_CHK)
    {
        if(mTotalNanoSec > mTimeMax) mTimeMax = mTotalNanoSec;

        mTimeSum += mTotalNanoSec;

        mFrameNumber++;
    }

    CheckEndTest();
}

void TimeLog::CheckFrame10()
{
	//mFrameCount++;

    if (mFrameID == 1)
        CheckBeginTest();

    //if(mFrameCount == 10)
    {
        //mFrameCount = 0;

        clock_gettime(CLOCK_MONOTONIC, &mNow10);

        mDiffSec = mNow10.tv_sec - mStart10.tv_sec;
        mDiffNanoSec = mNow10.tv_nsec - mStart10.tv_nsec;

        mTotalNanoSec = mDiffSec * NANO_SEC + mDiffNanoSec;

        mFPS = 1 / (mTotalNanoSec / NANO_SEC);
		PrintFPS();

        clock_gettime(CLOCK_MONOTONIC, &mStart10);
    }

    mFrameID++;
}

void TimeLog::CheckNBFrame(float paramTime)
{
	mCurrentTime = paramTime;

	mNBFrames++;

	if (mCurrentTime - mLastTime >= 1.0)
	{
		//PrintNBFrame();
		mNBFrames = 0;
		mLastTime += 1.0;
	}
}

float TimeLog::GetTimeDiff()
{
	struct timespec * tTimeTempPtr;

    clock_gettime(CLOCK_MONOTONIC, mTimeCurrPtr);

    mDiffSec = mTimeCurrPtr->tv_sec - mTimePrevPtr->tv_sec;
    mDiffNanoSec = mTimeCurrPtr->tv_nsec - mTimePrevPtr->tv_nsec;

    mTotalNanoSec = mDiffSec * NANO_SEC + mDiffNanoSec;

	tTimeTempPtr = mTimeCurrPtr;
	mTimeCurrPtr = mTimePrevPtr;
	mTimePrevPtr = tTimeTempPtr;

	return mTotalNanoSec;
}

void TimeLog::PrintFPS()
{
    cout << "FPS: " << mFPS << endl;
}

void TimeLog::PrintNBFrame()
{
	cout << mNBFrames << " fps" << endl;
}

void TimeLog::PrintResult()
{
    //cout << mTimeSum << " ns \t" << mFrameNumber << " frames \t" << mTimeSum / mFrameNumber  << " ns \t" << mTimeSum / mFrameNumber / 1000000 << " ms" << endl;
    //cout << mTimeMax << " ns \t" << mTimeMax / MICRO_SEC << " ms" << endl;
    cout << "Ave : " <<  mTimeSum / mFrameNumber / 1000000 << " ms "  <<  1000 / (mTimeSum / mFrameNumber / 1000000) << " fps" << endl;
    cout << "Worst : " <<  mTimeMax / MICRO_SEC << " ms " <<  1000 / (mTimeMax / MICRO_SEC) << " fps" << endl;
}
