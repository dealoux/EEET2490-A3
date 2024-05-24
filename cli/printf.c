#include "./printf.h"
#include "../uart/uart1.h"

#define MAX_PRINT_SIZE 256

// Helper function for padding
void add_padding(char *buffer, int *buffer_index, int pad_count, int width, int zeroPad) {
  char pad_char = zeroPad ? '0' : ' ';
  while(pad_count < width && *buffer_index < MAX_PRINT_SIZE - 1){
    buffer[(*buffer_index)++] = pad_char;
    pad_count++;
  }
}

// Helper function for integer to ASCII conversion
void itoa(int num, char *temp_buffer, int *temp_index, int base) {
  int is_negative = 0;
  if (num < 0 && base == 10) {  // Handle negative numbers only for decimal
    is_negative = 1;
    num = -num;
  }

  while ((num /= base) != 0){
    int digit = num % base;
    temp_buffer[(*temp_index)--] = digit < 10 ? '0' + digit : 'a' + (digit - 10);
  }

  if (is_negative) {
    temp_buffer[(*temp_index)--] = '-';
  }
}

// Main printf function
void printf(const char *string, ...) {
  va_list ap;
  va_start(ap, string);
  
  char buffer[MAX_PRINT_SIZE];
  int buffer_index = 0;

  while (*string && buffer_index < MAX_PRINT_SIZE - 1) {
    if (*string != '%') {
      buffer[buffer_index++] = *string++;
      continue;
    }

    string++;  // Skip '%'
    char temp_buffer[MAX_PRINT_SIZE];
    int temp_index = MAX_PRINT_SIZE - 1;
    int zeroPad = 0, width = 0, precision = -1;

    if (*string == '0') {
      zeroPad = 1;
      string++;
    }

    while (*string >= '0' && *string <= '9') {
      width = width * 10 + (*string - '0');
      string++;
    }

    if (*string == '.') {
      precision = 0;
      string++;
      while (*string >= '0' && *string <= '9') {
        precision = precision * 10 + (*string - '0');
        string++;
      }
    }

    int count = 0;

    // Handle format specifiers
    switch (*string++) {
    case 'd':
    case 'x':
      int base = (*string == 'x') ? 16 : 10;
      int num = va_arg(ap, int);
      itoa(num, temp_buffer, &temp_index, base);
      count = MAX_PRINT_SIZE - 1 - temp_index;
      add_padding(buffer, &buffer_index, count, width, zeroPad);
      for (int i = temp_index + 1; i < MAX_PRINT_SIZE; i++) {
          buffer[buffer_index++] = temp_buffer[i];
      }
      break;
    
    case 's': 
      char *str = va_arg(ap, char *);
      while(*str && (precision == -1 || count < precision)){
        buffer[buffer_index++] = *str++;
        count++;
      }
      add_padding(buffer, &buffer_index, count, width, zeroPad);
      break;
      
    case 'c': 
      char c = (char)va_arg(ap, int);  // char is promoted to int in variadic functions
      buffer[buffer_index++] = c;
      add_padding(buffer, &buffer_index, 1, width, zeroPad);
      break;
    
    default:
      buffer[buffer_index++] = '%';  // Handle unknown format specifiers
      if(*string) {
        buffer[buffer_index++] = *string;
      }
    }
  }

  buffer[buffer_index] = '\0';  // Ensure null termination
  uart_puts(buffer);  // Function to send the buffer to UART
  va_end(ap);
}