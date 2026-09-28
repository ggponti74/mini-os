// kernel/main.c

#define VGA_MEMORY (volatile char *)0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Default attribute: Light Grey text on Black background
#define COLOR_DEFAULT 0x07
// Highlight attribute: Bright White text on Black background
#define COLOR_WHITE 0x0F

static int cursor_x = 0;
static int cursor_y = 0;

// Write a byte to an I/O port
static inline void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a byte from an I/O port
static inline unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Clear the full VGA text screen
void clear_screen(void)
{
    volatile char *vga = VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2)
    {
        vga[i] = ' ';
        vga[i + 1] = COLOR_DEFAULT;
    }
    cursor_x = 0;
    cursor_y = 0;
}

// Print a single character with automatic newline and scrolling support
void kputchar(char c, char color)
{
    volatile char *vga = VGA_MEMORY;

    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else
    {
        int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        vga[offset] = c;
        vga[offset + 1] = color;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH)
        {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // Basic vertical bounds check (reset to top if we hit the bottom row)
    if (cursor_y >= VGA_HEIGHT)
    {
        cursor_y = 0;
    }
}

// Print a null-terminated string
void kprint(const char *str, char color)
{
    int i = 0;
    while (str[i] != '\0')
    {
        kputchar(str[i], color);
        i++;
    }
}

// Sound

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

// kernel/main.c

#define SB16_REG_IN_SWITCH_L 0x3D // Input Switch Left Channel
#define SB16_REG_IN_SWITCH_R 0x3E // Input Switch Right Channel
#define SB16_REG_GAIN_CTRL 0x3F   // Master Gain Control (Bits 7-6: L Gain, Bits 5-4: R Gain)



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

// Main
void kernel_main(void)
{
    clear_screen();

    kprint("mini-os kernel 0.1\n", COLOR_WHITE);
    kprint("------------------\n", COLOR_DEFAULT);

    // 1. m-VR: Set Master Volume to Maximum (Left: 0xF, Right: 0xF)
    set_master_volume(0x0F, 0x0F);

    // 2. m-S: Enable Master Output Switch for Voice and MIDI
    set_master_switch(SB16_OUT_VOICE_L | SB16_OUT_MIDI_L | SB16_OUT_MIDI_R);

    // 3. S: Set Master Gain to x1 (Left: 0, Right: 0) or x2 (Left: 1, Right: 1)
    set_master_gain(0x01, 0x01);

    // Read back values to verify CT1745 register state
    unsigned char vol = sb16_mixer_read(SB16_REG_MASTER_VOL);
    unsigned char msw = get_master_switch();
    unsigned char gain = sb16_mixer_read(SB16_REG_GAIN_CTRL);

    kprint("m-VR  (0x22): 0x", COLOR_DEFAULT);
    kputchar("0123456789ABCDEF"[(vol >> 4) & 0x0F], COLOR_WHITE);
    kputchar("0123456789ABCDEF"[vol & 0x0F], COLOR_WHITE);
    kputchar('\n', COLOR_DEFAULT);

    kprint("m-S   (0x3C): 0x", COLOR_DEFAULT);
    kputchar("0123456789ABCDEF"[(msw >> 4) & 0x0F], COLOR_WHITE);
    kputchar("0123456789ABCDEF"[msw & 0x0F], COLOR_WHITE);
    kputchar('\n', COLOR_DEFAULT);

    kprint("S Gain(0x3F): 0x", COLOR_DEFAULT);
    kputchar("0123456789ABCDEF"[(gain >> 4) & 0x0F], COLOR_WHITE);
    kputchar("0123456789ABCDEF"[gain & 0x0F], COLOR_WHITE);
    kputchar('\n', COLOR_DEFAULT);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}
