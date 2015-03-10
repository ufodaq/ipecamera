#ifndef _IPECAMERA_MODEL_H
#define _IPECAMERA_MODEL_H

#include <stdio.h>

#include "../pcilib.h"
#include "public.h"

//#define IPECAMERA_DEBUG

#define IPECAMERA_DMA_R3
#define IPECAMERA_DMA_ADDRESS 0
#define IPECAMERA_DMA_PACKET_LENGTH 4096

//#define IPECAMERA_REGISTER_SPACE 0xfeaffc00
#define IPECAMERA_REGISTER_SPACE 0x9000
#define IPECAMERA_REGISTER_WRITE (IPECAMERA_REGISTER_SPACE + 0)
#define IPECAMERA_REGISTER_READ (IPECAMERA_REGISTER_WRITE + 16)

#ifdef _IPECAMERA_MODEL_C
pcilib_register_bank_description_t ipecamera_register_banks[] = {
    { PCILIB_REGISTER_BANK0,    PCILIB_BAR0,    128, IPECAMERA_REGISTER_PROTOCOL, IPECAMERA_REGISTER_READ , IPECAMERA_REGISTER_WRITE, PCILIB_LITTLE_ENDIAN, 8 , PCILIB_LITTLE_ENDIAN, "%lu"  , "cmosis", "CMOSIS CMV2000 Registers" },
    { PCILIB_REGISTER_BANK1,    PCILIB_BAR0, 0x0200, PCILIB_DEFAULT_PROTOCOL    , IPECAMERA_REGISTER_SPACE, IPECAMERA_REGISTER_SPACE, PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "0x%lx", "fpga", "IPECamera Registers" },
//    { PCILIB_REGISTER_BANK_DMA, PCILIB_BAR0, 0xA000, PCILIB_DEFAULT_PROTOCOL    , 0,                        0,                        PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "0x%lx", "dma", "DMA Registers"},
    { PCILIB_REGISTER_BANK_DMA, PCILIB_BAR0, 0x0200, PCILIB_DEFAULT_PROTOCOL    , 0,                        0,                    PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "0x%lx", "dma", "DMA Registers"},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

pcilib_register_description_t ipecamera_registers[] = {
{1, 	0, 	16, 	1088, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines",  ""},
{3, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start1", ""},
{5, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start2", ""},
{7, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start3", ""},
{9, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start4", ""},
{11,	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start5", ""},
{13, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start6", ""},
{15, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start7", ""},
{17, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_start8", ""},
{19, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines1", ""},
{21, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines2", ""},
{23, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines3", ""},
{25, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines4", ""},
{27, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines5", ""},
{29, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines6", ""},
{31, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines7", ""},
{33, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_lines8", ""},
{35, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_sub_s", ""},
{37, 	0, 	16, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_sub_a", ""},
{39, 	0, 	1, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_color", ""},
{40, 	0, 	2, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_image_flipping", ""},
{41, 	0, 	2, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_flags", ""},
{42, 	0, 	24, 	1088, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_time", ""},
{45, 	0, 	24, 	1088, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_step", ""},
{48, 	0, 	24, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_kp1", ""},
{51, 	0, 	24, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_kp2", ""},
{54, 	0, 	2, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_nr_slopes", ""},
{55, 	0, 	8, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_seq", ""},
{56, 	0, 	24, 	1088, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_time2", ""},
{59, 	0, 	24, 	1088, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_step2", ""},
{68, 	0, 	2, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_nr_slopes2", ""},
{69, 	0, 	8, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_exp_seq2", ""},
{70, 	0, 	16, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_number_frames", ""},
{72, 	0, 	2, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_output_mode", ""},
{78, 	0, 	12, 	85, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_training_pattern", ""},
{80, 	0, 	18, 	0x3FFFF,0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_channel_en", ""},
{82, 	0, 	3, 	7, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_special_82", ""},
{89, 	0, 	8, 	96, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_vlow2", ""},
{90, 	0, 	8, 	96, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_vlow3", ""},
{100, 	0, 	14, 	16260, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_offset", ""},
{102, 	0, 	2, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_pga", ""},
{103, 	0, 	8, 	32, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_adc_gain", ""},
{111, 	0, 	1, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_bit_mode", ""},
{112, 	0, 	2, 	0, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_adc_resolution", ""},
{115, 	0, 	1, 	1, 	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "cmosis_special_115", ""},
/*{126, 	0, 	16, 	0,                        	PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK0, "temp", ""},*/
{0x00,	0, 	32,	0,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "spi_conf_input", ""},
{0x10,	0, 	32,	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "spi_conf_output", ""},
{0x20,	0, 	32,	0,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "spi_clk_speed", ""},
{0x30,	0, 	32,	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "firmware_info", ""},
{0x30, 	0, 	8, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "firmware_version",  ""},
{0x30, 	8, 	1, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "firmware_bitmode",  ""},
{0x30, 	12, 	2, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "adc_resolution",  ""},
{0x30, 	16, 	2, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "output_mode",  ""},
{0x40,	0, 	32, 	0,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "control", ""},
{0x50,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "status", ""},
{0x54,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "status2", ""},
{0x58,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "status3", ""},
{0x5c,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_status", ""},
{0x70,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "start_address", ""},
{0x74,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "end_address", ""},
{0x78,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "rd_address", ""},
{0xa0,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_param1", ""},
{0xa0, 	0, 	10, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fr_skip_lines",  ""},
{0xa0, 	10, 	11, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fr_num_lines",  ""},
{0xa0, 	21, 	11, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fr_start_address",  ""},
{0xb0,	0, 	32, 	0,	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_param2", ""},
{0xb0, 	0, 	11, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fr_threshold_start_line",  ""},
{0xb0, 	16, 	10, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fr_area_lines",  ""},
{0xc0,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "skiped_lines", ""},
{0xd0,	0, 	32, 	0,	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_thresholds", ""},
{0xd0,	0, 	10, 	0,	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_pixel_thr", ""},
{0xd0,	10, 	11, 	0,	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_num_pixel_thr", ""},
{0xd0,	21, 	11, 	0,	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "fr_num_lines_thr", ""},
{0x100,	0, 	32, 	0,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "rawdata_pkt_addr", ""},
{0x110,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "temperature_info", ""},
{0x110,	0, 	16, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "sensor_temperature",  ""},
{0x110,	16, 	3, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "sensor_temperature_alarms",  ""},
{0x110,	19, 	10, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fpga_temperature",  ""},
{0x110,	29, 	3, 	0, 	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "fpga_temperature_alarms",  ""},
{0x120,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "num_lines", ""},
{0x130,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "start_line", ""},
{0x140,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "exp_time", ""},
{0x150,	0, 	32, 	0,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "motor", ""},
{0x150,	0, 	5, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "motor_phi",  ""},
{0x150,	5, 	5, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "motor_z",  ""},
{0x150,	10, 	5, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "motor_y",  ""},
{0x150,	15, 	5, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_RW, PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "motor_x",  ""},
{0x150,	20, 	8, 	0, 	PCILIB_REGISTER_ALL_BITS, PCILIB_REGISTER_R,  PCILIB_REGISTER_BITS,     PCILIB_REGISTER_BANK1, "adc_gain",  ""},
{0x160,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "write_status", ""},
{0x170,	0, 	32, 	0,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "num_triggers", ""},
{0x180,	0, 	32, 	0x280,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "trigger_period", ""},
{0x190,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "temperature_sample_period", ""},
{0x1a0,	0, 	32, 	0x64,	0,                        PCILIB_REGISTER_RW, PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "ddr_max_frames", ""},
{0x1b0,	0, 	32, 	0,	0,                        PCILIB_REGISTER_R,  PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK1, "ddr_num_frames", ""},
{0,	0,	0,	0,	0,                        0,                  0,                        0,                     NULL, NULL}
};

pcilib_register_range_t ipecamera_register_ranges[] = {
//    {0xF000, 	0xF000 + 128, 	PCILIB_REGISTER_BANK0, 0},
//    {0x9000,	0x9FFF,	PCILIB_REGISTER_BANK1, -0x9000},
    {0, 0, 0, 0}
};

pcilib_event_description_t ipecamera_events[] = {
    {PCILIB_EVENT0, "new_frame", ""},
    {0, NULL, NULL}
};

pcilib_event_data_type_description_t ipecamera_data_types[] = {
    {IPECAMERA_IMAGE_DATA, PCILIB_EVENT0, "image", "16 bit pixel data" },
    {IPECAMERA_RAW_DATA, PCILIB_EVENT0, "raw", "raw data from camera" },
    {IPECAMERA_CHANGE_MASK, PCILIB_EVENT0, "cmask", "change mask" },
    {0, 0, NULL, NULL}
};

#else
extern pcilib_register_description_t ipecamera_registers[];
extern pcilib_register_bank_description_t ipecamera_register_banks[];
extern pcilib_register_range_t ipecamera_register_ranges[];
extern pcilib_event_description_t ipecamera_events[];
extern pcilib_event_data_type_description_t ipecamera_data_types[];
#endif 

#ifdef _IPECAMERA_IMAGE_C
pcilib_event_api_description_t ipecamera_image_api = {
    "ipecamera",
    
    ipecamera_init,
    ipecamera_free,

    ipecamera_init_dma,

    ipecamera_reset,
    ipecamera_start,
    ipecamera_stop,
    ipecamera_trigger,
    
    ipecamera_stream,
    ipecamera_next_event,
    ipecamera_get,
    ipecamera_return
};
#else
extern pcilib_event_api_description_t ipecamera_image_api;
#endif

int ipecamera_read(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
int ipecamera_write(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);

#endif /* _IPECAMERA_MODEL_H */
