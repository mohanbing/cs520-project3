#include <stdio.h>
#include <stddef.h>

#include "apex_cpu.h"
#include "rob.h"

bool IsRobFree(APEX_CPU *cpu)
{
    return cpu->ROB[cpu->rob_tail] == NULL;
}

int AddRobEntry(APEX_CPU *cpu, CPU_Stage *stage, int lsq_index)
{
    ROB_ENTRY rob_entry;
    rob_entry.architectural_rd = stage->rs1;
    rob_entry.dcache_bit = 0;
    rob_entry.establised_bit = 0;
    //rob_entry.instruction_type = stage->opcode_str;
}

int DeleteRobEntry(APEX_CPU *cpu)
{
    return 0;
}

int CommitRobEntry(APEX_CPU *cpu)
{
    return 0;
}

void PrintRobContents(APEX_CPU *cpu)
{

}