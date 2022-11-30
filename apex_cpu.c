/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "lsq.h"
#include "apex_macros.h"
#include "rob.h"

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

static void
add_phy_reg_free_list(APEX_CPU *cpu, int tag)
{
    cpu->phy_regs[tag]->is_valid = FALSE;
    cpu->free_list[cpu->free_list_tail] = tag;
    cpu->free_list_tail++;
    cpu->free_list_tail = cpu->free_list_tail%PHY_REG_FILE_SIZE;
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
    cpu->rename_table[rd] = cpu->free_list[cpu->free_list_head];
    cpu->free_list_head++;
    cpu->free_list_head = cpu->free_list_head%PHY_REG_FILE_SIZE;
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

        /* Update PC for next instruction */
        cpu->pc += 4;

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
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_ADDL:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_LOAD:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_LDR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_STORE:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;

                break;
            }

            case OPCODE_STR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                if (cpu->decode_rename1.renamed_rs3==-1)
                {
                    cpu->decode_rename1.renamed_rs3 = cpu->rename_table[cpu->decode_rename1.rs3];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                cpu->decode_rename1.rs3_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs3]->reg_value;

                break;
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                break;
            }

            case OPCODE_SUB:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_SUBL:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_MUL:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_DIV:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;
                
                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];

                break;
            }

            case OPCODE_CMP:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;

                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                
                break;
            }

            case OPCODE_AND:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;

                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                
                break;
            }

            case OPCODE_OR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;

                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                
                break;
            }

            case OPCODE_XOR:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }

                if (cpu->decode_rename1.renamed_rs2==-1)
                {
                    cpu->decode_rename1.renamed_rs2 = cpu->rename_table[cpu->decode_rename1.rs2];
                }

                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;
                cpu->decode_rename1.rs2_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs2]->reg_value;

                rename_table_assign_free_reg(cpu, cpu->decode_rename1.rd);
                if(cpu->rename_stall==0)
                    cpu->decode_rename1.renamed_rd = cpu->rename_table[cpu->decode_rename1.rd];
                
                break;
            }

            case OPCODE_JUMP:
            {
                if (cpu->decode_rename1.renamed_rs1==-1)
                {
                    cpu->decode_rename1.renamed_rs1 = cpu->rename_table[cpu->decode_rename1.rs1];
                }
                cpu->decode_rename1.rs1_value = cpu->phy_regs[cpu->decode_rename1.renamed_rs1]->reg_value;

                break;
            }

            case OPCODE_NOP:
            {
                break;
            }

            case OPCODE_BZ:
            {
                break;
            }

            case OPCODE_BNZ:
            {
                break;
            }
        }

        /* Copy data from decode latch to execute latch*/
        if(cpu->rename2_dispatch.has_insn == 0)
            cpu->rename2_dispatch = cpu->decode_rename1;
            
        cpu->decode_rename1.has_insn = FALSE;

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

    for(i=0; i<PHY_REG_FILE_SIZE; i++)
    {
        cpu->phy_regs[i] = malloc(sizeof(APEX_PHY_REG));
        cpu->phy_regs[i]->reg_flag = -1;
        cpu->phy_regs[i]->reg_value = 0;
        cpu->phy_regs[i]->reg_tag = i;
    }

    for(i=8; i<ARCH_REG_FILE_SIZE; i++)
    {
        cpu->free_list[i] = i;
    }

    for(i=0; i<PHY_REG_FILE_SIZE; i++)
    {
        cpu->rename_table[i] = i;
    }

    cpu->lsq_head = 0;
    cpu->lsq_tail = 0;

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

        APEX_fetch(cpu);

        print_arch_reg_file(cpu);

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