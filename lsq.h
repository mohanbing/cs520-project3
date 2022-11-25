#include "apex_cpu.h"
#include <stdlib.h>

bool is_lsq_free(APEX_CPU *cpu)
{
    if(cpu->lsq_head == cpu->lsq_tail && cpu->lsq[cpu->lsq_head]!=NULL)
    {
        return FALSE;
    }
    return TRUE;
}

void add_lsq_entry(APEX_CPU *cpu, CPU_Stage *stage)
{
    cpu->lsq[cpu->lsq_tail] = (LSQ_Entry*)malloc(sizeof(LSQ_Entry));
    cpu->lsq[cpu->lsq_tail]->lsq_estd = 1;
    cpu->lsq[cpu->lsq_tail]->opcode = stage->opcode;
    cpu->lsq[cpu->lsq_tail]->pc = stage->pc;
    
    if(stage->opcode == OPCODE_LDR || stage->opcode == OPCODE_LOAD)
    {
        cpu->lsq[cpu->lsq_tail]->load_str = 0;
        cpu->lsq[cpu->lsq_tail]->renamed_rd = stage->renamed_rd;
    }
    
    else if(stage->opcode == OPCODE_STR || stage->opcode == OPCODE_STORE)
        cpu->lsq[cpu->lsq_tail]->load_str = 1;
    
    cpu->lsq[cpu->lsq_tail]->mem_addr_valid = 0;
    cpu->lsq[cpu->lsq_tail]->mem_addr = -1;

    if(stage->renamed_rs1!=-1)
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs1 = stage->renamed_rs1;
    }
    else
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs1_value_valid = 1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs1 = -1;
    }

    if(stage->renamed_rs2!=-1)
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs2 = stage->renamed_rs2;
    }
    else
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs2_value_valid = 1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs2 = -1;
    }

    if(stage->renamed_rs3!=-1)
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs3 = stage->renamed_rs3;
    }
    else
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs3_value_valid = 1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs3 = -1;
    }

    cpu->lsq_tail++;
    cpu->lsq_tail = cpu->lsq_tail%LSQ_SIZE;
}

void set_rob_idx(APEX_CPU *cpu, int rob_idx, int lsq_idx)
{
    cpu->lsq[lsq_idx]->rob_idx = rob_idx;
}

void delete_lsq_entry(APEX_CPU *cpu)
{
    free(cpu->lsq[cpu->lsq_head]);
    cpu->lsq_head++;
    cpu->lsq_head%LSQ_SIZE;
}