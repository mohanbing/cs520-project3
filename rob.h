#include "apex_cpu.h"

bool IsRobFree(APEX_CPU *cpu);

int AddRobEntry(APEX_CPU *cpu, CPU_Stage *stage, int lsq_index);

int DeleteRobEntry(APEX_CPU *cpu);

int CommitRobEntry(APEX_CPU *cpu);

void PrintRobContents(APEX_CPU *cpu);