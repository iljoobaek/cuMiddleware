#include <stdio.h> //fprintf
#include <unistd.h> // sleep()
#include <sys/types.h> // pid_t
#include "tag_frame.h" // FrameController
#include "tag_gpu.h"

int main(int argc, char **argv) {

	pid_t pid = getpid();
	pid_t tid = gettid();
	const char *job_name = "opencv_get_image";
	int64_t frame_period_us = 500;
	int64_t deadline_us = 100;
	for (int i = 0; i < 10; i++)
	{
		fprintf(stdout, "opencv: tag_beginning() %d\n", i);
		// Tagging begin ///////////////////////////////////////////////////////////////////
		int res = tag_job_begin(pid, tid, job_name, 
				frame_period_us, deadline_us,
				15L, false, true, 1UL);

        usleep(50000);
		// Tag_end /////////////////////////////////////////////////////////////////////////
		tag_job_end(pid, tid, job_name);
		deadline_us += 50000;

		fprintf(stdout, "opencv_get_image: onto next job in 1s\n");
		sleep(1);
	}
}
