/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"

int
main(int argc, char const *argv[])
{
    APEX_CPU *cpu;
    const char *mode;
    int sim_mode=0;
    int cycles=INT_MAX;

    fprintf(stderr, "APEX CPU Pipeline Simulator\n");

    cpu = APEX_cpu_init(argv[1]);
    if (!cpu)
    {
        fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
        exit(1);
    }

    if(argc ==3)
    {
        mode = argv[2];
        if(strcmp(mode, "simulate")==0)
        {
            sim_mode = 1;
            cpu->single_step = 0;
            if(argc!=4)
            {
                fprintf(stderr, "APEX_Help: Usage %s <number of cycles>\n", argv[0]);
                exit(1);
            }
            cycles = atoi(argv[3]);
            if(!cycles)
            {
                fprintf(stderr, "APEX_Help: Usage %s <number of cycles>\n", argv[0]);
                exit(1);
            }
        }

        else if(strcmp(mode, "display")==0)
        {
            sim_mode = 2;
            cpu->single_step = 0;
            if(argc!=4)
            {
                fprintf(stderr, "APEX_Help: Usage %s <number of cycles>\n", argv[0]);
                exit(1);
            }
            cycles = atoi(argv[3]);
            if(!cycles)
            {
                fprintf(stderr, "APEX_Help: Usage %s <number of cycles>\n", argv[0]);
                exit(1);
            }
        }
    }

    APEX_cpu_run(cpu, sim_mode, cycles);
    APEX_cpu_stop(cpu);
    return 0;
}