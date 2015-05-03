#include <stdio.h>
#include <stdlib.h>

#include <pcilib.h>
#include <pcilib/error.h>

#include <ipecamera.h>

void log_error(void *arg, const char *file, int line, pcilib_log_priority_t prio, const char *format, va_list ap) {
    vprintf(format, ap);
    printf("\n");

    if (prio == PCILIB_LOG_ERROR) {
	printf("Exiting at [%s:%u]\n\n", file, line);
	exit(-1);
    }
}


int main() {
    int err;
    pcilib_event_id_t evid;
    ipecamera_event_info_t info;
    ipecamera_t *ipecamera;
    size_t size;
    void *data;
    FILE *f;

    pcilib_set_logger(PCILIB_LOG_WARNING, &log_error, NULL);

    pcilib_t *pcilib = pcilib_open("/dev/fpga0", "ipecamera");
    if (!pcilib) pcilib_error("Error opening device");

    ipecamera = (ipecamera_t*)pcilib_get_event_engine(pcilib);
    if (!ipecamera) pcilib_error("Failed to get ipecamera event engine");

    err = ipecamera_set_buffer_size(ipecamera, 8);
    if (err) pcilib_error("Error (%i) setting buffer size", err);

    err = pcilib_start(pcilib, PCILIB_EVENTS_ALL, PCILIB_EVENT_FLAGS_DEFAULT);
    if (err) pcilib_error("Error (%i) starting event engine", err);

    err = pcilib_trigger(pcilib, PCILIB_EVENT0, 0, NULL);
    if (err) pcilib_error("Error (%i) triggering event", err);

    err = pcilib_get_next_event(pcilib, 100000, &evid, sizeof(info), (pcilib_event_info_t*)&info);
    if (err) pcilib_error("Error (%i) while waiting for event", err);

    data = pcilib_get_data(pcilib, evid, PCILIB_EVENT_DATA, &size);
    if (!data) pcilib_error("Error getting event data");

    printf("Writting %zu bytes to /dev/null\n", size);
    f = fopen("/dev/null", "w");
    if (f) {
	fwrite(data, 1, size, f);
	fclose(f);
    }

    err = pcilib_return_data(pcilib, evid, PCILIB_EVENT_DATA, data);
    if (err) pcilib_error("Error returning data, data is possibly corrupted");

    pcilib_stop(pcilib, PCILIB_EVENT_FLAGS_DEFAULT);
}
