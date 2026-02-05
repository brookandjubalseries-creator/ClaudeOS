# ClaudeOS

A 32-bit x86 operating system built entirely by multiple Claude AI instances working collaboratively.

![ClaudeOS Banner](https://img.shields.io/badge/ClaudeOS-v0.2.0-blue) ![Architecture](https://img.shields.io/badge/arch-i386-green) ![Built By](https://img.shields.io/badge/built%20by-Claude%20AI-purple)

## What is This?

ClaudeOS is a fully functional operating system kernel written from scratch by Claude AI instances. The unique aspect of this project is that it was developed through **multi-agent collaboration** - multiple Claude instances were assigned different roles and worked together, coordinating through a shared text file (`Channel.txt`).

## How It Was Made

### The Multi-Claude Development Process

1. **Boss Claude** - The coordinator who assigned tasks, reviewed work, and managed the overall architecture
2. **Kernel Claude (Worker1)** - Responsible for core kernel development: bootloader, interrupts, memory management, process scheduling, and system calls
3. **Shell+FS Claude (Worker2)** - Built the shell, command parser, filesystem, and user-facing features

The Claudes communicated through `Channel.txt`, posting updates, asking questions, and coordinating their work - simulating a real development team.

### Development Timeline

The OS was built in phases:
- **Phase 1**: Basic bootloader with Multiboot header, VGA text output
- **Phase 2**: Interrupt handling (IDT, ISR, IRQ), keyboard driver
- **Phase 3**: Shell with lexer/parser, Virtual File System
- **Phase 4**: Timer, process scheduler, system calls, AI assistant

### Tools Used

- **Assembler**: NASM (for bootloader and ISR stubs)
- **Compiler**: i686-elf-gcc cross-compiler (generates proper ELF binaries)
- **Emulator**: QEMU for testing
- **AI**: Claude Opus 4.5 (Anthropic)

## Features

### Kernel
- Multiboot-compliant bootloader (works with QEMU's `-kernel` option)
- Protected mode (32-bit)
- Interrupt Descriptor Table (IDT) with 256 entries
- Hardware interrupt handling via 8259 PIC
- Programmable Interval Timer (PIT) at 100Hz
- Kernel heap allocator (`kmalloc`/`kfree`)
- Basic round-robin process scheduler
- System call interface (INT 0x80)

### Drivers
- VGA text mode (80x25, 16 colors)
- PS/2 keyboard with full scancode translation
- Timer with uptime tracking

### Shell
- Interactive command-line interface
- Lexer/parser for command parsing
- Support for pipes (`|`), redirects (`>`, `>>`), and background (`&`)
- Command history
- 20+ built-in commands

### File System
- In-memory Virtual File System (VFS)
- Directories and files
- Pre-populated with `/etc/motd`, `/etc/hostname`, sample files

### Built-in Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `ls` | List directory contents |
| `cd` | Change directory |
| `cat` | Display file contents |
| `echo` | Print text |
| `pwd` | Print working directory |
| `mkdir` | Create directory |
| `touch` | Create empty file |
| `clear` | Clear screen |
| `uname` | System information |
| `uptime` | Show system uptime |
| `ps` | List processes |
| `whoami` | Current user |
| `date` | Show current date |
| `reboot` | Reboot system |
| `claude` | **AI Assistant** - ask questions! |

## Building

### Prerequisites

1. **NASM** - Netwide Assembler
2. **i686-elf cross-compiler** - Download from [i686-elf-tools](https://github.com/lordmilko/i686-elf-tools/releases)
3. **QEMU** - For running the OS
4. **Make** - GNU Make or compatible

### Setup (Windows)

```bash
# Install NASM
winget install NASM.NASM

# Download i686-elf-tools and extract to C:\i686-elf-tools
# Update Makefile TOOLCHAIN path accordingly

# Install QEMU
winget install SoftwareFreedomConservancy.QEMU
```

### Build

Edit the `Makefile` to set your toolchain paths:

```makefile
TOOLCHAIN = /path/to/i686-elf-tools/bin
AS = /path/to/nasm
```

Then build:

```bash
make clean
make all
```

### Run

```bash
make run
```

Or directly:

```bash
qemu-system-i386 -kernel build/claudeos.bin -m 128M
```

## Project Structure

```
ClaudeOS/
├── boot/
│   ├── boot.asm        # Multiboot bootloader
│   └── demo.asm        # Standalone 512-byte boot demo
├── kernel/
│   ├── kernel.c        # Main kernel entry
│   ├── idt.c           # Interrupt Descriptor Table
│   ├── isr.asm         # Interrupt Service Routines
│   ├── pic.c           # 8259 PIC driver
│   ├── kmalloc.c       # Kernel heap allocator
│   ├── process.c       # Process scheduler
│   └── syscall.c       # System call handler
├── drivers/
│   ├── vga.c           # VGA text mode driver
│   ├── keyboard.c      # PS/2 keyboard driver
│   └── timer.c         # PIT timer driver
├── shell/
│   ├── shell.c         # Main shell REPL
│   ├── lexer.c         # Command tokenizer
│   ├── parser.c        # Command parser
│   ├── builtins.c      # Built-in commands
│   └── ai_assistant.c  # AI assistant feature
├── fs/
│   ├── vfs.c           # Virtual File System
│   └── ramfs.c         # RAM filesystem
├── include/            # Header files
├── Makefile            # Build system
├── linker.ld           # Linker script
└── Channel.txt         # Multi-Claude communication log
```

## Screenshots

When you boot ClaudeOS, you'll see:

```
   ██████╗██╗      █████╗ ██╗   ██╗██████╗ ███████╗ ██████╗ ███████╗
  ██╔════╝██║     ██╔══██╗██║   ██║██╔══██╗██╔════╝██╔═══██╗██╔════╝
  ██║     ██║     ███████║██║   ██║██║  ██║█████╗  ██║   ██║███████╗
  ██║     ██║     ██╔══██║██║   ██║██║  ██║██╔══╝  ██║   ██║╚════██║
  ╚██████╗███████╗██║  ██║╚██████╔╝██████╔╝███████╗╚██████╔╝███████║
   ╚═════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝

                    Version 0.2.0 - Built by MultiClaude Team
              Kernel Claude | Shell+FS Claude | Boss Claude

  Type 'help' for commands, 'cat /etc/motd' for welcome message
  NEW: Type 'claude' for AI assistant - ask me anything!

claude@os:/$
```

## The AI Assistant

One unique feature is the built-in AI assistant. Type `claude` in the shell to enter interactive mode:

```
claude@os:/$ claude
ClaudeOS AI Assistant - I can help with commands and system info!
Type 'exit' to return to shell, or ask me anything.

claude> how do I list files?
Use 'ls' to list files in the current directory.
Use 'ls /path' to list files in a specific directory.
Example: ls /etc

claude> exit
Goodbye!
claude@os:/$
```

## Why This Exists

This project demonstrates that AI can:
1. **Write complex low-level code** - Bootloaders, interrupt handlers, memory management
2. **Collaborate effectively** - Multiple AI instances coordinating on a shared codebase
3. **Build real, functional software** - Not just code snippets, but a complete bootable OS

## License

This project was created by Claude AI instances. Feel free to use, modify, and learn from it.

## Acknowledgments

- Built entirely by **Claude** (Anthropic's AI assistant)
- Coordinated by a human who provided the initial prompt and ran the build commands
- Inspired by OSDev.org tutorials and the x86 architecture

---

*"An operating system written by AI, for humans to learn from."*
