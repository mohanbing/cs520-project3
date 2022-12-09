/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsq.h"
#include "rob.c"
#include "apex_iq.c"
#include "dcache.c"
#include "btb.c"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_data_memory(APEX_CPU *cpu)
{
    printf("================STATE OF DATA MEMORY (Printing only non-zero memory locations)================\n");
    int i;
    for(i=0; i<4000; i++)
    {
        if(cpu->data_memory[i]!=0)
            printf("| \t MEM[%d] \t | \t Data Value = %d \t |\n", i, cpu->data_memory[i]);
    }
}

static void
print_rename_table(APEX_CPU *cpu)
{
    printf("================RENAME TABLE================\n");
    int i;
    for(i=0; i<ARCH_REG_FILE_SIZE; i++)
    {
        printf("| \t ARCH REG [%d] \t | \t PHY REG = %d \t |\n", i, cpu->rename_table[i]);
    }
}

static void
print_free_list(APEX_CPU *cpu)
{
    printf("================FREE LIST================\n");
    int i;
    for(i=0; i<PHY_REG_FILE_SIZE; i++)
    {
        if(i==cpu->free_list_head && cpu->free_list_head == cpu->free_list_tail)
        {
            printf("HEAD TAIL---> %d\n", cpu->free_list[i]);
        }
        else if(i==cpu->free_list_head)
        {
            printf("HEAD ---> %d\n", cpu->free_list[i]);
        }
        else if(i==cpu->free_list_tail)
        {
            printf("TAIL ---> %d\n", cpu->free_list[i]);
        }
        else
            printf("%d\n", cpu->free_list[i]);
    }
}

static void
print_fwd_bus(APEX_CPU *cpu)
{
    printf("================FORWARDING BUS================\n");
    int i;
    for(i=0; i<PHY_REG_FILE_SIZE; i++)
    {
        printf("IDX: [%d] TAG VALID: [%d] DATA VALUE [%d]\n", i, cpu->forwarding_bus[i].tag_valid, cpu->forwarding_bus[i].data_value);
    }
}

static void
print_issue_q_entries(APEX_CPU *cpu)
{
    printf("================ISSUE QUEUE================\n");
    int i;
    for(i=0; i<IQ_SIZE; i++)
    {
        if(cpu->iq[i]!=NULL)
            printf("[ I%d ]", ((cpu->iq[i]->pc-4000)/4)+1);
    }
    printf("\n");
}

static void
print_rob_entries(APEX_CPU *cpu)
{
    printf("================ROB================\n");
    int i;
    for(i=0; i<ROB_SIZE; i++)
    {
        if(cpu->rob[i]!=NULL)
            printf("[ I%d ]", ((cpu->rob[i]->pc-4000)/4)+1);
    }
    printf("\n");
}

static void
print_lsq_entries(APEX_CPU *cpu)
{
    printf("================LSQ================\n");
    int i;
    for(i=0; i<LSQ_SIZE; i++)
    {
        if(cpu->lsq[i]!=NULL)
            printf("[ I%d ]", ((cpu->lsq[i]->pc-4000)/4)+1);
    }
    printf("\n");
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_ADDL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_SUB:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_SUBL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_MUL:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
        case OPCODE_DIV:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
        case OPCODE_AND:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
        case OPCODE_OR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_LDR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_STR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rs3, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_BZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }
        case OPCODE_BNZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1,
                   stage->imm);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    if(stage!=NULL)
    {
        printf("%-15s: I%d: (%d) ", name, ((stage->pc-4000)/4)+1,stage->pc);
        print_instruction(stage);
    }
    else
    {
        printf("%-15s: EMPTY", name);
    }
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_arch_reg_file(const APEX_CPU *cpu)
{
    printf("----------\n%s\n----------\n", "State of Architectural Register File");
    int i;
    for (i = 0; i < ARCH_REG_FILE_SIZE ; ++i)
    {
        printf("R%-3d\tValue:[%-3d]", i, cpu->arch_regs[i]);
        printf("\n");
    }
}

