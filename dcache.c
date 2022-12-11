#include<stdio.h>
#include "dcache.h"
#include "phy_regs.c"
#include "forwarding_bus.h"

int CommitLoadStoreInstr(APEX_CPU *cpu)
{
    if (cpu->dcache.has_insn)
    {
        if(cpu->dcache.renamed_rs1 != -1)
        {
            if(cpu->phy_regs[cpu->dcache.renamed_rs1]->valid)
                cpu->dcache.rs1_value = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
            else
                cpu->dcache.rs1_value = cpu->forwarding_bus[cpu->dcache.renamed_rs1].data_value;
        }

        if(cpu->dcache.renamed_rs2 != -1)
        {
            if(cpu->phy_regs[cpu->dcache.renamed_rs2]->valid)
                cpu->dcache.rs2_value = cpu->phy_regs[cpu->dcache.renamed_rs2]->reg_value;
            else
                cpu->dcache.rs2_value = cpu->forwarding_bus[cpu->dcache.renamed_rs2].data_value;
        }

        if(cpu->dcache.renamed_rs3 != -1)
        {
            if(cpu->phy_regs[cpu->dcache.renamed_rs3]->valid)
                cpu->dcache.rs3_value = cpu->phy_regs[cpu->dcache.renamed_rs3]->reg_value;
            else
                cpu->dcache.rs3_value = cpu->forwarding_bus[cpu->dcache.renamed_rs3].data_value;
        }

        decrement_vcount(cpu, cpu->dcache.renamed_rs1, cpu->dcache.renamed_rs2, cpu->dcache.renamed_rs3);

        switch (cpu->dcache.opcode)
        {
            case OPCODE_LDR:
                /* code */
                cpu->dcache.memory_address = cpu->dcache.rs1_value + cpu->dcache.rs2_value;
                cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value = cpu->data_memory[cpu->dcache.memory_address];
                cpu->phy_regs[cpu->dcache.renamed_rd]->valid = 1;
                // cpu->arch_regs[cpu->dcache.rd] = cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value;
                //data forwarding
                request_forwarding_bus_access(cpu, cpu->dcache.renamed_rd, "dcache");
                break;
            
            case OPCODE_LOAD:
                /* code */
                cpu->dcache.memory_address = cpu->dcache.rs1_value + cpu->dcache.imm;
                cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value = cpu->data_memory[cpu->dcache.memory_address];
                cpu->phy_regs[cpu->dcache.renamed_rd]->valid = 1;
                // cpu->arch_regs[cpu->dcache.rd] = cpu->phy_regs[cpu->dcache.renamed_rd]->reg_value;
                //data forwarding
                request_forwarding_bus_access(cpu, cpu->dcache.renamed_rd, "dcache");
                break;

            case OPCODE_STORE:
                cpu->dcache.memory_address = cpu->dcache.rs2_value + cpu->dcache.imm;
                cpu->data_memory[cpu->dcache.memory_address] = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
                break;

            case OPCODE_STR:
                cpu->dcache.memory_address = cpu->dcache.rs2_value + cpu->dcache.rs3_value;
                cpu->data_memory[cpu->dcache.memory_address] = cpu->phy_regs[cpu->dcache.renamed_rs1]->reg_value;
                break;
            
            default:
                break;
        }
    }

    //moved has_insn = FALSE to dcache in apex_cpu.c
    return 0;
}