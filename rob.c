#include <stdio.h>
#include <stddef.h>
#include "rob.h"
#include "phy_regs.c"

bool IsRobFree(APEX_CPU *cpu)
{
    return cpu->rob[cpu->rob_tail] == NULL;
}

void flush_rob(APEX_CPU* cpu, int pc)
{
    int i=cpu->rob_head;
    int last_modified_idx = cpu->rob_tail;

    while(i != cpu->rob_tail)
    {
        if(cpu->rob[i]!=NULL)
        {
            ROB_ENTRY *rob_entry = cpu->rob[i];
            if(rob_entry->pc >= pc)
            {
                free(cpu->rob[i]);
                cpu->rob[i] = NULL;
                if(cpu->rob_head == i)
                    cpu->rob_head = (cpu->rob_head+1)%ROB_SIZE;
                last_modified_idx = i;
            }
        }
        i = (i+1)%ROB_SIZE;
    }
    cpu->rob_tail = last_modified_idx;
}


int AddRobEntry(APEX_CPU *cpu, CPU_Stage *stage, int lsq_index)
{
    ROB_ENTRY *rob_entry = (ROB_ENTRY*)malloc(sizeof(ROB_ENTRY));
    rob_entry->pc = stage->pc;
    rob_entry->architectural_rd = stage->rd;
    rob_entry->physical_rd = stage->renamed_rd;
    rob_entry->dcache_bit = 0;
    rob_entry->establised_bit = 1;
    rob_entry->instruction_type = stage->opcode; //store opcode value
    rob_entry->lsq_index = lsq_index;//shouldn't this be lsq tail?
    rob_entry->prev_physical_rd = stage->prev_renamed_rd;

    int rob_idx = cpu->rob_tail;
    //add at the tail and inrement tail
    cpu->rob[cpu->rob_tail] = rob_entry;
    cpu->rob_tail = (cpu->rob_tail + 1) % ROB_SIZE;
    return rob_idx;     
}

int DeleteRobEntry(APEX_CPU *cpu)
{
    //delete from head and increment head
    cpu->insn_completed++;
    free(cpu->rob[cpu->rob_head]);
    cpu->rob[cpu->rob_head] = NULL;
    cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;

    return 1;
}

int CommitRobEntry(APEX_CPU *cpu)
{
    if(cpu->rob[cpu->rob_head]==NULL)
        return 0;
    //commit the entry from head to maintain program order
    int opcode = cpu->rob[cpu->rob_head]->instruction_type;
    if
    (  
        opcode != OPCODE_STORE &&
        opcode != OPCODE_STR &&
        opcode != OPCODE_BNZ &&
        opcode != OPCODE_BZ &&
        opcode != OPCODE_NOP &&
        opcode != OPCODE_HALT &&
        opcode != OPCODE_JUMP 
    )
    {
        if(cpu->phy_regs[cpu->rob[cpu->rob_head]->physical_rd]->valid)
        {

            int arch_regiter_index = cpu->rob[cpu->rob_head]->architectural_rd;
            int arch_register_value = cpu->phy_regs[cpu->rob[cpu->rob_head]->physical_rd]->reg_value;
            cpu->arch_regs[arch_regiter_index] = arch_register_value;        
            DeleteRobEntry(cpu);
        }        
    }
    else if (opcode == OPCODE_STORE || opcode == OPCODE_STR || opcode == OPCODE_BNZ ||
             opcode == OPCODE_JUMP || opcode == OPCODE_BZ)
    {
        DeleteRobEntry(cpu);
    }

    add_phy_reg_free_list(cpu);

    //handle halt
    //return 1 or 0?
    if(opcode == OPCODE_HALT)
    {
        return 1;
        //DeleteRobEntry(cpu);
    }

    //handle LOAD/LDR and STORE/STR operations
      //check if LSQ entry is valid from LSQ index and head of LSQ is LOAD/LDR and STORE/STR
      //perform a memory update via D-Cache
    int lsq_index=-1;
    if(cpu->rob[cpu->rob_head]!=NULL)
        lsq_index = cpu->rob[cpu->rob_head]->lsq_index;
    
    if(
        lsq_index!=-1 &&
        cpu->rob[cpu->rob_head]->pc == cpu->lsq[lsq_index]->pc &&
        cpu->lsq[lsq_index]->mem_addr_valid && cpu->lsq[lsq_index]->renamed_rs1_value_valid && 
        cpu->lsq[lsq_index]->renamed_rs2_value_valid && cpu->lsq[lsq_index]->renamed_rs3_value_valid        
    )
    {
        // cpu->dcache_entry = cpu->lsq[lsq_index];
        //populate the entries in cpu stage.        
        cpu->dcache.pc = cpu->lsq[lsq_index]->pc;
        strcpy(cpu->dcache.opcode_str, cpu->lsq[lsq_index]->opcode_str);

        cpu->dcache.rs1 = cpu->lsq[lsq_index]->arch_rs1;
        cpu->dcache.rs2 = cpu->lsq[lsq_index]->arch_rs2;
        cpu->dcache.rs3 = cpu->lsq[lsq_index]->arch_rs3;
        cpu->dcache.rd = cpu->lsq[lsq_index]->arch_rd;

        cpu->dcache.opcode = cpu->lsq[lsq_index]->opcode;
        cpu->dcache.renamed_rs1 = cpu->lsq[lsq_index]->renamed_rs1;
        cpu->dcache.renamed_rs2 = cpu->lsq[lsq_index]->renamed_rs2;
        cpu->dcache.renamed_rs3 = cpu->lsq[lsq_index]->renamed_rs3;
        cpu->dcache.renamed_rd = cpu->lsq[lsq_index]->renamed_rd;
        cpu->dcache.imm = cpu->lsq[lsq_index]->imm;
        cpu->dcache.memory_address = cpu->lsq[lsq_index]->mem_addr;
        cpu->dcache.has_insn = 1;
        cpu->dcache.rd = cpu->rob[cpu->rob_head]->architectural_rd;

        free(cpu->lsq[lsq_index]);
        cpu->lsq[lsq_index] = NULL;

        // DeleteRobEntry(cpu);
        // add_phy_reg_free_list(cpu, cpu->rob[cpu->rob_head]->physical_rd);
    }   
    
    return 0;
}

void PrintRobContents(APEX_CPU *cpu)
{
}