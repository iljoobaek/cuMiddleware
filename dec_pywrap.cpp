#include "tag_dec.h"

extern "C"
{
	void *CreateDecoratorObj(void *fn, meta_job_t *init_meta);
	void DeleteDecoratorObj(void *dec_obj);
	void *CallDecorator(void *dec_obj, int argc, ...);
	void *tag_decorator(void *fn, meta_job_t *init_meta);

	void *CreateDecoratorObj(void *fn, meta_job_t *init_meta) {
		return new(std::nothrow) TagDecorator
	}
	void DeleteDecoratorObj(void *dec_obj);
	void *CallDecorator(void *dec_obj, int argc, ...);
}	
