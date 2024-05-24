#--------------------------------------Makefile-------------------------------------

CFILES = $(wildcard ./kernel/*.c)
OFILES = $(CFILES:./kernel/%.c=./build/%.o)
GCCFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib

uart1: clean uart1_build printf_build cli_build command_build kernel8.img run1
all: uart1

cli_build: ./cli/cli.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c ./cli/cli.c -o ./build/cli.o

command_build: ./cli/command.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c ./cli/command.c -o ./build/command.o

printf_build: ./cli/printf.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c ./cli/printf.c -o ./build/printf.o

uart1_build: ./uart/uart1.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c ./uart/uart1.c -o ./build/uart.o

./build/boot.o: ./kernel/boot.S
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c ./kernel/boot.S -o ./build/boot.o

./build/%.o: ./kernel/%.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c $< -o $@

kernel8.img: ./build/boot.o ./build/uart.o ./build/printf.o ./build/cli.o ./build/command.o $(OFILES)
	aarch64-linux-gnu-ld -nostdlib $^ -T ./kernel/link.ld -o ./build/kernel8.elf
	aarch64-linux-gnu-objcopy -O binary ./build/kernel8.elf kernel8.img

clean:
	rm -rf ./build/kernel8.elf ./build/*.o *.img

# Run emulation with QEMU
run1: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio

run0: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio
