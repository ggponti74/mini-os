#ifndef _SOUND_H_
#define _SOUND_H_

#include "display.h"

#define SB16_REG_IN_SWITCH_L 0x3D // Input Switch Left Channel
#define SB16_REG_IN_SWITCH_R 0x3E // Input Switch Right Channel
#define SB16_REG_GAIN_CTRL 0x3F   // Master Gain Control (Bits 7-6: L Gain, Bits 5-4: R Gain)

void sb16_mixer_write(unsigned char reg, unsigned char data);
void set_master_switch(unsigned char mask);
unsigned char get_master_switch(void);
void set_master_volume(unsigned char left, unsigned char right);
void set_input_switches(unsigned char left_mask, unsigned char right_mask);
void set_master_gain(unsigned char gain_left, unsigned char gain_right);

#endif
