#include <stdio.h>
#include <stdlib.h>
#include "env.h"


static const char *env_cache[IPECAMERA_MAX_ENV] = {0};

const char *ipecamera_getenv(ipecamera_env_t env, const char *var) {
    if (!env_cache[env]) {
	const char *var_env = getenv(var);
	env_cache[env] = var_env?var_env:(void*)-1;
	return var_env;
    }

    return (env_cache[env] == (void*)-1)?NULL:env_cache[env];
}

