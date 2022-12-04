#ifndef _PHY_REGS_
#define _PHY_REGS_

#include "apex_cpu.h"

static void
add_phy_reg_free_list(APEX_CPU *cpu)
{
    int tag;
    for(tag=0; tag<PHY_REG_FILE_SIZE; tag++)
    {
        if(cpu->phy_regs[tag]->renamed_bit && cpu->phy_regs[tag]->vCount == 0 && cpu->phy_regs[tag]->cCount == 0)
        {
            cpu->phy_regs[tag]->reg_flag = -1;
            cpu->phy_regs[tag]->reg_value = 0;
            cpu->phy_regs[tag]->reg_tag = tag;
            cpu->phy_regs[tag]->renamed_bit = 0;
            cpu->phy_regs[tag]->vCount = 0;
            cpu->phy_regs[tag]->cCount = 0;
            cpu->phy_regs[tag]->valid = 0;
            cpu->free_list[cpu->free_list_tail] = tag;
            cpu->free_list_tail++;
            cpu->free_list_tail = cpu->free_list_tail%PHY_REG_FILE_SIZE;
        }
    }    
}

static void
rename_table_assign_free_reg(APEX_CPU *cpu, int rd)
{
    if(cpu->free_list_head == cpu->free_list_tail)
    {
        if(cpu->free_list[cpu->free_list_head] == -1)
        {
            cpu->rename_stall = 1;
            return;
        }
    }

    cpu->rename_stall = 0;
    cpu->phy_regs[cpu->rename_table[rd]]->renamed_bit = 1;
    cpu->rename_table[rd] = cpu->free_list[cpu->free_list_head];
    cpu->free_list[cpu->free_list_head] = -1;
    cpu->free_list_head++;
    cpu->free_list_head = cpu->free_list_head%PHY_REG_FILE_SIZE;
}

#endif