static void
print_phy_reg_file(const APEX_CPU *cpu)
{
    printf("----------\n%s\n----------\n", "State of Physical Register File");
    int i;
    for (i = 0; i < PHY_REG_FILE_SIZE ; ++i)
    {
        printf("P%-3d\tValue:[%-3d] \t Flag: [%d] \t Valid: [%d]", i, cpu->phy_regs[i]->reg_value, cpu->phy_regs[i]->reg_flag, 
                    cpu->phy_regs[i]->valid);
        printf("\n");
    }
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;        

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.rs3 = current_ins->rs3;
        cpu->fetch.imm = current_ins->imm;

        cpu->fetch.renamed_rd = -1;
        cpu->fetch.renamed_rs1 = -1;
        cpu->fetch.renamed_rs2 = -1;
        cpu->fetch.renamed_rs3 = -1;

        //probe BTB table to find a branch
        BTB_ENTRY *probedEntry = ProbeBtb(cpu, cpu->pc);

        /* Update PC for next instruction */
        if(probedEntry != NULL)
        {
            cpu->pc = probedEntry->target_pc;
        }
        else
        {
            cpu->pc += 4;
        }        

        /* Copy data from fetch latch to decode latch*/
        if(cpu->decode_rename1.has_insn==0)
            cpu->decode_rename1 = cpu->fetch;        

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
}

/*
 * Decode/Rename1 Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode_rename1(APEX_CPU *cpu)
{
    int insn_renamed = 0;
    if (cpu->decode_rename1.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode_rename1.opcode)
        {
            case OPCODE_ADD:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;

                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;

                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_ADDL:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_LOAD:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    insn_renamed = 1;
                }
                break;
            }

            case OPCODE_LDR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_STORE:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                insn_renamed = 1;

                break;
            }

            case OPCODE_STR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs3==-1)
                {
                    cpu->decode_rename1.renamed_rs3 = cpu->rename_table[cpu->decode_rename1.rs3];
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs3]->valid == 1)
                        cpu->decode_rename1.rs3_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs3]->reg_value;
                }

                insn_renamed = 1;
                break;
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    insn_renamed = 1;
                }
                break;
            }

            case OPCODE_SUB:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_SUBL:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_MUL:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_DIV:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }

                break;
            }

            case OPCODE_CMP:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }
                
                break;
            }

            case OPCODE_AND:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }
                
                break;
            }

            case OPCODE_OR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }
                
                break;
            }

            case OPCODE_XOR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->valid == 1)
                        cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                }        
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                {
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                    cpu->zero_flag = cpu->decode_rename1.renamed_rd;
                    insn_renamed = 1;
                }
                
                break;
            }

            case OPCODE_JUMP:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                    cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->vCount++;
                    if(cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->valid == 1)
                        cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                    insn_renamed=1;
                }
                break;
            }

            case OPCODE_NOP:
            {
                insn_renamed=1;
                break;
            }

            case OPCODE_BZ:
            {
                cpu->phy_regs[cpu->zero_flag]->cCount++;
                insn_renamed=1;
                break;
            }

            case OPCODE_BNZ:
            {
                cpu->phy_regs[cpu->zero_flag]->cCount++;
                insn_renamed=1;
                break;
            }

            case OPCODE_HALT:
            {
                insn_renamed=1;
                break;
            }
        }

        /* Copy data from decode latch to execute latch*/
        if(cpu->rename2_dispatch.has_insn == 0 && insn_renamed==1)
        {
            cpu->rename2_dispatch = cpu->decode_rename1;
            cpu->decode_rename1.has_insn = FALSE;
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/Rename1", &cpu->decode_rename1);
        }
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/Rename1", NULL);
        }
    }
}

