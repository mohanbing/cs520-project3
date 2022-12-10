#include<stdio.h>
#include "apex_cpu.h"

int IsBtbFree(APEX_CPU *cpu)
{
    return cpu->btb[cpu->btb_tail] == NULL;
}

BTB_ENTRY *ProbeBtb(APEX_CPU *cpu, int pc)
{
    int entryIndex = -1;
    for(int i = 0; i < BTB_SIZE; i++)
    {
        if(cpu->btb[i] != NULL && cpu->btb[i]->pc == pc)
        {
            entryIndex = i;
            break;
        }
    }
    return entryIndex >= 0 ? cpu->btb[entryIndex] : NULL;
}

void DeleteBtbEntry(APEX_CPU *cpu)
{
    free(cpu->btb[cpu->btb_head]);
    cpu->btb[cpu->btb_head] = NULL;
    cpu->btb_head = (cpu->btb_head + 1) % BTB_SIZE;
}

void AddBtbEntry(APEX_CPU *cpu, int pc)
{
    if(IsBtbFree(cpu))
    {
        BTB_ENTRY *btb_entry = (BTB_ENTRY*)(malloc(sizeof(BTB_ENTRY)));
        btb_entry->pc = pc;
        btb_entry->prediction = cpu->decode_rename1.imm < 0 ? TRUE : FALSE;
        btb_entry->target_pc = -1;

        cpu->btb[cpu->btb_tail] = btb_entry;
        cpu->btb_tail = (cpu->btb_tail + 1) % BTB_SIZE;
    }
    else
    {
        DeleteBtbEntry(cpu);
        BTB_ENTRY *btb_entry = (BTB_ENTRY*)(malloc(sizeof(BTB_ENTRY)));
        btb_entry->prediction = cpu->decode_rename1.imm < 0 ? TRUE : FALSE;
        btb_entry->pc = cpu->pc;
        btb_entry->target_pc = -1;

        cpu->btb[cpu->btb_tail] = btb_entry;
        cpu->btb_tail = (cpu->btb_tail + 1) % BTB_SIZE;
    }
}

void UpdateBtbEntry(APEX_CPU *cpu, int instruction_pc, int updated_pc)
{
    for(int i = 0; i < BTB_SIZE; i++)
    {
        if(cpu->btb[i]->pc == instruction_pc)
        {
            cpu->btb[i]->target_pc = updated_pc;
            break;
        }
    }
}