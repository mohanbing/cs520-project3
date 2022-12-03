#include<stdio.h>
#include "dcache.h"
#include "phy_regs.c"
#include "forwarding_bus.h"

int CommitLoadStoreInstr(APEX_CPU *cpu)
{
    if (cpu->dcache.has_insn)
    {
        switch (cpu->dcache.opcode)
        {
            case OPCODE_LDR:
                /* code */
                cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value + cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value;
                cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value = cpu->data_memory[cpu->dcache.memory_address];
                // cpu->arch_regs[cpu->dcache.rd] = cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value;
                //data forwarding
                request_forwarding_bus_access(cpu, cpu->dcache.renamed_rd, "dcache");
                //free the physical register rs1 and rs2
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs1);
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs2);
                break;
            
            case OPCODE_LOAD:
                /* code */
                cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value + cpu->dcache.imm;
                cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value = cpu->data_memory[cpu->dcache.memory_address];
                // cpu->arch_regs[cpu->dcache.rd] = cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value;
                //data forwarding
                request_forwarding_bus_access(cpu, cpu->dcache.renamed_rd, "dcache");
                //free the physical register rs1
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs1);
                break;

            case OPCODE_STORE:
                cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value + cpu->dcache.imm;
                cpu->data_memory[cpu->dcache.memory_address] = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
                //free physical rs1, rs2
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs1);
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs2);
                break;

            case OPCODE_STR:
                cpu->dcache.memory_address = cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value + cpu->phy_regs[cpu->dcache.renamed_rs3]->reg_value;
                cpu->data_memory[cpu->dcache.memory_address] = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
                //free physical rs1, rs2 and rs3
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs1);
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs2);
                add_phy_reg_free_list(cpu, cpu->dcache.renamed_rs3);
                break;
            
            default:
                break;
        }
    }    
    cpu->dcache.has_insn = 0;
}