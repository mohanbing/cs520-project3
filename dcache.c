#include<stdio.h>
#include"dcache.h"

int CommitLoadStoreInstr(APEX_CPU *cpu)
{
    switch (cpu->dcache.opcode)
    {
        case OPCODE_LDR:
            /* code */
            cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value + cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value;
            cpu->phy_regs[cpu->dcache.renamed_rd] = cpu->data_memory[cpu->dcache.memory_address];
            cpu->arch_regs[cpu->dcache.rd] = cpu->phy_regs[cpu->dcache.renamed_rd];
            //data forwarding
            //free the physical register rs1 and rs2
            break;
        
        case OPCODE_LOAD:
            /* code */
            cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value + cpu->dcache.imm;
            cpu->phy_regs[cpu->dcache.renamed_rd] = cpu->data_memory[cpu->dcache.memory_address];
            cpu->arch_regs[cpu->dcache.rd] = cpu->phy_regs[cpu->dcache.renamed_rd];
            //data forwarding
            //free the physical register rs1
            break;

        case OPCODE_STORE:
            cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value + cpu->dcache.imm;
            cpu->data_memory[cpu->dcache.memory_address] = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
            //free physical rs1, rs2
            break;

        case OPCODE_STR:
            cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value + cpu->phy_regs[cpu->dcache.renamed_rs3]->reg_value;
            cpu->data_memory[cpu->dcache.memory_address] = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
            //free physical rs1, rs2 and rs3
            break;
        
        default:
            break;
    }
    cpu->dcache.has_insn = 0;
}