/*
 * Rename2/Dispatch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_rename2_dispatch(APEX_CPU *cpu)
{
    if(cpu->rename2_dispatch.has_insn==TRUE)
    {
        if(cpu->rename2_dispatch.renamed_rs1 != -1)
        {
            if(cpu->phy_regs[cpu->rename2_dispatch.renamed_rs1]->valid)
                cpu->rename2_dispatch.rs1_value = cpu->phy_regs[cpu->rename2_dispatch.renamed_rs1]->reg_value;
        }

        if(cpu->rename2_dispatch.renamed_rs2 != -1)
        {
            if(cpu->phy_regs[cpu->rename2_dispatch.renamed_rs2]->valid)
                cpu->rename2_dispatch.rs2_value = cpu->phy_regs[cpu->rename2_dispatch.renamed_rs2]->reg_value;
        }

        if(cpu->rename2_dispatch.renamed_rs3 != -1)
        {
            if(cpu->phy_regs[cpu->rename2_dispatch.renamed_rs3]->valid)
                cpu->rename2_dispatch.rs3_value = cpu->phy_regs[cpu->rename2_dispatch.renamed_rs3]->reg_value;
        }
            
        if(cpu->rename2_dispatch.opcode == OPCODE_LDR ||
            cpu->rename2_dispatch.opcode == OPCODE_LOAD ||
            cpu->rename2_dispatch.opcode == OPCODE_STORE ||
            cpu->rename2_dispatch.opcode == OPCODE_STR)
        {
            if(is_iq_free(cpu) && is_lsq_free(cpu) && IsRobFree(cpu))
            {
                int lsq_idx = add_lsq_entry(cpu, &cpu->rename2_dispatch);
                int rob_idx = AddRobEntry(cpu, &cpu->rename2_dispatch, lsq_idx);
                set_rob_idx(cpu, rob_idx, lsq_idx);
                cpu->rename2_dispatch.has_insn = FALSE;
            }
        }
        else
        {
            if(is_iq_free(cpu) && IsRobFree(cpu))
            {
                int issue_q_idx = insert_iq_entry(cpu, &cpu->rename2_dispatch);
                int rob_idx = AddRobEntry(cpu, &cpu->rename2_dispatch, -1);
                cpu->rename2_dispatch.has_insn = FALSE;
            }
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Rename2/Dispatch", &cpu->rename2_dispatch);
        }
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Rename2/Dispatch", NULL);
        }
    }
}

static void
APEX_int_fu(APEX_CPU *cpu)
{
    if(cpu->int_fu.has_insn==TRUE)
    {
        // pickup values from frwding bus
        if(cpu->int_fu.renamed_rs1 != -1)
        {
            if(cpu->phy_regs[cpu->int_fu.renamed_rs1]->valid)
                cpu->int_fu.rs1_value = cpu->phy_regs[cpu->int_fu.renamed_rs1]->reg_value;
            else
                cpu->int_fu.rs1_value = cpu->forwarding_bus[cpu->int_fu.renamed_rs1].data_value;
        }

        if(cpu->int_fu.renamed_rs2 != -1)
        {
            if(cpu->phy_regs[cpu->int_fu.renamed_rs2]->valid)
                cpu->int_fu.rs2_value = cpu->phy_regs[cpu->int_fu.renamed_rs2]->reg_value;
            else
                cpu->int_fu.rs2_value = cpu->forwarding_bus[cpu->int_fu.renamed_rs2].data_value;
        }

        if(cpu->int_fu.opcode == OPCODE_ADD)
        {
            cpu->int_fu.result_buffer = cpu->int_fu.rs1_value + cpu->int_fu.rs2_value;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_value = cpu->int_fu.result_buffer;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->valid = 1;

            if(cpu->int_fu.result_buffer == 0)
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 0;
        }
        else if(cpu->int_fu.opcode == OPCODE_ADDL)
        {
            cpu->int_fu.result_buffer = cpu->int_fu.rs1_value + cpu->rename2_dispatch.imm;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_value = cpu->int_fu.result_buffer;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->valid = 1;

            if(cpu->int_fu.result_buffer == 0)
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 0;
        }
        else if(cpu->int_fu.opcode == OPCODE_SUB)
        {
            cpu->int_fu.result_buffer = cpu->int_fu.rs1_value - cpu->int_fu.rs2_value;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_value = cpu->int_fu.result_buffer;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->valid = 1;

            if(cpu->int_fu.result_buffer == 0)
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 0;
        }
        else if(cpu->int_fu.opcode == OPCODE_SUBL)
        {
            cpu->int_fu.result_buffer = cpu->int_fu.rs1_value - cpu->rename2_dispatch.imm;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_value = cpu->int_fu.result_buffer;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->valid = 1;

            if(cpu->int_fu.result_buffer == 0)
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_flag = 0;
        }
        else if(cpu->int_fu.opcode == OPCODE_BZ)
        {
            if (cpu->phy_regs[cpu->zero_flag]->reg_flag == 1)
            {
                /* Calculate new PC, and send it to fetch unit */
                // cpu->pc = cpu->int_fu.pc + cpu->int_fu.imm;
                
                /* Since we are using reverse callbacks for pipeline stages, 
                    * this will prevent the new instruction from being fetched in the current cycle*/
                // cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                // if(cpu->decode.has_insn==TRUE)
                // {
                //     if(cpu->decode.rd!=-1)
                //         cpu->regs_valid[cpu->decode.rd]++;
                // }
                // cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                // cpu->fetch.has_insn = TRUE;
            }
        }

        else if(cpu->int_fu.opcode == OPCODE_BNZ)
        {
            if (cpu->phy_regs[cpu->zero_flag]->reg_flag == 0)
            {
                /* Calculate new PC, and send it to fetch unit */
                // cpu->pc = cpu->int_fu.pc + cpu->int_fu.imm;
                
                /* Since we are using reverse callbacks for pipeline stages, 
                    * this will prevent the new instruction from being fetched in the current cycle*/
                // cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                // if(cpu->decode.has_insn==TRUE)
                // {
                //     if(cpu->decode.rd!=-1)
                //         cpu->regs_valid[cpu->decode.rd]++;
                // }
                // cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                // cpu->fetch.has_insn =  TRUE;
            }
        }

        else if(cpu->int_fu.opcode == OPCODE_MOVC)
        {
            cpu->int_fu.result_buffer = cpu->int_fu.imm;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->reg_value = cpu->int_fu.result_buffer;
            cpu->phy_regs[cpu->int_fu.renamed_rd]->valid = 1;
        }

        print_stage_content("INT FU -->", &cpu->int_fu);

        cpu->int_fu.has_insn = FALSE;
    }
    else if(cpu->int_fu.has_insn == FALSE)
    {
        print_stage_content("INT FU -->", NULL);
    }
    
}

