#ifndef _FORWARDING_BUS_H_
#define _FORWARDING_BUS_H_

#include "apex_cpu.h"

static void
request_forwarding_bus_access(APEX_CPU *cpu, int renamed_rd, char *fu_type);

static void
process_forwarding_requests(APEX_CPU *cpu);

#endif