#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apex_iq.h"
#include "forwarding_bus.c"
#include "phy_regs.c"

static bool is_iq_free(APEX_CPU *cpu)
{
    if(cpu->iq_tail == IQ_SIZE)
    {
        return FALSE;
    }
    return TRUE;
}

static int insert_iq_entry(APEX_CPU* cpu, CPU_Stage *stage) //insert in iq
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

    return cpu->iq_tail++;
}

static
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

    if(strcmp(fu_type,"IntFU")==0)
    {
        if(cpu->int_fu.has_insn==FALSE)
        {
            return &(cpu->int_fu);
        }
        return NULL;
    }

    if(strcmp(fu_type,"LopFU")==0)
    {
        if(cpu->lop_fu.has_insn==FALSE)
        {
            return &(cpu->lop_fu);
        }
        return NULL;
    }
    return NULL;
}

void flush_iq(APEX_CPU* cpu, int pc)
{
    int i;
    IQ_Entry *temp_arr[IQ_SIZE];
    for(i=0; i<IQ_SIZE; i++)
        temp_arr[i]=NULL;

    int temp_idx=0;

    for(i=0; i<IQ_SIZE; i++)
    {
        if(cpu->iq[i]!=NULL)
        {
            IQ_Entry *iq_entry = cpu->iq[i];
            if(cpu->iq[i]->pc >= pc)
            {
                decrement_ccount(cpu, iq_entry->dst_tag, iq_entry->opcode);

                free(cpu->iq[i]);
                cpu->iq[i] = NULL;
            }
        }
    }

    for(i=0; i<IQ_SIZE; i++)
    {
        if(cpu->iq[i]!=NULL)
        {
            temp_arr[temp_idx] = cpu->iq[i];
            temp_idx++;
        }
    }

    for(i=0; i<IQ_SIZE; i++)
    {
        if(temp_arr[i]!=NULL)
        {
            cpu->iq[i] = temp_arr[i];
        }
        else
        {
            cpu->iq[i] = NULL;
        }
    }
    
    cpu->iq_head = 0;
    cpu->iq_tail = temp_idx;
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
        if(cpu->iq[i]!=NULL)
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
}

static void wakeup(APEX_CPU *cpu)
{
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        if(cpu->iq[i]!=NULL)
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
}

static void
selection_logic(APEX_CPU *cpu)
{
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        if(cpu->iq[i]!=NULL && cpu->iq[i]->allocated==1)
        {
            IQ_Entry *iq_entry = cpu->iq[i];
            if(iq_entry->request_exec)
            {
                CPU_Stage *fu_stage = issue_iq(cpu, iq_entry->fu_type);
                if(fu_stage!=NULL)
                {
                    iq_entry->granted = TRUE;
                    *fu_stage = iq_entry->dispatch;
                    (*fu_stage).has_insn = TRUE;
                    char fu_type[10];
                    strcpy(fu_type, iq_entry->fu_type);

                    decrement_ccount(cpu, fu_stage->renamed_rd, fu_stage->opcode);

                    if(fu_stage->rd != -1 && fu_stage->opcode!=OPCODE_MUL)
                    {
                        request_forwarding_bus_access(cpu, fu_stage->renamed_rd, fu_type);
                    }
                    
                    // deletes entry from issue queue after swapping
                    free(iq_entry);
                    cpu->iq[i]=NULL;
                }
            }
        }
    }
    IQ_Entry *temp[IQ_SIZE];
    int j=0;
    for(j=0; j<IQ_SIZE; j++)
    {
        temp[j]=NULL;
    }
    int idx=0;
    for(j=0; j<IQ_SIZE; j++)
    {
        if(cpu->iq[j]!=NULL)
        {
            temp[idx] = cpu->iq[j];
            idx++;
        }
    }

    for(j=0; j<IQ_SIZE; j++)
    {
        cpu->iq[j] = temp[j];
    }
    cpu->iq_head = 0;
    cpu->iq_tail = idx;
}
