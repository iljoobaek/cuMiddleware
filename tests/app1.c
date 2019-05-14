#include <stdio.h> //fprintf
#include <unistd.h> // sleep()
#include "tag_gpu.h" 
int main(int argc, char *argv){ 

	for (int i = 0; i < 10; i++)
	{
		fprintf(stdout, "opengl_get_image: tag_beginning()\n");
		// Tagging begin ///////////////////////////////////////////////////////////////////
		const char *task_name = "opengl_get_image";
		tag_begin(1, task_name, 0L, 0L, 0L, 0.0, 0.0, 0.0, 0, 0);

		// Tag_end /////////////////////////////////////////////////////////////////////////
		tag_end(1, task_name);

		fprintf(stdout, "opengl_get_image: onto next job in 1s\n");
		sleep(1);
	}
}
