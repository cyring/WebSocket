/*
 * Part of xfreq-api.h by CyrIng
 *
 * XFreq
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define	TRUE	1
#define	FALSE	0

typedef unsigned int Bool32;

typedef	struct
{
	long long int	nsec_high,
			nsec_low;
} RUNTIME;

#define	TASK_COMM_LEN		16
#define	TASK_PIPE_DEPTH		10

typedef	struct
{
		char		state[8];
		char		comm[TASK_COMM_LEN];
		long int	pid;
		RUNTIME		vruntime;
		long long int	nvcsw;			// sum of [non]voluntary context switch counts
		long int	prio;
		RUNTIME		exec_vruntime;		// a duplicate of vruntime ?
		RUNTIME		sum_exec_runtime;
		RUNTIME		sum_sleep_runtime;
		long int	node;
		char		group_path[32];		// #define PATH_MAX 4096 # chars in a path name including nul
} TASK_STRUCT;

typedef	struct {
	TASK_STRUCT Task[TASK_PIPE_DEPTH];
} CPU_STRUCT;

typedef	enum
{
		SORT_FIELD_NONE       = 0x0,
		SORT_FIELD_STATE      = 0x1,
		SORT_FIELD_COMM       = 0x2,
		SORT_FIELD_PID        = 0x3,
		SORT_FIELD_RUNTIME    = 0x4,
		SORT_FIELD_CTX_SWITCH = 0x5,
		SORT_FIELD_PRIORITY   = 0x6,
		SORT_FIELD_EXEC       = 0x7,
		SORT_FIELD_SUM_EXEC   = 0x8,
		SORT_FIELD_SUM_SLEEP  = 0x9,
		SORT_FIELD_NODE       = 0xa,
		SORT_FIELD_GROUP      = 0xb
} SORT_FIELDS;

#define	SCHED_CPU_SECTION	"cpu#%d"
#define	SCHED_PID_FIELD		"curr->pid"
#define	SCHED_PID_FORMAT	"  .%-30s: %%ld\n"
#define	SCHED_TASK_SECTION	"runnable tasks:"
#define	TASK_SECTION		"Task Scheduling"
#define	TASK_STATE_FMT		"%c"
#define	TASK_COMM_FMT		"%15s"
#define	TASK_TIME_FMT		"%9Ld.%06Ld"
#define	TASK_CTXSWITCH_FMT	"%9Ld"
#define	TASK_PRIORITY_FMT	"%5ld"
#define	TASK_NODE_FMT		"%ld"
#define	TASK_GROUP_FMT		"%s"

#define	TASK_PID_FMT		"%5ld"

#define	TASK_STRUCT_FORMAT	TASK_STATE_FMT""TASK_COMM_FMT" "TASK_PID_FMT" "TASK_TIME_FMT" "TASK_CTXSWITCH_FMT" "TASK_PRIORITY_FMT" "TASK_TIME_FMT" "TASK_TIME_FMT" "TASK_TIME_FMT" "TASK_NODE_FMT" "TASK_GROUP_FMT

#define	PROCESSOR_FMT

typedef struct {
	unsigned int CPU;
	char Model[64];
	CPU_STRUCT *C;
} PROC_STRUCT;

PROC_STRUCT *uSchedInit(void) ;
void uSchedFree(PROC_STRUCT *P) ;
Bool32 uSchedule(Bool32 Reverse, short int SortField, PROC_STRUCT *P) ;