static void
APEX_mul_fu1(APEX_CPU *cpu)
{
    if(cpu->mul_fu1.has_insn == TRUE)
    {
        if(cpu->mul_fu1.renamed_rs1 != -1)
        {
            if(cpu->phy_regs[cpu->mul_fu1.renamed_rs1]->valid)
                cpu->mul_fu1.rs1_value = cpu->phy_regs[cpu->mul_fu1.renamed_rs1]->reg_value;
            else
                cpu->mul_fu1.rs1_value = cpu->forwarding_bus[cpu->mul_fu1.renamed_rs1].data_value;
        }

        if(cpu->mul_fu1.renamed_rs2 != -1)
        {
            if(cpu->phy_regs[cpu->mul_fu1.renamed_rs2]->valid)
                cpu->mul_fu1.rs2_value = cpu->phy_regs[cpu->mul_fu1.renamed_rs2]->reg_value;
            else
                cpu->mul_fu1.rs2_value = cpu->forwarding_bus[cpu->mul_fu1.renamed_rs2].data_value;
        }

        if(cpu->mul_fu1.opcode == OPCODE_MUL)
        {
            cpu->mul_fu1.result_buffer = cpu->mul_fu1.rs1_value * cpu->mul_fu1.rs2_value;
            cpu->phy_regs[cpu->mul_fu1.renamed_rd]->reg_value = cpu->mul_fu1.result_buffer;
            cpu->phy_regs[cpu->mul_fu1.renamed_rd]->valid = 0;

            if(cpu->mul_fu1.result_buffer == 0)
                cpu->phy_regs[cpu->mul_fu1.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->mul_fu1.renamed_rd]->reg_flag = 0;
            
            print_stage_content("MUL FU 1-->", &cpu->mul_fu1);
            if(cpu->mul_fu2.has_insn==FALSE)
            {
                cpu->mul_fu2 = cpu->mul_fu1;
                cpu->mul_fu1.has_insn = FALSE;
            }
        }
    }
    else
    {
        print_stage_content("MUL FU 1-->", NULL);
    }
    
}

static void
APEX_mul_fu2(APEX_CPU *cpu)
{
    if(cpu->mul_fu2.has_insn == TRUE)
    {
        if(cpu->mul_fu2.opcode == OPCODE_MUL)
        {
            cpu->mul_fu2.result_buffer = cpu->mul_fu2.rs1_value * cpu->mul_fu2.rs2_value;
            cpu->phy_regs[cpu->mul_fu2.renamed_rd]->reg_value = cpu->mul_fu2.result_buffer;
            cpu->phy_regs[cpu->mul_fu2.renamed_rd]->valid = 0;

            if(cpu->mul_fu2.result_buffer == 0)
                cpu->phy_regs[cpu->mul_fu2.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->mul_fu2.renamed_rd]->reg_flag = 0;
            
            print_stage_content("MUL FU 2-->", &cpu->mul_fu2);
            if(cpu->mul_fu3.has_insn==FALSE)
            {
                cpu->mul_fu3 = cpu->mul_fu2;
                cpu->mul_fu2.has_insn = FALSE;
            }
        }
    }
    else
    {
        print_stage_content("MUL FU 2-->", NULL);
    }
}

