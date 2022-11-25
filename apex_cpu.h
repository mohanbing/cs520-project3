/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"
#include "apex_iq.h"

 #define No_of_IQ_Entry 8

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int rs3;
    int imm;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rs3;
    int renamed_rs1;
    int renamed_rs2;
    int renamed_rs3;
    int rd;
    int renamed_rd;
    int imm;
    int rs1_value;
    int rs2_value;
    int rs3_value;
    int result_buffer;
    int memory_address;
    int has_insn;
} CPU_Stage;

typedef struct IQ_Entry
{
    int pc;
    char opcode_str[128];
    int counter;
    int free;
    char fu_type[128];
    int imm;
    //use it for arch register
    int rs1;
    int rs2;
    int rd;
    APEX_PHY_REG *prs1;
    APEX_PHY_REG *prs2;
    int lsqindex;
    int robindex;
}IQ_Entry;

typedef struct IQ
{
    int iq_free; // flag if iq is free or not
    IQ_Entry iq_entry[No_of_IQ_Entry];
}IQ;


typedef struct APEX_PHY_REG
{
    int reg_tag;
    int reg_value;
    int reg_flag;
} APEX_PHY_REG;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
    
    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int fetch_from_next_cycle;

    int arch_regs[ARCH_REG_FILE_SIZE];       /* Integer register file */
    APEX_PHY_REG *phy_regs[PHY_REG_FILE_SIZE];

    int free_list[PHY_REG_FILE_SIZE];
    int rename_table[PHY_REG_FILE_SIZE];
    int free_list_head;
    int free_list_tail;
    int rename_stall;

    //iq
    // IQ_Entry *iq_fifo[No_of_IQ_Entry];
    IQ iq_fifo;

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode_rename1;
    CPU_Stage rename2_dispatch;

} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
#endif
