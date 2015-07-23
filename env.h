#ifndef _IPECAMERA_ENV_H
#define _IPECAMERA_ENV_H

typedef enum {
    IPECAMERA_DEBUG_RAW_FRAMES_ENV,
    IPECAMERA_DEBUG_BROKEN_FRAMES_ENV,
    IPECAMERA_DEBUG_RAW_PACKETS_ENV,
    IPECAMERA_DEBUG_HARDWARE_ENV,
    IPECAMERA_DEBUG_FRAME_HEADERS_ENV,
    IPECAMERA_MAX_ENV
} ipecamera_env_t;

#ifdef __cplusplus
extern "C" {
#endif

const char *ipecamera_getenv(ipecamera_env_t env, const char *var);

#ifdef __cplusplus
}
#endif

#endif /* _IPECAMERA_ENV_H */
