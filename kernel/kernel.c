#include "utils.h"
#include "../uart/uart1.h"
#include "../cli/printf.h"
#include "../cli/cli.h"

void main(){
	// set up serial console
	uart_init();
	framebfInit();
	initCli();

	// run CLI
	while (1){
		cli_main();
	}
}