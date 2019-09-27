#include <stdio.h> //fprintf, sprintf
#include <unistd.h> // sleep()
#include <sys/types.h> // pid_t
#include "tag_frame.h" // FrameController
#include "tag_gpu.h"

int main(int argc, char **argv) {

	pid_t pid = getpid();
	pid_t tid = gettid();
	const char *job_name = "opencv_get_image2";
	int64_t frame_period_us = 500;
	int64_t deadline_us = 500;
	for (int i = 0; i < 10000; i++)
	{
		fprintf(stdout, "opencv: tag_beginning() %d\n", i);
		// Tagging begin ///////////////////////////////////////////////////////////////////
		char j_name[80];
		sprintf(j_name, "%s:%d", job_name, i);
		int res = tag_job_begin(pid, tid, j_name, 
				frame_period_us, deadline_us,
				14L, false, true, 1UL);

        usleep(10000);

		// Tag_end /////////////////////////////////////////////////////////////////////////
		tag_job_end(pid, tid, j_name);

		fprintf(stdout, "opencv_get_image: onto next job in 1s\n");
		usleep(10000);
		deadline_us += 20000;
	}
}