static void
APEX_mul_fu3(APEX_CPU *cpu)
{
    if(cpu->mul_fu3.has_insn == TRUE)
    {
        if(cpu->mul_fu3.opcode == OPCODE_MUL)
        {
            cpu->mul_fu3.result_buffer = cpu->mul_fu3.rs1_value * cpu->mul_fu3.rs2_value;
            cpu->phy_regs[cpu->mul_fu3.renamed_rd]->reg_value = cpu->mul_fu3.result_buffer;
            cpu->phy_regs[cpu->mul_fu3.renamed_rd]->valid = 0;

            if(cpu->mul_fu3.result_buffer == 0)
                cpu->phy_regs[cpu->mul_fu3.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->mul_fu3.renamed_rd]->reg_flag = 0;
            
            print_stage_content("MUL FU 3-->", &cpu->mul_fu3);
            if(cpu->mul_fu4.has_insn==FALSE)
            {
                cpu->mul_fu4 = cpu->mul_fu3;
                cpu->mul_fu3.has_insn = FALSE;
            }
            request_forwarding_bus_access(cpu, cpu->mul_fu3.renamed_rd, "MulFU");
        }
    }
    else
    {
        print_stage_content("MUL FU 3-->", NULL);
    }
}

static void
APEX_mul_fu4(APEX_CPU *cpu)
{
    if(cpu->mul_fu4.has_insn == TRUE)
    {
        if(cpu->mul_fu4.opcode == OPCODE_MUL)
        {
            cpu->mul_fu4.result_buffer = cpu->mul_fu4.rs1_value * cpu->mul_fu4.rs2_value;
            cpu->phy_regs[cpu->mul_fu4.renamed_rd]->reg_value = cpu->mul_fu4.result_buffer;
            cpu->phy_regs[cpu->mul_fu4.renamed_rd]->valid = 1;

            if(cpu->mul_fu4.result_buffer == 0)
                cpu->phy_regs[cpu->mul_fu4.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->mul_fu4.renamed_rd]->reg_flag = 0;
            
            print_stage_content("MUL FU 4-->", &cpu->mul_fu4);
            cpu->mul_fu4.has_insn = FALSE;
        }
    }
    else
    {
        print_stage_content("MUL FU 4-->", NULL);
    }
}

static void
APEX_lop_fu(APEX_CPU *cpu)
{
    if(cpu->lop_fu.has_insn==TRUE)
    {
        if(cpu->lop_fu.renamed_rs1 != -1)
        {
            if(cpu->phy_regs[cpu->lop_fu.renamed_rs1]->valid)
                cpu->lop_fu.rs1_value = cpu->phy_regs[cpu->lop_fu.renamed_rs1]->reg_value;
            else
                cpu->lop_fu.rs1_value = cpu->forwarding_bus[cpu->lop_fu.renamed_rs1].data_value;
        }

        if(cpu->lop_fu.renamed_rs2 != -1)
        {
            if(cpu->phy_regs[cpu->lop_fu.renamed_rs2]->valid)
                cpu->lop_fu.rs2_value = cpu->phy_regs[cpu->lop_fu.renamed_rs2]->reg_value;
            else
                cpu->lop_fu.rs2_value = cpu->forwarding_bus[cpu->lop_fu.renamed_rs2].data_value;
        }

        if(cpu->lop_fu.opcode == OPCODE_OR)
        {
            cpu->lop_fu.result_buffer = cpu->lop_fu.rs1_value | cpu->lop_fu.rs2_value;
            cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = cpu->lop_fu.result_buffer;
            cpu->phy_regs[cpu->lop_fu.renamed_rd]->valid = 1;

            if(cpu->lop_fu.result_buffer == 0)
                cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = 0;
        }
        else if(cpu->lop_fu.opcode == OPCODE_AND)
        {
            cpu->lop_fu.result_buffer = cpu->lop_fu.rs1_value & cpu->lop_fu.rs2_value;
            cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = cpu->lop_fu.result_buffer;
            cpu->phy_regs[cpu->lop_fu.renamed_rd]->valid = 1;

            if(cpu->lop_fu.result_buffer == 0)
                cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = 0;
        }
        else if(cpu->lop_fu.opcode == OPCODE_XOR)
        {
            cpu->lop_fu.result_buffer = cpu->lop_fu.rs1_value ^ cpu->lop_fu.rs2_value;
            cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = cpu->lop_fu.result_buffer;
            cpu->phy_regs[cpu->lop_fu.renamed_rd]->valid = 1;

            if(cpu->lop_fu.result_buffer == 0)
                cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = 1;
            else
                cpu->phy_regs[cpu->lop_fu.renamed_rd]->reg_flag = 0;
        }

        print_stage_content("LOP FU -->", &cpu->lop_fu);
        cpu->lop_fu.has_insn = FALSE;

    }
    else
    {
        print_stage_content("LOP FU -->", NULL);
    }
}

static void
APEX_dcache(APEX_CPU *cpu)
{
    CommitLoadStoreInstr(cpu);
    if(cpu->dcache.has_insn)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("D_Cache-->", &cpu->dcache);
        }
        cpu->dcache.has_insn = 0;
    }
    else
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("D_Cache-->", NULL);
        }
    }
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    for(i=0; i<4; i++)
    {
        int j;
        for(j=0; j<10; j++)
        {
            cpu->fwd_bus_req_list[i][j] = -1;
        }
    }

    for(i=0; i<PHY_REG_FILE_SIZE; i++)
    {
        cpu->phy_regs[i] = malloc(sizeof(APEX_PHY_REG));
        cpu->phy_regs[i]->reg_flag = -1;
        cpu->phy_regs[i]->reg_value = 0;
        cpu->phy_regs[i]->reg_tag = i;
        cpu->phy_regs[i]->renamed_bit = 0;
        cpu->phy_regs[i]->vCount = 0;
        cpu->phy_regs[i]->cCount = 0;
        cpu->phy_regs[i]->valid = 0;
    }

    for(i=0; i<ARCH_REG_FILE_SIZE; i++)
    {
        cpu->arch_regs[i] = 0;
    }

    for(i=8; i<PHY_REG_FILE_SIZE; i++)
    {
        cpu->free_list[i] = -1;
    }

    for(i=8; i<=PHY_REG_FILE_SIZE; i++)
    {
        cpu->free_list[i-8] = i;
    }

    for(i=0; i<ARCH_REG_FILE_SIZE; i++)
    {
        cpu->rename_table[i] = i;
        cpu->phy_regs[i]->reg_flag = -1;
        cpu->phy_regs[i]->valid = 1;
        cpu->phy_regs[i]->reg_value = 0;
    }

    cpu->lsq_head = 0;
    cpu->lsq_tail = 0;
    cpu->iq_head = 0;
    cpu->iq_tail = 0;
    cpu->rob_head = 0;
    cpu->rob_tail = 0;
    cpu->fwd_req_list_idx = 0;
    cpu->free_list_head = 0;
    cpu->free_list_tail = 7;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        //add rob commit functions

        if(CommitRobEntry(cpu))
        {
            //halt on rob's head
            break;
        }

        APEX_int_fu(cpu);
        APEX_dcache(cpu);
        APEX_mul_fu4(cpu);
        APEX_mul_fu3(cpu);
        APEX_mul_fu2(cpu);
        APEX_mul_fu1(cpu);
        APEX_lop_fu(cpu);
        
        pickup_forwarded_values(cpu);
        pickup_forwarded_values_lsq(cpu);

        wakeup(cpu);
        selection_logic(cpu);
        process_forwarding_requests(cpu);

        APEX_rename2_dispatch(cpu);
        APEX_decode_rename1(cpu);
        APEX_fetch(cpu);

        print_issue_q_entries(cpu);
        print_rob_entries(cpu);
        print_lsq_entries(cpu);
        print_arch_reg_file(cpu);
        print_phy_reg_file(cpu);
        print_rename_table(cpu);
        print_data_memory(cpu);
        print_free_list(cpu);
        print_fwd_bus(cpu);
        

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}