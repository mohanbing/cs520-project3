#include "apex_cpu.h"

static bool is_iq_free(APEX_CPU *cpu); // iq is free or not

static int insert_iq_entry(APEX_CPU* cpu, CPU_Stage *stage); //insert in iq

void set_rob_index(APEX_CPU *cpu, int iq_idx, int rob_idx);

void set_lsq_index(APEX_CPU *cpu, int iq_idx, int lsq_idx);

static CPU_Stage* issue_iq(APEX_CPU* cpu,char* fu_type);

void flush_iq(APEX_CPU* cpu, int pc);

void print_iq(APEX_CPU* cpu);

static void wakeup(APEX_CPU *cpu);

static void selection_logic(APEX_CPU *cpu);