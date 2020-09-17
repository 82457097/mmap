#ifndef HOOK_DEF
#define HOOK_DEF
 
#include "hook_base.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
 
typedef uid_t (*HOOK_FUNC_TEMPLATE(getuid))(void);
#endif
