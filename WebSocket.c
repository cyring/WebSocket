/*
 * WebSocket.c by CyrIng
 *
 * Licenses: GPL2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <libwebsockets.h>
#include "sched.h"

#define	MAXCPU 8

typedef	enum {LOW_LEVEL=1, RISING_EDGE, HIGH_LEVEL, FALLING_EDGE} WAVEFORM;
WAVEFORM Clock=LOW_LEVEL;

#define	BT(F, N) (F & (1 << N))
#define BS(F, N) (F |=(1 << N))
#define BC(F) (F = 0)
#define	QUOTES '"'
#define	TIMESTR "YYYY-MM-DD HH:MM:SS"
#define	JSONSTR	"{"						\
		"\"Time\":\"\","				\
		"\"Suspended\":0,"				\
		"\"SysInfo\":"					\
			"{"					\
			"\"Processes\":00000,"			\
			"\"Memory\":"				\
				"{"				\
				"\"Total\":00000000000000,"	\
				"\"Free\":00000000000000,"	\
				"\"Shared\":00000000000000,"	\
				"\"Buffer\":00000000000000"	\
				"}"				\
			"},"					\
		"\"CPU\":[]"					\
		"}"

#define	COUNT 5
static int cTimer=COUNT;
static unsigned long int flags;

void Transition(void)
{
	cTimer--;
	if(!cTimer)
	{
		if(Clock == FALLING_EDGE)
			{Clock=LOW_LEVEL;}
		else
			{Clock++;}
		cTimer=COUNT;
	}
}

static int fSuspended=0;
static char *jsonString=NULL;
static size_t jsonLength=0;
static struct sysinfo sysLinux;
static CPU_STRUCT *C=NULL;

size_t jsonStringify(char *jsonStr)
{
	size_t fullLen=0;
	time_t now=time(NULL);
	char timeStr[sizeof(TIMESTR) + 1];

	strftime(timeStr, sizeof(TIMESTR),
		"%Y-%m-%d %H:%M:%S", localtime(&now));

	if(!fSuspended)
	{
		char *jsonCPU=calloc(MAXCPU, (TASK_PIPE_DEPTH + 3) * (TASK_COMM_LEN + 3));
		int cpu=0, i=0;
		for(cpu=0; cpu < MAXCPU; cpu++)
		{
			strcat(jsonCPU, "[");
			for(i=0; i < TASK_PIPE_DEPTH; i++)
				sprintf(jsonCPU, "%s\"%s\"%c", jsonCPU, C[cpu].Task[i].comm,
						(i < (TASK_PIPE_DEPTH - 1)) ? ',' : '\0');
			if(cpu < (MAXCPU - 1))
				strcat(jsonCPU, "],");
			else
				strcat(jsonCPU, "]");
		}
		fullLen=sprintf(jsonStr,
				"{"					\
				"\"Time\":\"%s\","			\
				"\"Suspended\":%d,"			\
				"\"SysInfo\":"				\
					"{"				\
					"\"Processes\":%hu,"		\
					"\"Memory\":"			\
						"{"			\
						"\"Total\":%14lu,"	\
						"\"Free\":%14lu,"	\
						"\"Shared\":%14lu,"	\
						"\"Buffer\":%14lu"	\
						"}"			\
					"},"				\
				"\"CPU\":[%s]"				\
				"}",
				timeStr,
				fSuspended,
					sysLinux.procs,
						sysLinux.totalram,
						sysLinux.freeram,
						sysLinux.sharedram,
						sysLinux.bufferram,
				jsonCPU);
		free(jsonCPU);
	}
	else
	{
		fullLen=sprintf(jsonStr,
				"{"					\
				"\"Time\":\"%s\","			\
				"\"Suspended\":%d,"			\
				"\"SysInfo\":"				\
					"{"				\
					"\"Processes\":%hu,"		\
					"\"Memory\":"			\
						"{"			\
						"\"Total\":%14lu,"	\
						"\"Free\":%14lu,"	\
						"\"Shared\":%14lu,"	\
						"\"Buffer\":%14lu"	\
						"}"			\
					"}"				\
				"}",
				timeStr,
				fSuspended,
					sysLinux.procs,
						sysLinux.totalram,
						sysLinux.freeram,
						sysLinux.sharedram,
						sysLinux.bufferram);
	}
	return(fullLen);
}

#define	ROOTDIR "http"
typedef struct
{
	char *filePath;
	struct stat fileStat;
} SESSION_HTTP;

const char *MIME_type(const char *filePath)
{
	struct
	{
		const char *extension;
		const char *fullType;
	} MIME_db[]=
	{
		{".html", "text/html"},
		{".css",  "text/css"},
		{".ico",  "image/x-icon"},
		{".js",   "text/javascript"},
		{".png",  "image/png"},
		{NULL,    "text/plain"}
	}, *walker=NULL;
	for(walker=MIME_db; walker->extension != NULL; walker++)
		if(!strcmp(&filePath[strlen(filePath)-strlen(walker->extension)], walker->extension))
			break;
	return(walker->fullType);
}

static int callback_http(struct libwebsocket_context *ctx,
			 struct libwebsocket *wsi,
			 enum libwebsocket_callback_reasons reason,
			 void *user, void *in, size_t len)
{
	SESSION_HTTP *session=(SESSION_HTTP *) user;
	char *pIn=(char *)in;

	int rc=0;
	switch(reason)
	{
		case LWS_CALLBACK_HTTP:
			if(len > 0)
			{
				session->filePath=calloc(2048, 1);
				strcat(session->filePath, ROOTDIR);

				if((len == 1) && (pIn[0] == '/'))
					strcat(session->filePath, "/index.html");
				else
					strncat(session->filePath, pIn, len);

				if(!stat(session->filePath, &session->fileStat)
				&& (session->fileStat.st_size > 0)
				&& (S_ISREG(session->fileStat.st_mode)))
				{
					if(libwebsockets_serve_http_file(ctx, wsi,
									 session->filePath,
									 MIME_type(session->filePath),
									 NULL, 0) < 0)
						rc=-1;
					else
						rc=0;
				}
				else
				{
					libwebsockets_return_http_status(ctx, wsi,
									HTTP_STATUS_NOT_FOUND, NULL);
					rc=-1;
				}
				free(session->filePath);
			}
			else
			{
				libwebsockets_return_http_status(ctx, wsi,
								HTTP_STATUS_BAD_REQUEST, NULL);
				if(lws_http_transaction_completed(wsi))
					rc=-1;
			}
		break;
		case LWS_CALLBACK_HTTP_BODY:
		break;
		case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		{
			libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_OK, NULL);
		}
		break;
		case LWS_CALLBACK_HTTP_WRITEABLE:
		break;
		case LWS_CALLBACK_CLOSED_HTTP:
		break;
		default:
		break;
	}
	return(rc);
}


typedef struct
{
	int sum;
	int remainder;
	unsigned char *buffer;
} SESSION_JSON;

static int callback_simple_json(struct libwebsocket_context *ctx,
				struct libwebsocket *wsi,
				enum libwebsocket_callback_reasons reason,
				void *user, void *in, size_t len)
{
	SESSION_JSON *session=(SESSION_JSON *) user;

	int rc=0;
	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
		{
			session->sum=0;
			session->remainder=jsonLength;
			session->buffer=malloc(	LWS_SEND_BUFFER_PRE_PADDING
						+ jsonLength
						+ LWS_SEND_BUFFER_POST_PADDING);
			strncpy((char *) &session->buffer[LWS_SEND_BUFFER_PRE_PADDING],
					jsonString,
					jsonLength);
		}
		break;
		case LWS_CALLBACK_RECEIVE:
			if(len > 0)
			{
				char *jsonStr=calloc(len+1, 1);
				strncpy(jsonStr, in, len);
				const char jsonCmp[2][12+1]=
				{
					{QUOTES,'R','e','s','u','m','e','B','t','n',QUOTES,'\0'},
					{QUOTES,'S','u','s','p','e','n','d','B','t','n',QUOTES,'\0'}
				};
				int i=0;
				for(i=0; i < 2; i++)
					if(!strcmp(jsonStr, jsonCmp[i]))
					{
						fSuspended=i;
						break;
					}
				free(jsonStr);
			}
		break;
		case LWS_CALLBACK_SERVER_WRITEABLE:
			{
				int written;
				if(session->remainder > 0)
				{
					written=libwebsocket_write(wsi,
						&session->buffer[session->sum + LWS_SEND_BUFFER_PRE_PADDING],
									session->remainder, LWS_WRITE_TEXT);
					if(written < 0) {
						rc=1;
						break;
					}
					else {
						session->sum+=written;
						session->remainder-=written;
						if(session->remainder)
							libwebsocket_callback_on_writable(ctx, wsi);
					}
				}
				else {
					session->sum=0;
					session->remainder=jsonLength;
					session->buffer=realloc(session->buffer,
								LWS_SEND_BUFFER_PRE_PADDING
								+ jsonLength
								+ LWS_SEND_BUFFER_POST_PADDING);
					strncpy((char *) &session->buffer[LWS_SEND_BUFFER_PRE_PADDING],
							jsonString,
							jsonLength);
				}
			}
		break;
		case LWS_CALLBACK_CLOSED:
		{
			free(session->buffer);
		}
		break;
		default:
		break;
	}
	return(rc);
}

static struct libwebsocket_protocols protocols[]=
{
	{"http-only", callback_http, sizeof(SESSION_HTTP)},
	{"simple-json", callback_simple_json, sizeof(SESSION_JSON)},
	{NULL, NULL, 0}
};

int main(int argc, char *argv[])
{
	int rc=0;
	jsonLength=sizeof(TIMESTR);
	jsonLength+=sizeof(JSONSTR);
	jsonLength+=MAXCPU * (TASK_PIPE_DEPTH + 3) * (TASK_COMM_LEN + 3);
	jsonString=malloc(jsonLength);

	memset(&sysLinux, 0, sizeof(struct sysinfo));
	C=malloc(MAXCPU * sizeof(CPU_STRUCT));

	struct libwebsocket_context *Ctx;
	struct lws_context_creation_info CtxInfo;
	memset(&CtxInfo, 0, sizeof(CtxInfo));
	CtxInfo.port=(argc == 2) ? atoi(argv[1]) : 8080;
	CtxInfo.gid=-1;
	CtxInfo.uid=-1;
	CtxInfo.protocols=protocols;
	if((Ctx=libwebsocket_create_context(&CtxInfo)) != NULL)
	{
		BC(flags);
		while(1)	// SIGTERM
		{
			libwebsocket_service(Ctx, 50);

			Transition();
			switch(Clock)
			{
			case LOW_LEVEL:
				if(!BT(flags, Clock)) {
					if(!fSuspended) {
						sysinfo(&sysLinux);

						uSchedule(MAXCPU,
							FALSE,
							(argc == 3) ? atoi(argv[2])
								: SORT_FIELD_RUNTIME,
							C);
					}
					BS(flags, Clock);
				}
			break;
			case RISING_EDGE:
				if(!BT(flags, Clock)) {
					jsonLength=jsonStringify(jsonString);
					BS(flags, Clock);
				}
			break;
			case HIGH_LEVEL:
				if(!BT(flags, Clock)) {
					libwebsocket_callback_on_writable_all_protocol(&protocols[1]);
					BS(flags, Clock);
				}
			break;
			case FALLING_EDGE:
				if(flags) {
					BC(flags);
				}
			break;
			}
			fflush(stdout);
		}
		libwebsocket_context_destroy(Ctx);
		rc=0;
	}
	else
		rc=-1;

	free(C);
	if(jsonString)
		free(jsonString);
	return(rc);
}
