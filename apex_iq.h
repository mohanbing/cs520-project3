#include "apex_cpu.h"

bool is_iq_free(APEX_CPU *cpu); // iq is free or not

int insert_iq_entry(APEX_CPU* cpu, CPU_Stage *stage); //insert in iq

void set_rob_index(APEX_CPU *cpu, int iq_idx, int rob_idx);

void set_lsq_index(APEX_CPU *cpu, int iq_idx, int lsq_idx);

CPU_Stage* issue_iq(APEX_CPU* cpu,char* fu_type);

int flush_iq(APEX_CPU* cpu);

void print_iq(APEX_CPU* cpu);

void wakeup(APEX_CPU *cpu);

void selection_logic(APEX_CPU *cpu);