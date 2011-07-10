#ifndef _PCILIB_NWL_REGISTERS_H
#define _PCILIB_NWL_REGISTERS_H 

  // DMA
static pcilib_register_description_t nwl_dma_registers[] = {
    {0x4000, 	0, 	32, 	0, 	0x00000011,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_control_and_status",  ""},
    {0x4000, 	0, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_enable",  ""},
    {0x4000, 	1, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_active",  ""},
    {0x4000, 	2, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_pending",  ""},
    {0x4000, 	3, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_interrupt_mode",  ""},
    {0x4000, 	4, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_user_interrupt_enable",  ""},
    {0x4000, 	5, 	1, 	0, 	0x00000011,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_user_interrupt_active",  ""},
    {0x4000, 	16, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_s2c_interrupt_status",  ""},
    {0x4000, 	24, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_c2s_interrupt_status",  ""},
    {0x8000, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_design_version",  ""},
    {0x8000, 	0, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_subversion_number",  ""},
    {0x8000, 	4, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_version_number",  ""},
    {0x8000, 	28, 	4, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_targeted_device",  ""},
    {0x8200, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_transmit_utilization",  ""},
    {0x8200, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_transmit_sample_count",  ""},
    {0x8200, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_transmit_dword_count",  ""},
    {0x8204, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_receive_utilization",  ""},
    {0x8004, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_receive_sample_count",  ""},
    {0x8004, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_receive_dword_count",  ""},
    {0x8208, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_mwr",  ""},
    {0x8008, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_mwr_sample_count",  ""},
    {0x8008, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_mwr_dword_count",  ""},
    {0x820C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_cpld",  ""},
    {0x820C, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_cpld_sample_count",  ""},
    {0x820C, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma_cpld_dword_count",  ""},
    {0x8210, 	0, 	12, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_cpld",  ""},
    {0x8214, 	0, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_cplh",  ""},
    {0x8218, 	0, 	12, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_npd",  ""},
    {0x821C, 	0, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_nph",  ""},
    {0x8220, 	0, 	12, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_pd",  ""},
    {0x8224, 	0, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma_init_fc_ph",  ""},
    {0,		0,	0,	0,	0x00000000,	0,                                           0,                        0, NULL, NULL}
};

 // DMA Engine Registers
#define NWL_MAX_DMA_ENGINE_REGISTERS 64
#define NWL_MAX_REGISTER_NAME 128
static char nwl_dma_engine_register_names[PCILIB_MAX_DMA_ENGINES * NWL_MAX_DMA_ENGINE_REGISTERS][NWL_MAX_REGISTER_NAME];
static pcilib_register_description_t nwl_dma_engine_registers[] = {
    {0x0000, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_engine_capabilities",  ""},
    {0x0000, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_present",  ""},
    {0x0000, 	1, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_direction",  ""},
    {0x0000, 	4, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_type",  ""},
    {0x0000, 	8, 	8, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_number",  ""},
    {0x0000, 	24, 	6, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_max_buffer_size",  ""},
    {0x0004, 	0, 	32, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_engine_control",  ""},
    {0x0004, 	0, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_interrupt_enable",  ""},
    {0x0004, 	1, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_interrupt_active",  ""},
    {0x0004, 	2, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_descriptor_complete",  ""},
    {0x0004, 	3, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_descriptor_alignment_error",  ""},
    {0x0004, 	4, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_descriptor_fetch_error",  ""},
    {0x0004, 	5, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW1C, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_sw_abort_error",  ""},
    {0x0004, 	8, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_enable",  ""},
    {0x0004, 	9, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_running",  ""},
    {0x0004, 	10, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_waiting",  ""},
    {0x0004, 	14, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_reset_request", ""},
    {0x0004, 	15, 	1, 	0, 	0x0000C100,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_reset", ""},
    {0x0008, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_next_descriptor",  ""},
    {0x000C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_sw_descriptor",  ""},
    {0x0010, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_last_descriptor",  ""},
    {0x0014, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_active_time",  ""},
    {0x0018, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_wait_time",  ""},
    {0x001C, 	0, 	32, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_counter",  ""},
    {0x001C, 	0, 	2, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_sample_count",  ""},
    {0x001C, 	2, 	30, 	0, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "dma%0*u%s_dword_count",  ""},
    {0,		0,	0,	0,	0x00000000,	0,                                          0,                        0, NULL, NULL}
};

/*
 // XAUI registers
static pcilib_register_description_t nwl_xaui_registers[] = {
    {0,		0,	0,	0,	0,                                          0,                        0, NULL, NULL}
};
*/

 // XRAWDATA registers
static pcilib_register_description_t nwl_xrawdata_registers[] = {
    {0x9100, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_enable_generator",  ""},
    {0x9104, 	0, 	16, 	0, 	0x00000000,	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_packet_length",  ""},
    {0x9108, 	0, 	2, 	0, 	0x00000003,	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_control",  ""},
    {0x9108, 	0, 	1, 	0, 	0x00000003,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "xrawdata_enable_checker",  ""},
    {0x9108, 	1, 	1, 	0, 	0x00000003,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS, PCILIB_REGISTER_BANK_DMA, "xrawdata_enable_loopback",  ""},
    {0x910C, 	0, 	1, 	0, 	0x00000000,	PCILIB_REGISTER_R , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_DMA, "xrawdata_data_mistmatch",  ""},
    {0,		0,	0,	0,	0x00000000,	0,                                            0,                        0, NULL, NULL}
};

int nwl_add_registers(nwl_dma_t *ctx);
#endif /* _PCILIB_NWL_REGISTERS_H */
