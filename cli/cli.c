#include "cli.h"
#include "command.h"
#include "printf.h"
#include "../uart/uart1.h"
#include "../kernel/mbox.h"
#include "../kernel/string.h"

#define MAX_CMD_SIZE 100
#define MAX_HISTORY 20

char commandHistory[MAX_HISTORY][MAX_CMD_SIZE];
int lastCommandIndex = 0;
int historyIndex = 0;

extern volatile unsigned int mBuf[];

void processCommand(char *command){
  // seperate the commands and arguements
  char *commandName = strtok(command, " ");
  char *args = strtok(NULL, "");
  for (size_t i = 0; i < sizeof(commandList) / sizeof(Command); i++)
  {
    if (strcmp(commandName, commandList[i].name) == 0){
      printf("\n");
      commandList[i].handler(args);
      return;
    }
  }
  printf("\nCommand not found: %s. Please use help to view all valid commands.", commandName);
}

void autocompleteHandler(char *buffer, int *index)
{
  // For simplicity, autocomplete with the first matching command
  for (int i = 0; i < sizeof(commandList) / sizeof(Command); i++){
    if (strstr(commandList[i].name, buffer) == commandList[i].name){
      // found a match, complete the buffer
      strcpy(buffer, commandList[i].name);
      *index = strlen(buffer);
      printf("\rMyBareOS> %s", buffer); // refresh the CLI prompt
      return;
    }
  }
}

void cli_main() {
  static char cli_buffer[MAX_CMD_SIZE] = {0};
  static int index = 0;
  static int isNewCommand = 1;

  if (isNewCommand) {
    printf("\nMyBareOS> ");
    isNewCommand = 0;
  }

  char c = uart_getc();

  // Detecting the beginning of an escape sequence (for handling arrow keys) 
  if (c == '\033') { // ESC character
    uart_getc(); // skip the '['
    c = uart_getc(); // get the actual arrow direction

    // Handle arrow keys
    switch (c) {
      // Up arrow
      case 'A':
        if (historyIndex > 0) {
          historyIndex--;
          strcpy(cli_buffer, commandHistory[historyIndex]);
          printf("\rMyBareOS> %s", cli_buffer);
          index = strlen(cli_buffer);
        }
        break;

      // Down arrow
      case 'B':
        if (historyIndex < lastCommandIndex - 1) {
            historyIndex++;
            strcpy(cli_buffer, commandHistory[historyIndex]);
            printf("\rMyBareOS> %s", cli_buffer);
            index = strlen(cli_buffer);
        } 
        else if (historyIndex == lastCommandIndex - 1) {
            historyIndex++;
            cli_buffer[0] = '\0'; // clear buffer to allow new command entry
            printf("\rMyBareOS> ");
            index = 0;
        }
        break;
      }
    } 
  // Handle normal characters
  else {
    switch (c) {
      // Handle backspace and delete
      case 0x7F:
      case 0x08:
        if (index > 0) {
          uart_sendc('\b');      // move cursor backwards
          uart_sendc(' ');       // overwrite the last character with space
          uart_sendc('\b');      // move cursor backwards again
          cli_buffer[--index] = '\0';  // "delete" last char in buffer
        }
        break;

      // Handle tab
      case '\t':
        autocompleteHandler(cli_buffer, &index);
        break;

      // Handle enter
      case '\n':
        cli_buffer[index] = '\0';
        if (index > 0) {  // Only save non-empty commands
          strcpy(commandHistory[historyIndex % MAX_HISTORY], cli_buffer);
          lastCommandIndex = (lastCommandIndex + 1) % MAX_HISTORY;
          historyIndex = lastCommandIndex;
        }
        processCommand(cli_buffer);
        isNewCommand = 1;
        index = 0;
        break;

      // Handle all other characters
      default:
        if (index < MAX_CMD_SIZE - 1) {
          uart_sendc(c);
          cli_buffer[index++] = c;
        }
        break;
    }
  }
}

void initCli(){
	// ANSI escape code to reset to default
	const char *resetColor = "\033[0m";

	printf("%s", colorMappings[3].textColorAscii); // Set the text color to yellow
	printf("oooooooooooo oooooooooooo oooooooooooo ooooooooooooo    .oooo.         .o    .ooooo.     .oooo.\n");
	printf("`888'     `8 `888'     `8 `888'     `8 8'   888   `8  .dP  888b      .d88   888' `Y88.  d8P'`Y8b\n");
	printf(" 888          888          888              888             ]8P'   .d'888   888    888 888    888\n");
	printf(" 888oooo8     888oooo8     888oooo8         888           .d8P'  .d'  888    'Vbood888 888    888\n");
	printf(" 888          888          888              888         .dP'     88ooo888oo       888' 888    888\n");
	printf(" 888       o  888       o  888       o      888       .oP     .o      888       .88P'  `88b  d88'\n");
	printf("o888ooooood8 o888ooooood8 o888ooooood8     o888o      8888888888     o888o    .oP'      `Y8bd8P'\n");
	printf("\n");
	printf("\n");
	printf("\n");
	printf("oooooooooo.        .o.       ooooooooo.   oooooooooooo         .oooooo.    .oooooo..o\n");
	printf("`888'   `Y8b      .888.      `888   `Y88. `888'     `g        d8P'  `Y8b  d88P'   `Y8\n");
	printf(" 888     888     .8 888.      888   .d88'  888               888      888 Y88bo.\n");
	printf(" 888oooo888'    .8' `888.     888ooo88P'   888oooo8          888      888   `Y8888o.\n");
	printf(" 888    `88b   .88ooo8888.    888`88b.     888               888      888      ` Y88b\n");
	printf(" 888    .88P  .8'     `888.   888  `88b.   888       o       `88b    d88' oo     .d8P\n");
	printf("o888bood8P'  o88o     o8888o o888o  o888o o888ooooood8        `Y8bood8P'  888888P'\n");
	printf("\n");
	printf("%s", colorMappings[2].textColorAscii); // green footer
	printf("    Developed by Le Minh Duc - s4000577\n");

	printf("%s", resetColor); // default text color
}