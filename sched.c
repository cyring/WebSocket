/*
 * xfreq-intel.c by CyrIng
 *
 * XFreq
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"

// Monitor the kernel tasks scheduling.
Bool32 IsGreaterRuntime(RUNTIME *rt1, RUNTIME *rt2)
{
	return(	( (rt1->nsec_high > rt2->nsec_high) || ((rt1->nsec_high == rt2->nsec_high) && ((rt1->nsec_low > rt2->nsec_low))) ) ? TRUE : FALSE);
}

Bool32 uSchedule(unsigned int CPU, Bool32 Reverse, short int SortField, CPU_STRUCT *C)
{
	FILE	*fSD=NULL;

	if(((fSD=fopen("/proc/sched_debug", "r")) != NULL))
	{
		TASK_STRUCT oTask=
		{
			.state={0},
			.comm={0},
			.pid=0,
			.vruntime={0},
			.nvcsw=0,
			.prio=0,
			.exec_vruntime={0},
			.sum_exec_runtime={0},
			.sum_sleep_runtime={0},
			.node=0,
			.group_path={0}
		};

		const TASK_STRUCT rTask=
		{
			.state={0x7f},
			.comm={0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x0},
			.pid=(unsigned short) -1,
			.vruntime={(unsigned) -1, (unsigned) -1},
			.nvcsw=(unsigned) -1,
			.prio=(unsigned short) -1,
			.exec_vruntime={(unsigned) -1, (unsigned) -1},
			.sum_exec_runtime={(unsigned) -1, (unsigned) -1},
			.sum_sleep_runtime={(unsigned) -1, (unsigned) -1},
			.node=(unsigned short) -1,
			.group_path={0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x0}
		},
		*pTask=(Reverse) ? &rTask : &oTask;

		unsigned int cpu=0, depth=0;
		for(cpu=0; cpu < CPU; cpu++)
			for(depth=0; depth < TASK_PIPE_DEPTH; depth++)
				memcpy(&C[cpu].Task[depth], pTask, sizeof(TASK_STRUCT));

		char buffer[1024], schedPidFmt[48];
		sprintf(schedPidFmt, SCHED_PID_FORMAT, SCHED_PID_FIELD);

		while(fgets(buffer, sizeof(buffer), fSD) != NULL)
		{
			if((sscanf(buffer, SCHED_CPU_SECTION, &cpu) > 0) && (cpu >= 0) && (cpu < CPU))
			{
				while(fgets(buffer, sizeof(buffer), fSD) != NULL)
				{
					if((buffer[0] == '\n') || (sscanf(buffer, schedPidFmt, &oTask.pid) > 0))
					{
						C[cpu].Task[0].pid=oTask.pid;

						while(fgets(buffer, sizeof(buffer), fSD) != NULL)
						{
							if(!strncmp(buffer, SCHED_TASK_SECTION, 15))
							{
								fgets(buffer, sizeof(buffer), fSD);	// skip until "\n"
								fgets(buffer, sizeof(buffer), fSD);	// skip until "\n"

								while(fgets(buffer, sizeof(buffer), fSD) != NULL)
								{
									if((buffer[0] != '\n') && !feof(fSD))
									{
										sscanf(	buffer, TASK_STRUCT_FORMAT,
											oTask.state,
											oTask.comm,
											&oTask.pid,
											&oTask.vruntime.nsec_high,
											&oTask.vruntime.nsec_low,
											&oTask.nvcsw,
											&oTask.prio,
											&oTask.exec_vruntime.nsec_high,
											&oTask.exec_vruntime.nsec_low,
											&oTask.sum_exec_runtime.nsec_high,
											&oTask.sum_exec_runtime.nsec_low,
											&oTask.sum_sleep_runtime.nsec_high,
											&oTask.sum_sleep_runtime.nsec_low,
											&oTask.node,
											oTask.group_path);

										if(C[cpu].Task[0].pid == oTask.pid)
											memcpy(&C[cpu].Task[0], &oTask, sizeof(TASK_STRUCT));
										else	// Insertion Sort.
											for(depth=1; depth < TASK_PIPE_DEPTH; depth++)
											{
												Bool32 isFlag=FALSE;

												switch(SortField)
												{
												case SORT_FIELD_NONE:
													isFlag=TRUE;
												break;
												case SORT_FIELD_STATE:
													isFlag=(strcasecmp(oTask.state, C[cpu].Task[depth].state) > 0);
												break;
												case SORT_FIELD_COMM:
													isFlag=(strcasecmp(oTask.comm, C[cpu].Task[depth].comm) > 0);
												break;
												case SORT_FIELD_PID:
													isFlag=(oTask.pid > C[cpu].Task[depth].pid);
												break;
												case SORT_FIELD_RUNTIME:
													isFlag=IsGreaterRuntime(&oTask.vruntime, &C[cpu].Task[depth].vruntime);
												break;
												case SORT_FIELD_CTX_SWITCH:
													isFlag=(oTask.nvcsw > C[cpu].Task[depth].nvcsw);
												break;
												case SORT_FIELD_PRIORITY:
													isFlag=(oTask.prio > C[cpu].Task[depth].prio);
												break;
												case SORT_FIELD_EXEC:
													isFlag=IsGreaterRuntime(&oTask.exec_vruntime, &C[cpu].Task[depth].exec_vruntime);
												break;
												case SORT_FIELD_SUM_EXEC:
													isFlag=IsGreaterRuntime(&oTask.sum_exec_runtime, &C[cpu].Task[depth].sum_exec_runtime);
												break;
												case SORT_FIELD_SUM_SLEEP:
													isFlag=IsGreaterRuntime(&oTask.sum_sleep_runtime, &C[cpu].Task[depth].sum_sleep_runtime);
												break;
												case SORT_FIELD_NODE:
													isFlag=(oTask.node > C[cpu].Task[depth].node);
												break;
												case SORT_FIELD_GROUP:
													isFlag=(strcasecmp(oTask.group_path, C[cpu].Task[depth].group_path) > 0);
												break;
												}
												if(isFlag ^ Reverse)
												{
													size_t shift=(TASK_PIPE_DEPTH - depth - 1) * sizeof(TASK_STRUCT);
													if(shift != 0)
														memmove(&C[cpu].Task[depth + 1], &C[cpu].Task[depth], shift);

													memcpy(&C[cpu].Task[depth], &oTask, sizeof(TASK_STRUCT));

													break;
												}
											}
									}
									else
										break;
								}
								break;
							}
						}
						break;
					}
				}
			}
		}
		fclose(fSD);
		return(TRUE);
	}
	else
		return(FALSE);
}
