#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apex_iq.h"
#include "forwarding_bus.c"

bool is_iq_free(APEX_CPU *cpu)
{
    if(cpu->iq_head == cpu->iq_tail && cpu->iq[cpu->iq_head]!=NULL)
    {
        return FALSE;
    }
    return TRUE;
}

int insert_iq_entry(APEX_CPU* cpu, CPU_Stage *stage) //insert in iq
{
    cpu->iq[cpu->iq_tail] = (IQ_Entry *)malloc(sizeof(IQ_Entry));
    cpu->iq[cpu->iq_tail]->allocated = 1;
    cpu->iq[cpu->iq_tail]->pc = stage->pc;
    cpu->iq[cpu->iq_tail]->opcode = stage->opcode;
    // cpu->iq_fifo.iq_entry[index].counter = iq_entry->counter;
    
    if(stage->opcode == OPCODE_MUL)
        strcpy(cpu->iq[cpu->iq_tail]->fu_type, "MulFU");
    else if(stage->opcode == OPCODE_STR || stage->opcode == OPCODE_STORE || stage->opcode == OPCODE_LOAD || stage->opcode == OPCODE_LDR)
        strcpy(cpu->iq[cpu->iq_tail]->fu_type, "LdStr");
    else if(stage->opcode == OPCODE_AND || stage->opcode == OPCODE_OR || stage->opcode == OPCODE_XOR)
        strcpy(cpu->iq[cpu->iq_tail]->fu_type, "LopFU");
    else
        strcpy(cpu->iq[cpu->iq_tail]->fu_type, "IntFU");

    cpu->iq[cpu->iq_tail]->imm = stage->imm;

    if(stage->rs1!=-1)
    {
        cpu->iq[cpu->iq_tail]->src1_tag = stage->renamed_rs1;
        if(cpu->phy_regs[stage->renamed_rs1]->valid==1)
        {
            cpu->iq[cpu->iq_tail]->src1_valid = 1;
            cpu->iq[cpu->iq_tail]->src1_value = cpu->phy_regs[stage->renamed_rs1]->reg_value;
        }
        else
        {
            cpu->iq[cpu->iq_tail]->src1_valid = 0;
        }
    }
    else
        cpu->iq[cpu->iq_tail]->src1_valid = 1;

    if(stage->rs2!=-1)
    {
        cpu->iq[cpu->iq_tail]->src2_tag = stage->renamed_rs2;
        if(cpu->phy_regs[stage->renamed_rs2]->valid==1)
        {
            cpu->iq[cpu->iq_tail]->src2_valid = 1;
            cpu->iq[cpu->iq_tail]->src2_value = cpu->phy_regs[stage->renamed_rs2]->reg_value;
        }
        else
        {
            cpu->iq[cpu->iq_tail]->src2_valid = 0;
        }
    }
    else
        cpu->iq[cpu->iq_tail]->src2_valid = 1;

    if(stage->rs3!=-1)
    {
        cpu->iq[cpu->iq_tail]->src3_tag = stage->renamed_rs3;
        if(cpu->phy_regs[stage->renamed_rs3]->valid==1)
        {
            cpu->iq[cpu->iq_tail]->src3_valid = 1;
            cpu->iq[cpu->iq_tail]->src3_value = cpu->phy_regs[stage->renamed_rs3]->reg_value;
        }
        else
        {
            cpu->iq[cpu->iq_tail]->src3_valid = 0;
        }
    }
    else
        cpu->iq[cpu->iq_tail]->src3_valid = 1;

    if(stage->rd!=-1)
        cpu->iq[cpu->iq_tail]->dst_tag = stage->renamed_rd;

    cpu->iq[cpu->iq_tail]->dispatch = *stage;

    int tail_idx = cpu->iq_tail;
    cpu->iq_tail++;
    cpu->iq_tail = cpu->iq_tail%IQ_SIZE;

    return tail_idx;
}

CPU_Stage* issue_iq(APEX_CPU* cpu, char* fu_type) // 
{
    // checks whether insn can be issued to the FU
    if(strcmp(fu_type,"MulFU")==0)
    {
        if(cpu->mul_fu1.has_insn==FALSE)
        {
            return &(cpu->mul_fu1);
        }
        return NULL;
    }

    if(strcmp(fu_type,"IntFU")==0 || strcmp(fu_type, "LdStr")==0)
    {
        if(cpu->mul_fu1.has_insn==FALSE)
        {
            return &(cpu->int_fu);
        }
        return NULL;
    }

    if(strcmp(fu_type,"LopFU")==0)
    {
        if(cpu->mul_fu1.has_insn==FALSE)
        {
            return &(cpu->lop_fu);
        }
        return NULL;
    }
    return NULL;
}

void flush_iq(APEX_CPU* cpu)
{
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        free(cpu->iq[i]);
        cpu->iq[i] = NULL;
    }
    
    cpu->iq_head = 0;
    cpu->iq_tail = 0;
}

void print_iq(APEX_CPU* cpu)
{

}

void set_rob_index(APEX_CPU *cpu, int iq_idx, int rob_idx)
{
    cpu->iq[iq_idx]->robindex = rob_idx;
}

void set_lsq_index(APEX_CPU *cpu, int iq_idx, int lsq_idx)
{
    cpu->iq[iq_idx]->lsqindex = lsq_idx;
}

void pickup_forwarded_values(APEX_CPU *cpu)
{
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        IQ_Entry *iq_entry = cpu->iq[i];
        if(iq_entry->allocated)
        {
            if(!iq_entry->src1_valid)
            {
                if(cpu->forwarding_bus[iq_entry->src1_tag].tag_valid == 1)
                {
                    iq_entry->src1_valid = TRUE;
                }
            }
            if(!iq_entry->src2_valid)
            {
                if(cpu->forwarding_bus[iq_entry->src2_tag].tag_valid == 1)
                {
                    iq_entry->src2_valid = TRUE;
                }
            }
            if(!iq_entry->src3_valid)
            {
                if(cpu->forwarding_bus[iq_entry->src3_tag].tag_valid == 1)
                {
                    iq_entry->src3_valid = TRUE;
                }
            }
        }
    }
}

void wakeup(APEX_CPU *cpu)
{
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        if(cpu->iq[i]->allocated == 1 && cpu->iq[i]->src1_valid == 1 && cpu->iq[i]->src2_valid == 1 && cpu->iq[i]->src3_valid == 1)
        {
            cpu->iq[i]->request_exec = 1;
        }
        else
        {
            cpu->iq[i]->request_exec = 0;
        }
    }
}

void selection_logic(APEX_CPU *cpu)
{
    // if(1) //TODO add selection logic on the basis of availability of FU and corresponding iq entry with request_exec == 1
    // {
    //     // set granted field in iq entry as 1 and copy the insn to the allocated FU (call function issue_iq)
    // }
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        IQ_Entry *iq_entry = cpu->iq[i];
        //check allocated
        if(iq_entry->request_exec)
        {
            CPU_Stage *fu_stage = issue_iq(cpu, iq_entry->fu_type);
            if(fu_stage!=NULL)
            {
                iq_entry->granted = TRUE;
                fu_stage = &iq_entry->dispatch;
                (*fu_stage).has_insn = TRUE;

                if(fu_stage->rd != -1 && fu_stage->opcode!=OPCODE_MUL)
                {
                    request_forwarding_bus_access(cpu, *fu_stage, iq_entry->fu_type);
                }
                // deletes entry from issue queue
                free(iq_entry);
                iq_entry = NULL;
                cpu->iq_head++;
                cpu->iq_head = cpu->iq_head % IQ_SIZE;
            }
        }
    }
}
