#include "utils.h"
#include "font.h"
#include "mbox.h"
#include "../uart/uart1.h"

// Screen info
unsigned int width, height, pitch;
/* Frame buffer address
 * (declare as pointer of unsigned char to access each byte) */
unsigned char *fb;

/**
 * Set screen resolution to 1024x768
 */
void framebfInit()
{
    mBuf[0] = 35 * 4; // Length of message in bytes
    mBuf[1] = MBOX_REQUEST;
    mBuf[2] = MBOX_TAG_SETPHYWH;  // Set physical width-height
    mBuf[3] = 8;                  // Value size in bytes
    mBuf[4] = 0;                  // REQUEST CODE = 0
    mBuf[5] = SCREEN_WIDTH;       // Value(width)
    mBuf[6] = SCREEN_HEIGHT;      // Value(height)
    mBuf[7] = MBOX_TAG_SETVIRTWH; // Set virtual width-height
    mBuf[8] = 8;
    mBuf[9] = 0;
    mBuf[10] = 2000;
    mBuf[11] = 2000;
    mBuf[12] = MBOX_TAG_SETVIRTOFF; // Set virtual offset
    mBuf[13] = 8;
    mBuf[14] = 0;
    mBuf[15] = 0;                 // x offset
    mBuf[16] = 0;                 // y offset
    mBuf[17] = MBOX_TAG_SETDEPTH; // Set color depth
    mBuf[18] = 4;
    mBuf[19] = 0;
    mBuf[20] = COLOR_DEPTH;         // Bits per pixel
    mBuf[21] = MBOX_TAG_SETPXLORDR; // Set pixel order
    mBuf[22] = 4;
    mBuf[23] = 0;
    mBuf[24] = PIXEL_ORDER;
    mBuf[25] = MBOX_TAG_GETFB; // Get frame buffer
    mBuf[26] = 8;
    mBuf[27] = 0;
    mBuf[28] = 16;                // alignment in 16 bytes
    mBuf[29] = 0;                 // will return Frame Buffer size in bytes
    mBuf[30] = MBOX_TAG_GETPITCH; // Get pitch
    mBuf[31] = 4;
    mBuf[32] = 0;
    mBuf[33] = 0; // Will get pitch value here
    mBuf[34] = MBOX_TAG_LAST;
    // Call Mailbox
    if (mbox_call(ADDR(mBuf), MBOX_CH_PROP) // mailbox call is successful ?
        && mBuf[20] == COLOR_DEPTH          // got correct color depth ?
        && mBuf[24] == PIXEL_ORDER          // got correct pixel order ?
        && mBuf[28] != 0                    // got a valid address for frame buffer ?
    )
    {
        /* Convert GPU address to ARM address (clear higher address bits)
         * Frame Buffer is located in RAM memory, which VideoCore MMU
         * maps it to bus address space starting at 0xC0000000.
         * Software accessing RAM directly use physical addresses
         * (based at 0x00000000)
         */
        mBuf[28] &= 0x3FFFFFFF;
        // Access frame buffer as 1 byte per each address
        fb = (unsigned char *)((unsigned long)mBuf[28]);
        width = mBuf[5];  // Actual physical width
        height = mBuf[6]; // Actual physical height
        pitch = mBuf[33]; // Number of bytes per line
    }
    else
    {
        uart_puts("Unable to get a frame buffer with provided setting\n");
    }
}

void drawPixelARGB32(int x, int y, unsigned int attr)
{
    int offs = (y * pitch) + (COLOR_DEPTH / 8 * x);
    // Access 32-bit together
    *((unsigned int *)(fb + offs)) = attr;
}

// Function to draw the image
void drawImage(const unsigned int *image, int x_offset, int y_offset, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            unsigned long pixel = image[y * width + x];
            drawPixelARGB32(x + x_offset, y + y_offset, pixel);
        }
    }
}

void drawrectARGB32(int x1, int y1, int x2, int y2, unsigned int attr, int fill)
{
    for (int y = y1; y <= y2; y++){
        for (int x = x1; x <= x2; x++){
            drawPixelARGB32(x, y, attr);
        }
    }
}

void drawLineARGB32(int x1, int y1, int x2, int y2, unsigned int color)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    while (1) {
        drawPixelARGB32(x1, y1, color);

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

int abs(int x) {
    return (x < 0) ? -x : x;
}

/* Functions to display text on the screen */
// NOTE: zoom = 0 will not display the character
void drawChar(unsigned char ch, int x, int y, unsigned int attr, int zoom)
{
    unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

    for (int i = 1; i <= (FONT_HEIGHT*zoom); i++) {
		for (int j = 0; j< (FONT_WIDTH*zoom); j++) {
			unsigned char mask = 1 << (j/zoom);
            if (*glyph & mask) { //only draw pixels belong to the character glyph
			    drawPixelARGB32(x + j, y + i, attr);
            }
		}
		glyph += (i % zoom) ? 0 : FONT_BPL;
    }
}

// Function to draw string of words
void drawString(int x, int y, char *str, unsigned int attr, int zoom)
{
    while (*str) {
        if (*str == '\r') {
            x = 0;
        } else if (*str == '\n') {
            x = 0; 
			y += (FONT_HEIGHT*zoom);
        } else {
            drawChar(*str, x, y, attr, zoom);
            x += (FONT_WIDTH*zoom);
        }
        str++;
    }
}

/* New function for Uart: Check and return if no new character, don't wait */
#if 0 //UART0
unsigned int uart_isReadByteReady(){
	return ( !(UART0_FR & UART0_FR_RXFE) );
}

#else //UART1
unsigned int uart_isReadByteReady(){
	return (AUX_MU_LSR & 0x01);
}
#endif

unsigned char getUart(){
    unsigned char ch = 0;
    if (uart_isReadByteReady())
    	ch = uart_getc();
    return ch;
}

/* Functions to delay, set/wait timer */

void wait_msec(unsigned int msVal){
    register unsigned long f, t, r, expiredTime; //64 bits

    // Get the current counter frequency (Hz), 1Hz = 1 pulses/second
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    
    // Read the current counter value
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    
    // Calculate expire value for counter
    /* Note: both expiredTime and counter value t are 64 bits,
    thus, it will still be correct when the counter is overflow */  
    expiredTime = t + f * msVal / 1000;

    do {
    	asm volatile ("mrs %0, cntpct_el0" : "=r"(r));
    } while(r < expiredTime);
}


void set_wait_timer(int set, unsigned int msVal) {
    static unsigned long expiredTime = 0; //declare static to keep value
    register unsigned long r, f, t;
    
    if (set) { /* SET TIMER */
        // Get the current counter frequency (Hz)
        asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));

        // Read the current counter
        asm volatile ("mrs %0, cntpct_el0" : "=r"(t));

        // Calculate expired time:
        expiredTime = t + f * msVal / 1000;
    } 
    else { /* WAIT FOR TIMER TO EXPIRE */
        do {
            asm volatile ("mrs %0, cntpct_el0" : "=r"(r));
        } while(r < expiredTime);
    }
}