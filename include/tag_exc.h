#ifndef TAG_EXC_H
#define TAG_EXC_H

#ifdef __cplusplus
#include <iostream>
#include <exception>

// TODO: Move impl. to tag_exc.cpp

// Non-fatal exception when frame-job falls behind deadline
struct DroppedFrameException: public std::exception {
	const char * what () const throw () {
    	return "Dropping a frame! (non-fatal error)";
    }
}

// Fatal exception when job must be aborted
struct AbortJobException: public std::exception {
	const char * what () const throw () {
    	return "Job not allowed to run! (fatal error!)";
    }
}
#endif /* __cplusplus */

#endif /* TAG_EXC_H */
