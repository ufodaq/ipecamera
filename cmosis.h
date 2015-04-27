#ifndef _IPECAMERA_CMOSIS_H
#define _IPECAMERA_CMOSIS_H

#include <pcilib/bank.h>

int ipecamera_cmosis_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
int ipecamera_cmosis_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);

#endif /* _IPECAMERA_CMOSIS_H */
