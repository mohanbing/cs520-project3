#include "forwarding_bus.h"

static void
request_forwarding_bus_access(APEX_CPU *cpu, int renamed_rd, char *fu_type)
{
     int i;
     if(strcmp(fu_type, "IntFU") == 0)
     {
          for(i=0; i<10; i++)
          {
               if(cpu->fwd_bus_req_list[0][i]==-1)
               {
                    cpu->fwd_bus_req_list[0][i]=renamed_rd;
                    return;
               }
          }
     }
     else if(strcmp(fu_type, "MulFU") == 0)
     {
          for(i=0; i<10; i++)
          {
               if(cpu->fwd_bus_req_list[2][i]==-1)
               {
                    cpu->fwd_bus_req_list[2][i]=renamed_rd;
                    return;
               }
          }
     }
     else if(strcmp(fu_type, "LopFU") == 0)
     {
          for(i=0; i<10; i++)
          {
               if(cpu->fwd_bus_req_list[3][i]==-1)
               {
                    cpu->fwd_bus_req_list[3][i]=renamed_rd;
                    return;
               }
          }
     }
     else //dcache
     {
          for(i=0; i<10; i++)
          {
               if(cpu->fwd_bus_req_list[1][i]==-1)
               {
                    cpu->fwd_bus_req_list[1][i]=renamed_rd;
                    return;
               }
          }
     }
}

static void
process_forwarding_requests(APEX_CPU *cpu)
{
    int i;
    for(i=0;i<PHY_REG_FILE_SIZE+1; i++)
    {
          if(cpu->forwarding_bus[i].tag_valid==1)
          {
               cpu->forwarding_bus[i].data_value = cpu->phy_regs[i]->reg_value;
          }     
    }
    for(i=0; i<4; i++)
    {
          int j=0;
          for(j=0; j<10; j++)
          {
               if(cpu->fwd_bus_req_list[i][j]!=-1)
               {
                    cpu->forwarding_bus[cpu->fwd_bus_req_list[i][j]].tag_valid=1;
                    cpu->fwd_bus_req_list[i][j]=-1;
                    return;
               }
          }
    }
}