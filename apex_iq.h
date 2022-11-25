int check_iq(APEX_CPU *cpu); // iq is free or not

int insert_iq_entry(APEX_CPU* cpu,IQ_Entry* iq_entry); //insert in iq

int issue_iq(APEX_CPU* cpu,char* fu_type); // 

int flush_iq(APEX_CPU* cpu);

void print_iq(APEX_CPU* cpu);