#ifndef COMMAND_H
#define COMMAND_H

#define COMMAND_COUNT 10
#define COLOR_COUNT 8

// Function type for command handlers
typedef void (*CommandFunction)(char *args);

// Struct to represent a command
typedef struct{
  const char *name;        // Command name
  const char *description; // Short description
  CommandFunction handler; // Function to handle the command
} Command;

typedef struct{
  const char *colorName;      // Color name
  const char *textColorAscii; // Font color
  const char *bgColorAscii;   // Background colors
} ColorMap;

// List of commands
extern Command commandList[COMMAND_COUNT];
extern ColorMap colorMappings[COLOR_COUNT];

// Declarations
const char *findTextColor(const char *colorStr);
const char *findAsciiBgColor(const char *colorStr);
void displayAllCommands(char *args);
void clearScreen(char *args);
void setConsoleColor(char *args);
void displayBoardInfo(char *args);

// uart commands
void setBaudRate(char *args);
void setDataBits(char *args);
void setStopBits(char *args);
void setParity(char *args);
void setHandshaking(char *args);

#endif