#include <stdio.h>
#include<stdlib.h>
#include<string.h>

#include "apex_cpu.h";
#include "apex_iq.h";


int check_iq(APEX_CPU *cpu) // iq is free or not
{
    int i=0;
    for(i;i<No_of_IQ_Entry;i++)
    {
        if(cpu->iq_fifo.iq_entry[i].free)
        {
            cpu->iq_fifo.iq_free = i;
            return 1;
        }
    }
    return 0;
}

// iq_fifo == iq;
// iq_free == fre_entry;

int insert_iq_entry(APEX_CPU* cpu,IQ_Entry* iq_entry) //insert in iq
{
    int index;
    index = cpu->iq_fifo.iq_free;
    
    cpu->iq_fifo.iq_entry[index].pc = iq_entry->pc;
    cpu->iq_fifo.iq_entry[index].opcode_str = iq_entry->opcode_str;
    cpu->iq_fifo.iq_entry[index].counter = iq_entry->counter;
    cpu->iq_fifo.iq_entry[index].free = iq_entry->free;
    cpu->iq_fifo.iq_entry[index].fu_type = iq_entry->fu_type;
    cpu->iq_fifo.iq_entry[index].imm = iq_entry->imm;
    cpu->iq_fifo.iq_entry[index].rs1 = iq_entry->rs1;
    cpu->iq_fifo.iq_entry[index].rs2 = iq_entry->rs2;
    cpu->iq_fifo.iq_entry[index].rd = iq_entry->rd;

    cpu->iq_fifo.iq_entry[index].prs1->reg_flag = iq_entry->prs1->reg_flag;
    cpu->iq_fifo.iq_entry[index].prs1->reg_tag = iq_entry->prs1->reg_tag;
    cpu->iq_fifo.iq_entry[index].prs1->reg_value = iq_entry->prs1->reg_value;

    cpu->iq_fifo.iq_entry[index].prs2->reg_flag = iq_entry->prs2->reg_flag;
    cpu->iq_fifo.iq_entry[index].prs2->reg_tag = iq_entry->prs2->reg_tag;
    cpu->iq_fifo.iq_entry[index].prs2->reg_value = iq_entry->prs2->reg_value;

    cpu->iq_fifo.iq_entry[index].lsqindex = iq_entry->lsqindex;
    cpu->iq_fifo.iq_entry[index].robindex = iq_entry->robindex;
    return 0;
}

int issue_iq(APEX_CPU* cpu,char* fu_type) // 
{
    // forward this to MulFU
    if(strcmp(fu_type,"MulFU"))
    {
        
    }

    // forward this to IntFU
    if(strcmp(fu_type,"IntFU"))
    {

    }

    // forward this to LopFU
    if(strcmp(fu_type,"LopFU"))
    {

    }
}

int flush_iq(APEX_CPU* cpu)
{
    
}

void print_iq(APEX_CPU* cpu)
{

}
