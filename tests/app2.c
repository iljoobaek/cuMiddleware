#include <stdio.h> //fprintf
#include <unistd.h> // sleep()
#include "tag_gpu.h" 
int main(int argc, char *argv){ 

	for (int i = 0; i < 10; i++)
	{
		fprintf(stdout, "opencv_draw: tag_beginning()\n");
		// Tagging begin ///////////////////////////////////////////////////////////////////
		const char *task_name = "opencv_draw";
		tag_begin(2, task_name, 0L, 0L, 0L, 0.0, 0.0, 0.0, 0, 0);

		// Tag_end /////////////////////////////////////////////////////////////////////////
		tag_end(2, task_name);

		fprintf(stdout, "opencv_draw: onto next job in 1s\n");
		sleep(1);
	}
}
