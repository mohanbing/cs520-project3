#include "forwarding_bus.h"

static void
request_forwarding_bus_access(APEX_CPU *cpu, CPU_Stage stage, char *fu_type)
{
   if(strcmp(fu_type, "IntFU") == 0)
   {
        cpu->fwd_bus_req_list[0] = &stage;
   }
   else if(strcmp(fu_type, "MulFU") == 0)
   {
        cpu->fwd_bus_req_list[2] = &stage;
   }
   else if(strcmp(fu_type, "LopFU") == 0)
   {
        cpu->fwd_bus_req_list[3] = &stage;
   }
   else //dcache
   {
        cpu->fwd_bus_req_list[1] = &stage;
   }
}

static void
process_forwarding_requests(APEX_CPU *cpu)
{
    int i;
    for(i=0; i<4; i++)
    {
        if(cpu->fwd_bus_req_list[i]!=NULL)
        {
            cpu->forwarding_bus[cpu->fwd_bus_req_list[i]->renamed_rd].tag_valid=1;
            cpu->fwd_bus_req_list[i]=NULL;
            return;
        }
    }
}