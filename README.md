# UAConsole

Single-file OPC UA server console browser.

## Quick Start

```bash
# Install dependency
sudo apt-get install libopen62541-dev

# Compile
gcc -o uaconsole uaconsole.c -lopen62541 -lm

# Run
./uaconsole opc.tcp://10.0.0.128:4840

