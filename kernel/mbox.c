#include "mbox.h"
#include "gpio.h"
#include "../uart/uart1.h"
#include "../cli/printf.h"
#include "../gcclib/stddef.h"
#include "../gcclib/stdint.h"
#include "../gcclib/stdarg.h"

volatile unsigned int mBuf[36];

// Define mailbox buffer as per the alignment requirements
volatile unsigned int __attribute__((aligned(16))) mbox_buffer[36];

// Function to read from the mailbox
uint32_t mailbox_read(unsigned char channel) {
    uint32_t res = MBOX0_READ;

    while((res & 0xF) != channel){
      // Wait for mailbox to be non-empty
      while(MBOX0_STATUS & MBOX_EMPTY) {
        asm volatile("nop");
      }
      // Read the message
      res = MBOX0_READ;
    }

    return res;
}

// Function to write to the mailbox
void mailbox_send(uint32_t msg, unsigned char channel) {
  // Ensure that the message conforms to the mailbox format
  msg &= ~0xF;
  msg |= channel & 0xF;

  // Wait for the mailbox to be non-full
  while (MBOX1_STATUS & MBOX_FULL) {
    asm volatile("nop");
  }
  MBOX1_WRITE = msg;
}

// Function to make a mailbox call
int mbox_call(unsigned int buffer_addr, unsigned char channel) {
  unsigned int msg = (buffer_addr & ~0xF) | (channel & 0xF);
  mailbox_send(msg, channel);

  // Wait for the response
  if(msg == mailbox_read(channel)){
    return mbox_buffer[1] == MBOX_RESPONSE;
  }
  uart_puts("Mailbox call failed\n");
  return 0;
}

// Helper functions for mailbox buffer setup
void mbox_buffer_init(volatile unsigned int *mbox, unsigned int tag, unsigned int buffer_size) {
  mbox[0] = buffer_size;  // Overall buffer size
  mbox[1] = MBOX_REQUEST; // This is a request message
  mbox[2] = tag;          // Specific tag
  mbox[3] = buffer_size - 3 * sizeof(unsigned int); // Size of the data buffer
  mbox[4] = 0;            // Request code (0 for request, set by the GPU/VPU on response)
}

void mbox_buffer_finalize(volatile unsigned int *mbox, unsigned int total_size) {
  mbox[total_size / 4 - 1] = MBOX_TAG_LAST;
}

// Function to setup and send a mailbox buffer with variable arguments
void mbox_buffer_setup(unsigned int buffer_addr, unsigned int tag_identifier, unsigned int **res_data, unsigned int res_length, unsigned int req_length, ...) {
  va_list args;
  va_start(args, req_length);
  volatile unsigned int *mbox = (volatile unsigned int *)(uintptr_t)buffer_addr;

  unsigned int bufferSize = res_length >= req_length ? res_length : req_length;
  unsigned int totalSize = (bufferSize + 3) * 4; // Calculate total buffer size including header and end tag

  mbox_buffer_init(mbox, tag_identifier, totalSize);

  // Set data according to the tag identifier
  switch (tag_identifier) {
    case MBOX_TAG_GETSERIAL:
    case MBOX_TAG_MACADDR:
    case MBOX_TAG_GETMODEL:
    case MBOX_TAG_ARM_MEMORY:
    case MBOX_TAG_GETBOARDREVISION:
    case MBOX_TAG_VC_MEMORY:
      mbox[5] = 0;  // Initialize the response buffer area to 0
      *res_data = (unsigned int *) &mbox[5];
      break;

    case MBOX_TAG_GETCLKRATE:
      mbox[5] = va_arg(args, unsigned int);  // Additional data for the clock rate request
      mbox[6] = 0;  // Initialize the response buffer area to 0
      *res_data = (unsigned int *) &mbox[6];
      break;

    default:
      printf("Unknown tag identifier: %d\n", tag_identifier);
      va_end(args);
      return;
  }

  mbox_buffer_finalize(mbox, totalSize);
  va_end(args);
}