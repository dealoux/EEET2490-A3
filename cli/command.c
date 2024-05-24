#include "command.h"
#include "printf.h"
#include "../uart/uart1.h"
#include "../kernel/mbox.h"
#include "../kernel/string.h"

extern volatile unsigned int mBuf[];

// instantiate a list of commands
Command commandList[] = {
  {"help", "Show brief information of all commands.\nExample: MyBareOS> help\n", displayAllCommands},
  {"help <command_name>", "Show full information of the specified command.\nExample: MyBareOS> help showinfo\n"},
  {"clear", "Clear the terminal.\nExample: MyBareOS> clear\n", clearScreen},
  {"setcolor", "Set text color, and/or background color of the console to one of the following colors: BLACK, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE.\nExamples:\n MyBareOS> setcolor -t green\nMyBareOS> setcolor -b green -t yellow\n", setConsoleColor},
  {"showinfo", "Show board revision and board MAC address.", displayBoardInfo},
  // uarts commands
  {"set_baud", "Set UART baud rate.\nExample: MyBareOS> set_baud 9600", setBaudRate},
  {"set_databits", "Set number of data bits configuration to 5, 6, 7, or 8.\nExample: MyBareOS> set_databits 7", setDataBits},
  {"set_stopbits", "Set stop bits configuration to 1 or 2.\nExample: ", setStopBits},
  {"set_parity", "Set parity configuration to one of the following: NONE, EVEN, ODD.\nExample: MyBareOS> set_parity odd", setParity},
  {"set_handshaking", "Set CTS/RTS handshaking to ON or OFF.\nExample: MyBareOS> set_handshaking on", setHandshaking},
};

// Instantiate the colors
ColorMap colorMappings[] = {
  {"black", "\033[1;30m", "\x1b[40m"},
  {"red", "\033[1;31m", "\x1b[41m"},
  {"green", "\033[1;32m", "\x1b[42m"},
  {"yellow", "\033[1;33m", "\x1b[43m"},
  {"blue", "\033[1;34m", "\x1b[44m"},
  {"purple", "\033[1;35m", "\x1b[45m"},
  {"cyan", "\033[1;36m", "\x1b[46m"},
  {"white", "\033[1;37m", "\x1b[47m"}
};

unsigned long strtoul(const char *str, char **endptr, int base){
  unsigned long result = 0;
  while (*str){
    if (*str >= '0' && *str <= '9'){
      result = result * base + *str - '0';
    }
    else if (*str >= 'A' && *str <= 'Z'){
      result = result * base + *str - 'A' + 10;
    }
    else if (*str >= 'a' && *str <= 'z'){
      result = result * base + *str - 'a' + 10;
    }
    else{
      break;
    }
    str++;
  }
  if (endptr){
    *endptr = (char *)str;
  }
  return result;
}

const char *findTextColor(const char *colorStr){
  for (size_t i = 0; i < sizeof(colorMappings) / sizeof(ColorMap); i++){
    if (strcmp(colorStr, colorMappings[i].colorName) == 0){
      return colorMappings[i].textColorAscii;
    }
  }
  return NULL; // Not found
}

const char *findAsciiBgColor(const char *colorStr){
  for (size_t i = 0; i < sizeof(colorMappings) / sizeof(ColorMap); i++){
    if (strcmp(colorStr, colorMappings[i].colorName) == 0){
      return colorMappings[i].bgColorAscii;
    }
  }
  return NULL; // Not found
}

void displayAllCommands(char *args)
{
  // check if the arg and its value is not null
  if (args && *args){
    for (size_t i = 0; i < sizeof(commandList) / sizeof(Command); i++){
      // check if the command name is equal to the arg
      if (strcmp(args, commandList[i].name) == 0){
        printf("--%s: \n%s\n", commandList[i].name, commandList[i].description);
        return;
      }
    }
    printf("\nCommand '%s' not found.\n", args); // If no command matched
  }
  // Display all commands if no specific command name is provided
  else{
    printf("\nAvailable commands:\n");
    for (size_t i = 0; i < sizeof(commandList) / sizeof(Command); i++){
      printf("\n--%s: \n%s\n", commandList[i].name, commandList[i].description);
    }
  }
}

void clearScreen(char *args){
  printf("\033[2J\033[1;1H");
}

