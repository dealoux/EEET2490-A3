#include "command.h"
#include "printf.h"
#include "../kernel/utils.h"
#include "../uart/uart1.h"
#include "../kernel/mbox.h"
#include "../kernel/string.h"
#include "../resources/image.h"
#include "../resources/video.h"
#include "../resources/characterFont.h"
#include "../game/danmaku.h"

extern volatile unsigned int mBuf[];

// instantiate a list of commands
Command commandList[] = {
  {"help", "Show brief information of all commands.\nExample: MyBareOS> help\n", displayAllCommands},
  {"help <command_name>", "Show full information of the specified command.\nExample: MyBareOS> help showinfo\n"},
  {"clear", "Clear the terminal.\nExample: MyBareOS> clear\n", clearCLI},
  {"setcolor", "Set text color, and/or background color of the console to one of the following colors: BLACK, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE.\nExamples:\n MyBareOS> setcolor -t green\nMyBareOS> setcolor -b green -t yellow\n", setConsoleColor},
  {"showinfo", "Show board revision and board MAC address.", displayBoardInfo},
  // uarts commands
  {"set_baud", "Set UART baud rate.\nExample: MyBareOS> set_baud 9600", setBaudRate},
  {"set_databits", "Set number of data bits configuration to 5, 6, 7, or 8.\nExample: MyBareOS> set_databits 7", setDataBits},
  {"set_stopbits", "Set stop bits configuration to 1 or 2.\nExample: ", setStopBits},
  {"set_parity", "Set parity configuration to one of the following: NONE, EVEN, ODD.\nExample: MyBareOS> set_parity odd", setParity},
  {"set_handshaking", "Set CTS/RTS handshaking to ON or OFF.\nExample: MyBareOS> set_handshaking on", setHandshaking},
  // screen commands
  {"clear_display", "Clear the screen display.\nExample: MyBareOS> clear_display", clearDisplay},
  {"show_image", "Display the image on the screen, scroll vertically using W and S keys.\nExample: MyBareOS> show_image", showImage},
  {"show_video", "Display the video on the screen.\nExample: MyBareOS> show_video", showVideo},
  {"show_team_info", "Display team members' names on the screen.\nExample: MyBareOS> show_team_info", showTeamInfo},
  {"start_game", "Start the game.\nExample: MyBareOS> start_game", startGame}
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

int currentImageIndex = 0;
int currentXOffset = 0; // This will track our current X position when scrolling
int currentYOffset = -60; // This will track our current Y position when scrolling

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

void clearCLI(char *args){
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
  printf("\nBaud rate updated.\n");
}

void setDataBits(char *args) {
  if(strcmp(args, "5") != 0 && strcmp(args, "6") != 0 && strcmp(args, "7") != 0 && strcmp(args, "8") != 0) {
    printf("\nInvalid data bits setting. Use '5', '6', '7', or '8'.\n");
    return;
  }

  unsigned char dataBits = (unsigned char)strtoul(args, NULL, 10);
  uart_set_data_bits(dataBits);
  printf("\nData bits setting updated.\n");
}

void setStopBits(char *args) {
  unsigned char stop_bits = (unsigned char)strtoul(args, NULL, 10);
  if (stop_bits == 1 || stop_bits == 2) {
    uart_set_stop_bits(stop_bits);
    printf("\nStop bits setting updated.\n");
  } 
  else {
    printf("\nInvalid stop bits setting. Use '1' or '2'.\n");
  }
}

void setParity(char *args) {
  if (strcmp(args, "none") == 0 || strcmp(args, "even") == 0 || strcmp(args, "odd") == 0) {
    uart_set_parity(args);
    printf("\nParity setting updated.\n");
  } 
  else {
    printf("\nInvalid parity setting. Use 'none', 'even', or 'odd'.\n");
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
    printf("\nInvalid handshaking command. Use 'on' or 'off'.\n");
  }
}

// helper functions
void drawGlyph(const unsigned int glyph[][2500], char ch, int base, int x, int y, unsigned int attr) {
  int pos = ch - base;
  int row = 0;
  int col = 0;
  
  for (int i = y; i < y + 50; i++) {
    for (int j = x; j < x + 50; j++) {
      // Calculate the linear index for the 2D glyph data
      int pixCount = row * 50 + col;
      
      if (glyph[pos][pixCount] == 0x00000000) {
        drawPixelARGB32(j, i, attr);
      }
      
      col++;  // Move to next column
      if (col >= 50) {  // If end of the row, move to next row and reset column
        col = 0;
        row++;
      }
    }
  }
}

void drawCharacter(char ch, int x, int y, unsigned int attr) {
  drawGlyph(character, ch, 'A', x, y, attr);  // Use 'A' as the base character
}

void clearDisplay(char *args){
  drawRectARGB32(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00000000, 1);
}

void showImage(char *args) {
  // If necessary, load the image based on args or use a predefined image
  drawImage(large_img, currentXOffset, currentYOffset, IMG_WIDTH, IMG_HEIGHT);
  char key = 0;
  
  do {
    key = uart_getc();
    if (key == 'w' && currentYOffset < 0) {
      // Scroll up, but make sure we don't go past the top of the image
      currentYOffset += 3; // Scroll a fraction of the image's height
    }
    else if (key == 's' && currentYOffset > -(IMG_HEIGHT - SCREEN_HEIGHT)) {
      currentYOffset -= 3;
    }
    // else if (key == 'a' && currentXOffset < 0 ) {
    //   currentXOffset += 3; 
    // }
    // else if (key == 'd' && currentYOffset > -(IMG_WIDTH - SCREEN_WIDTH)) {
    //   currentXOffset -= 3;
    // }
    drawImage(large_img, currentXOffset, currentYOffset, IMG_WIDTH, IMG_HEIGHT);
  } while (key != 'q'); // Exit on pressing 'q'

  // clear screen
  drawRectARGB32(0, 0, 2000, 2000, 0x00000000, 1);
}

void showVideo(char *args) {
  for (int i = 0; i < VIDEO_FRAMES_LENGTH; i++) {
    drawImage(videoBitmapArr[i], 0, 0, VIDEO_WIDTH, VIDEO_HEIGHT);
    wait_msec(50000);
  }
}

// Helper function to draw a string on the screen
void drawStringHelper(const char* str, int startX, int startY, unsigned int color) {
  int x = startX;
  while(*str) {
    drawCharacter(*str, x, startY, color);
    x += 60;
    str++;
  }
}

void showTeamInfo(char *args) {
  drawStringHelper("DUC LE", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 0xFF00FF00);
}

void startGame(char *args){
  printf("Starting game...\n");
  gameInit();
  printf("Game initialized.\n");
  gameLoop();
}