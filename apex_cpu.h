/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include <stdbool.h>
#include "apex_macros.h"

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

typedef struct APEX_PHY_REG
{
    int valid;
    int reg_tag;
    int reg_value;
    int reg_flag;
} APEX_PHY_REG;

typedef struct IQ_Entry
{
    int allocated; // allocated/free bit
    int pc;
    int opcode;
    int counter;
    int free;
    char fu_type[128];
    int imm;

    //physical(renamed) registers
    int src1_valid;
    int src2_valid;
    int src3_valid;
    
    int src1_tag;
    int src2_tag;
    int src3_tag;
    int dst_tag;

    int src1_value;
    int src2_value;
    int src3_value;

    int lsqindex;
    int robindex;

    CPU_Stage *dispatch;

    int request_exec;
    int granted;

}IQ_Entry;

typedef struct LSQ_Entry
{
    int lsq_estd;
    int opcode;
    int pc;
    int load_str;
    int mem_addr_valid;
    int mem_addr;

    int rob_idx;

    int renamed_rs1;
    int renamed_rs1_value_valid;
    int renamed_rs1_value;

    int renamed_rs2;
    int renamed_rs2_value_valid;
    int renamed_rs2_value;

    int renamed_rs3;
    int renamed_rs3_value_valid;
    int renamed_rs3_value;

    int renamed_rd;
    int dest_type;
    CPU_Stage *disptach;
}LSQ_Entry;

typedef struct ROB_ENTRY
{
    int establised_bit;             //rob entry established bit
    int instruction_type;           //store opcode, can be used during commit
    int pc;                         //program counter
    int physical_rd;                //physical register destination    
    int architectural_rd;           //destination: architectural register
    int lsq_index;                  //load store queue index    
    int dcache_bit;                 //dcache accessed bit
    // int overwritten_entry;       //overwritten rename table entry; not needed
    //int mem_error_code;           //
}ROB_ENTRY;

typedef struct FORWARDING_BUS
{
    int tag_valid;
    int data_value;
}FORWARDING_BUS;

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

    FORWARDING_BUS forwarding_bus[PHY_REG_FILE_SIZE];

    //iq
    IQ_Entry *iq[IQ_SIZE];
    int iq_head;
    int iq_tail;

    LSQ_Entry *lsq[LSQ_SIZE];
    int lsq_head;
    int lsq_tail;

    //ROB entries
    ROB_ENTRY *rob[ROB_SIZE];
    int rob_head;
    int rob_tail;

    CPU_Stage *fwd_bus_req_list[4];
    int fwd_req_list_idx = 0;

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode_rename1;
    CPU_Stage rename2_dispatch;

    CPU_Stage int_fu;

    CPU_Stage mul_fu1;  // insn always issued to this FU first for MUL
    CPU_Stage mul_fu2;
    CPU_Stage mul_fu3;
    CPU_Stage mul_fu4;

    CPU_Stage dcache;
    CPU_Stage lop_fu;

} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
#endif
