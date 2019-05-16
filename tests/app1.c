#include <stdio.h> //fprintf
#include <unistd.h> // sleep()
#include "tag_gpu.h"
int main(int argc, char *argv){

	for (int i = 0; i < 10000; i++)
	{
		fprintf(stdout, "opencv: tag_beginning() %d\n", i);
		// Tagging begin ///////////////////////////////////////////////////////////////////
		const char *task_name = "opencv_get_image";
		tag_begin(1, task_name, 0L, 0L, 0L, 0.0, 0.0, 0.0, 0, 0);

        usleep(1000);

		// Tag_end /////////////////////////////////////////////////////////////////////////
		tag_end(1, task_name);

		fprintf(stdout, "opencv_get_image: onto next job in 1s\n");
		usleep(1000);
	}
}
