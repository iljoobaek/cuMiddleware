#include <stdio.h> //fprintf
#include <unistd.h> // sleep()
#include <sys/types.h> // pid_t
#include "tag_frame.h" // FrameController
#include "tag_gpu.h"

int main(int argc, char **argv) {

	pid_t pid = getpid();
	pid_t tid = gettid();
	const char *job_name = "opencv_get_image";
	int64_t frame_period_us = 500000;
	int64_t deadline_us = 100;
	bool use_slack = false;
	bool shareable = false;
	for (int i = 0; i < 10; i++)
	{
		fprintf(stdout, "opencv: tag_beginning() %d\n", i);
		// Tagging begin ///////////////////////////////////////////////////////////////////
		int res = tag_job_begin(pid, tid, job_name, 
				frame_period_us, deadline_us,
				0L, use_slack, shareable, 0UL);

        usleep(500000);
		// Tag_end /////////////////////////////////////////////////////////////////////////
		tag_job_end(pid, tid, job_name);

		fprintf(stdout, "opencv_get_image: onto next job in 1s\n");
		sleep(1);
	}
}
