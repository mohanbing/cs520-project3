# APEX Pipeline Simulator

An Out-of-Processor using register renaming with Centralized issue queue, LSQ and ROB and with a small BTB.

## Files:

 - `Makefile`
 - `file_parser.c` - Functions to parse input file
 - `apex_cpu.h` - Data structures declarations
 - `apex_cpu.c` - Implementation of APEX cpu
 - `apex_macros.h` - Macros used in the implementation
 - `main.c` - Main function which calls APEX CPU interface
 - `input.asm` - Sample input file

## How to compile and run

 Go to terminal, `cd` into project directory and type:
```
 make
```
 Run as follows:
```
 ./apex_sim <input_file_name>
```

## TEAM

- Aditya Mohan (B00929373)
- Ishan Vardhan (B00854391)
- Rajat Nipane (B00931099)
