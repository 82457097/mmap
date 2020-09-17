#ifndef HOOK_BASE
#define HOOK_BASE
 
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
 
#include <dlfcn.h>
 
#define HOOK_FUNC_TEMPLATE(function_name) function_name##_func_t
 
#define HOOK_FUNC_ORI_NAME(function_name) function_name##_ori
 
#define HOOK_FUNC_INIT(function_name) static HOOK_FUNC_TEMPLATE(function_name) HOOK_FUNC_ORI_NAME(function_name);
 
#define HOOK_FUNC(function_name) \
	    if (!HOOK_FUNC_ORI_NAME(function_name)) {	\
			HOOK_FUNC_ORI_NAME(function_name) = (HOOK_FUNC_TEMPLATE(function_name)) dlsym(RTLD_NEXT, #function_name);\
		}\

#define ORIGINAL_FUNC(function_name) ((HOOK_FUNC_TEMPLATE(function_name)) HOOK_FUNC_ORI_NAME(function_name))
							 
#endif
