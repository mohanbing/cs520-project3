#include "apex_cpu.h"
#include <stdlib.h>
#include "phy_regs.c"

bool is_lsq_free(APEX_CPU *cpu)
{
    if(cpu->lsq_head == cpu->lsq_tail && cpu->lsq[cpu->lsq_head]!=NULL)
    {
        return FALSE;
    }
    return TRUE;
}

int add_lsq_entry(APEX_CPU *cpu, CPU_Stage *stage)
{
    cpu->lsq[cpu->lsq_tail] = (LSQ_Entry*)malloc(sizeof(LSQ_Entry));
    cpu->lsq[cpu->lsq_tail]->lsq_estd = 1;
    cpu->lsq[cpu->lsq_tail]->opcode = stage->opcode;
    strcpy(cpu->lsq[cpu->lsq_tail]->opcode_str, stage->opcode_str);
    cpu->lsq[cpu->lsq_tail]->pc = stage->pc;
    cpu->lsq[cpu->lsq_tail]->imm = stage->imm;
    
    if(stage->opcode == OPCODE_LDR || stage->opcode == OPCODE_LOAD)
    {
        cpu->lsq[cpu->lsq_tail]->load_str = 0;
        cpu->lsq[cpu->lsq_tail]->renamed_rd = stage->renamed_rd;
    }
    
    else if(stage->opcode == OPCODE_STR || stage->opcode == OPCODE_STORE)
        cpu->lsq[cpu->lsq_tail]->load_str = 1;
    
    cpu->lsq[cpu->lsq_tail]->mem_addr_valid = 1;
    cpu->lsq[cpu->lsq_tail]->mem_addr = -1;

    if(stage->renamed_rs1!=-1)
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs1 = stage->renamed_rs1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs1_value_valid = cpu->phy_regs[stage->renamed_rs1]->valid;
        if(cpu->lsq[cpu->lsq_tail]->renamed_rs1_value_valid)
            cpu->lsq[cpu->lsq_tail]->renamed_rs1_value = cpu->phy_regs[stage->renamed_rs1]->reg_value;
    }
    else
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs1_value_valid = 1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs1 = -1;
    }

    if(stage->renamed_rs2!=-1)
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs2 = stage->renamed_rs2;
        cpu->lsq[cpu->lsq_tail]->renamed_rs2_value_valid = cpu->phy_regs[stage->renamed_rs2]->valid;
        if(cpu->lsq[cpu->lsq_tail]->renamed_rs2_value_valid)
            cpu->lsq[cpu->lsq_tail]->renamed_rs2_value = cpu->phy_regs[stage->renamed_rs2]->reg_value;
    }
    else
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs2_value_valid = 1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs2 = -1;
    }

    if(stage->renamed_rs3!=-1)
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs3 = stage->renamed_rs3;
        cpu->lsq[cpu->lsq_tail]->renamed_rs3_value_valid = cpu->phy_regs[stage->renamed_rs3]->valid;
        if(cpu->lsq[cpu->lsq_tail]->renamed_rs3_value_valid)
            cpu->lsq[cpu->lsq_tail]->renamed_rs3_value = cpu->phy_regs[stage->renamed_rs3]->reg_value;
    }
    else
    {
        cpu->lsq[cpu->lsq_tail]->renamed_rs3_value_valid = 1;
        cpu->lsq[cpu->lsq_tail]->renamed_rs3 = -1;
    }

    int lsq_idx = cpu->lsq_tail;
    cpu->lsq_tail++;
    cpu->lsq_tail = cpu->lsq_tail%LSQ_SIZE;

    return lsq_idx;
}

void pickup_forwarded_values_lsq(APEX_CPU *cpu)
{
    int i;
    for(i=0; i<LSQ_SIZE; i++)
    {
        if(cpu->lsq[i]!=NULL)
        {
            LSQ_Entry *lsq_entry = cpu->lsq[i];
            if(lsq_entry->lsq_estd)
            {
                if(!lsq_entry->renamed_rs1_value_valid)
                {
                    if(cpu->forwarding_bus[lsq_entry->renamed_rs1].tag_valid == 1)
                    {
                        lsq_entry->renamed_rs1_value_valid = TRUE;
                    }
                }
                if(!lsq_entry->renamed_rs2_value_valid)
                {
                    if(cpu->forwarding_bus[lsq_entry->renamed_rs2].tag_valid == 1)
                    {
                        lsq_entry->renamed_rs2_value_valid = TRUE;
                    }
                }
                if(!lsq_entry->renamed_rs3_value_valid)
                {
                    if(cpu->forwarding_bus[lsq_entry->renamed_rs3].tag_valid == 1)
                    {
                        lsq_entry->renamed_rs3_value_valid = TRUE;
                    }
                }
            }
        }
    }
}

void set_rob_idx(APEX_CPU *cpu, int rob_idx, int lsq_idx)
{
    cpu->lsq[lsq_idx]->rob_idx = rob_idx;
}

void delete_lsq_entry(APEX_CPU *cpu)
{
    free(cpu->lsq[cpu->lsq_head]);
    cpu->lsq_head++;
    cpu->lsq_head=cpu->lsq_head%LSQ_SIZE;
}

void flush_lsq(APEX_CPU* cpu, int pc)
{
    int i=cpu->lsq_head;
    int last_modified_idx = cpu->lsq_tail;

    while(i != cpu->lsq_tail)
    {
        if(cpu->lsq[i]!=NULL)
        {
            LSQ_Entry *lsq_entry = cpu->lsq[i];
            if(lsq_entry->pc >= pc)
            {
                decrement_vcount(cpu, lsq_entry->renamed_rs1, lsq_entry->renamed_rs2, lsq_entry->renamed_rs3);

                free(cpu->lsq[i]);
                cpu->lsq[i] = NULL;
                if(cpu->lsq_head == i)
                    cpu->lsq_head = (cpu->lsq_head+1)%LSQ_SIZE;
                last_modified_idx = i;
            }
        }
        i = (i+1)%LSQ_SIZE;
    }
    cpu->lsq_tail = last_modified_idx;
}