void setConsoleColor(char *args)
{
  char *token = strtok(args, " ");
  while (token)
  {
    const char *asciiColor = NULL;
    printf("\n%s", token);
    if (strcmp(token, "-t") == 0){
      token = strtok(NULL, " ");
      
      if (token){
        asciiColor = findTextColor(token); // retrieve the color input
        if (asciiColor){
            printf(asciiColor); // Set text color
            printf("\n");
        }
        else{
          printf("Invalid color: %s, please type help setcolor to find valid colors\n", token);
        }
      }
    }
    else if (strcmp(token, "-b") == 0){
      token = strtok(NULL, " ");
      
      if (token){
        asciiColor = findAsciiBgColor(token);
        if (asciiColor){
            printf(asciiColor); // Set background color
            printf("\n");
            return;
        }
        else{
          printf("Invalid color: %s, please type help setcolor to find valid colors\n", token);
        }
      }
    }
      token = strtok(NULL, " ");
  }
}

void displayBoardInfo(char *args){
  unsigned int *response = 0;
  // display board model
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_GETMODEL, &response, 4, 0);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("\nBoard model %16c %d\n", ':', response[0]);
  
  // display board serial
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_GETSERIAL, &response, 8, 0);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("Board serial %15c %d\n", ':', response[0]);
  
  // display mac address
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_MACADDR, &response, 6, 0);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  unsigned char bytes[6];
  bytes[0] = (response[0] >> 24) & 0xFF; // extract the most significant byte (1st byte)
  bytes[1] = (response[0] >> 16) & 0xFF; // 2nd byte
  bytes[2] = (response[0] >> 8) & 0xFF;  // 3rd byte
  bytes[3] = (response[0]) & 0xFF;       // 4th byte
  bytes[4] = (response[1] >> 8) & 0xFF;  // 5th byte (1st byte of the 2nd half of the MAC address)
  bytes[5] = (response[1]) & 0xFF;       // 6th byte (2nd byte of the 2nd half of the MAC address)
  printf("Board MAC address %10c %x:%x:%x:%x:%x:%x\n", ':', bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
  
  // display board revision
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_GETBOARDREVISION, &response, 4, 0);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("Board revision %13c %x\n", ':', response[0]);

  // display ARM memory
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_ARM_MEMORY, &response, 4, 0);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("ARM memory %17c %dMB\n", ':', response[0] / 10485760); // convert to megabytes
  
  // display VC memory
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_VC_MEMORY, &response, 8, 0);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("VC memory %18c %dMB\n", ':', response[0] / 10485760); // convert to megabytes

  // display clock rate of arm
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_GETCLKRATE, &response, 8, 0, 3);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("ARM clock rate %13c %dMHz\n", ':', response[0] / 1000000); // convert to MH

  // display clock rate of uart
  mbox_buffer_setup(ADDR(mBuf), MBOX_TAG_GETCLKRATE, &response, 8, 0, 2);
  mbox_call(ADDR(mBuf), MBOX_CH_PROP);
  printf("UART clock rate %12c %dMHz\n", ':', response[0] / 1000000); // convert to MH
}

void setBaudRate(char *args) {
  unsigned int baudRate = strtoul(args, NULL, 10);
  uart_set_baud_rate(baudRate);
  uart_puts("\nBaud rate updated.\n");
}

void setDataBits(char *args) {
  if(strcmp(args, "5") != 0 && strcmp(args, "6") != 0 && strcmp(args, "7") != 0 && strcmp(args, "8") != 0) {
    uart_puts("\nInvalid data bits setting. Use '5', '6', '7', or '8'.\n");
    return;
  }

  unsigned char dataBits = (unsigned char)strtoul(args, NULL, 10);
  uart_set_data_bits(dataBits);
  uart_puts("\nData bits setting updated.\n");
}

void setStopBits(char *args) {
  unsigned char stop_bits = (unsigned char)strtoul(args, NULL, 10);
  if (stop_bits == 1 || stop_bits == 2) {
    uart_set_stop_bits(stop_bits);
    uart_puts("\nStop bits setting updated.\n");
  } 
  else {
    uart_puts("\nInvalid stop bits setting. Use '1' or '2'.\n");
  }
}

void setParity(char *args) {
  if (strcmp(args, "none") == 0 || strcmp(args, "even") == 0 || strcmp(args, "odd") == 0) {
    uart_set_parity(args);
    uart_puts("\nParity setting updated.\n");
  } 
  else {
    uart_puts("\nInvalid parity setting. Use 'none', 'even', or 'odd'.\n");
  }
}

void setHandshaking(char *args) {
  if (strcmp(args, "on") == 0) {
    uart_enable_handshaking();
  } 
  else if (strcmp(args, "off") == 0) {
    uart_disable_handshaking();
  } 
  else {
    uart_puts("\nInvalid handshaking command. Use 'on' or 'off'.\n");
  }
}