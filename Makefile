SRC_DIR = ./kernel
BUILD_DIR = ./build
UART_DIR = ./uart
CLI_DIR = ./cli

# Find all .c files and replace with corresponding .o files
CFILES := $(shell find $(SRC_DIR) -name '*.c') $(shell find $(UART_DIR) -name '*.c') $(shell find $(CLI_DIR) -name '*.c')
OFILES := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(CFILES)))

GCCFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

all: clean kernel8.img run

# Ensure boot.o is built separately
$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot.s
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c $< -o $@

# Build UART, SRC, and CLI directories
$(BUILD_DIR)/%.o: $(UART_DIR)/%.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(CLI_DIR)/%.c
	aarch64-linux-gnu-gcc $(GCCFLAGS) -c $< -o $@

# Link all object files into the final kernel image
kernel8.img: $(BUILD_DIR)/boot.o $(OFILES)
	aarch64-linux-gnu-ld -nostdlib $^ -T $(SRC_DIR)/link.ld -o $(BUILD_DIR)/kernel8.elf
	aarch64-linux-gnu-objcopy -O binary $(BUILD_DIR)/kernel8.elf kernel8.img

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/kernel8.elf *.img

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio