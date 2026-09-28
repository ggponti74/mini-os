#include "sound.h"
#include "display.h"
#include "io.h"

#define SB16_MIXER_ADDR 0x224
#define SB16_MIXER_DATA 0x225

#define SB16_REG_MASTER_VOL 0x22 // Master Volume (Left/Right)
#define SB16_REG_MASTER_L 0x30   // Master Volume Left
#define SB16_REG_MASTER_R 0x31   // Master Volume Right

#define SB16_REG_OUTPUT_SWITCH 0x3C // Output / Master Switch Register

// Output Switch Bit Masks
#define SB16_OUT_VOICE_L (1 << 0)
#define SB16_OUT_MIDI_L (1 << 1)
#define SB16_OUT_MIDI_R (1 << 2)
#define SB16_OUT_CD_L (1 << 3)
#define SB16_OUT_CD_R (1 << 4)
#define SB16_OUT_LINE_L (1 << 5)
#define SB16_OUT_LINE_R (1 << 6)

// Write a value to an SB16 mixer register
void sb16_mixer_write(unsigned char reg, unsigned char data)
{
    outb(SB16_MIXER_ADDR, reg);
    outb(SB16_MIXER_DATA, data);
}

// Set Output Switch State (Enable/Disable routing mask)
void set_master_switch(unsigned char mask)
{
    sb16_mixer_write(SB16_REG_OUTPUT_SWITCH, mask);
}

// Read a value from an SB16 mixer register
unsigned char sb16_mixer_read(unsigned char reg)
{
    outb(SB16_MIXER_ADDR, reg);
    return inb(SB16_MIXER_DATA);
}

// Read current Output Switch state
unsigned char get_master_switch(void)
{
    return sb16_mixer_read(SB16_REG_OUTPUT_SWITCH);
}


// Set Master Volume (vol_left: 0-15, vol_right: 0-15)
void set_master_volume(unsigned char left, unsigned char right)
{
    // Clamp values to 4-bit range (0x0 to 0xF)
    left &= 0x0F;
    right &= 0x0F;

    // Combine into single byte [Left: 7-4, Right: 3-0]
    unsigned char val = (left << 4) | right;

    sb16_mixer_write(SB16_REG_MASTER_VOL, val);
}

// Set Input Channel Routing (Left & Right)
void set_input_switches(unsigned char left_mask, unsigned char right_mask)
{
    sb16_mixer_write(SB16_REG_IN_SWITCH_L, left_mask);
    sb16_mixer_write(SB16_REG_IN_SWITCH_R, right_mask);
}

// Set Gain Levels (gain_left: 0-3, gain_right: 0-3)
void set_master_gain(unsigned char gain_left, unsigned char gain_right)
{
    gain_left &= 0x03;
    gain_right &= 0x03;
    unsigned char val = (gain_left << 6) | (gain_right << 4);
    sb16_mixer_write(SB16_REG_GAIN_CTRL, val);
}
