#include <stdio.h>
#include <stddef.h>

#include "apex_cpu.h"
#include "rob.h"

bool IsRobFree(APEX_CPU *cpu)
{
    return cpu->rob[cpu->rob_tail] == NULL;
}

int AddRobEntry(APEX_CPU *cpu, CPU_Stage *stage, int lsq_index)
{
    ROB_ENTRY rob_entry;
    rob_entry.pc = stage->pc;
    rob_entry.architectural_rd = stage->rs1;
    rob_entry.physical_rd = stage->renamed_rs1;
    rob_entry.dcache_bit = 0;
    rob_entry.establised_bit = 1;
    rob_entry.instruction_type = stage->opcode; //store opcode value
    rob_entry.lsq_index = lsq_index;//shouldn't this be lsq tail?

    int rob_idx = cpu->rob_tail;
    //add at the tail and inrement tail
    cpu->rob[cpu->rob_tail] = &rob_entry;
    cpu->rob_tail = (cpu->rob_tail + 1) % ROB_SIZE;
    return rob_idx; 
}

int DeleteRobEntry(APEX_CPU *cpu)
{
    //delete from head and increment head
    cpu->rob[cpu->rob_head] = NULL;
    cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;
}

int CommitRobEntry(APEX_CPU *cpu)
{
    //commit the entry from head to maintain program order
    int opcode = cpu->rob[cpu->rob_head]->instruction_type;
    if
    (  
        opcode != OPCODE_STORE ||
        opcode != OPCODE_STR ||
        opcode != OPCODE_BNZ ||
        opcode != OPCODE_BZ ||
        opcode != OPCODE_NOP ||
        opcode != OPCODE_HALT ||
        opcode != OPCODE_JUMP 
    )
    {
        int arch_regiter_index = cpu->rob[cpu->rob_head]->architectural_rd;
        int arch_register_value = cpu->phy_regs[cpu->rob[cpu->rob_head]->physical_rd];
        cpu->arch_regs[arch_regiter_index] = arch_register_value;
    }
    
    //handle compare to update z flag
    //return 1 

    //handle halt

    //handle STORE/STR operations
      //check if LSQ entry is valid from LSQ index and head of LSQ is STR/STORE
      //perform a memory update via D-Cache
    

    return 0;
}

void PrintRobContents(APEX_CPU *cpu)
{

}