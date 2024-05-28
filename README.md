# Bare-Metal Arcade Shooter Game on Raspberry Pi

This project involves developing a simple arcade-style shooter game running on a bare-metal Raspberry Pi environment. The primary focus is on bare-metal OS development, involving direct hardware manipulation for graphics display and game mechanics.

## Features

- **Framebuffer Graphics**: Directly manipulate the framebuffer for graphics rendering, including drawing pixels, lines, rectangles, and text.
- **Timers and Interrupts**: Utilize system timers and interrupts for real-time game mechanics, such as player respawn and enemy spawning.
- **Game Mechanics**: Implement player movement, shooting, enemy spawning, collision detection, and power-up collection.

## Project Structure

- `danmaku.h`: Main game logic and structure definitions.
- `utils.h`, `font.h`, `mbox.h`: Utilities for framebuffer initialization and mailbox communication.
- `uart/uart1.h`: UART communication for debugging and input handling.
- `resources/`: Contains sprite data and font for rendering.
- `kernel/`: Kernel-level utilities and system timer configuration.

## Getting Started

### Prerequisites

- Raspberry Pi (any model)
- Cross-compilation toolchain for ARM
- SD card with bootable Raspberry Pi OS (lite version recommended)

### Building and Running

1. **Clone the repository**:

   ```bash
   git clone <repository-url>
   cd <repository-directory>
    ```

2. **Build the project**:

   ```bash
   make all
    ```