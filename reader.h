#ifndef _IPECAMERA_READER_H
#define _IPECAMERA_READER_H

int ipecamera_compute_buffer_size(ipecamera_t *ctx, size_t lines);

void *ipecamera_reader_thread(void *user);

#endif /* _IPECAMERA_READER_H */
