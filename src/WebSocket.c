/*
 * WebSocket.c by CyrIng
 * Copyright (C) 2015-2021 CYRIL INGENIERIE
 *
 * Licenses: GPL2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <libwebsockets.h>

static int fShutdown = 0;

typedef enum {LOW_LEVEL = 1, RISING_EDGE, HIGH_LEVEL, FALLING_EDGE} WAVEFORM;
WAVEFORM Clock = LOW_LEVEL;

#define BT(F, N) (F & (1 << N))
#define BS(F, N) (F |=(1 << N))
#define BC(F) (F = 0)
#define QUOTES '"'
#define TIMESTR "YYYY-MM-DDTHH:MM:SS.000Z"
#define JSONSTR "{"							\
	"\"Transmission\":"						\
		"{"							\
		"\"Time\":\"YYYY-MM-DDTHH:MM:SS.000Z\","		\
		"\"Starting\":false,"					\
		"\"Suspended\":false"					\
		"},"							\
	"\"SysInfo\":"							\
		"{"							\
		"\"Kernel\":\"Operating System 0.0.0.0\","		\
		"\"Processes\":00000,"					\
		"\"Memory\":"						\
			"{"						\
			"\"Total\":00000000000000,"			\
			"\"Free\":00000000000000,"			\
			"\"Shared\":00000000000000,"			\
			"\"Buffer\":00000000000000"			\
			"}"						\
		"}"							\
	"}"

#define COUNT 5
static int cTimer = COUNT;
static unsigned long int Flags;

void Transition(void)
{
	cTimer--;
	if (!cTimer)
	{
		if (Clock == FALLING_EDGE) {
			Clock = LOW_LEVEL;
		} else {
			Clock++;
		}
		cTimer = COUNT;
	}
}

static int fSuspended = 0;
static char *JSON_String = NULL;
static size_t JSON_Length = 0;
static struct sysinfo SysLinux;

size_t JSON_Startify(char *jsonStr)
{
	time_t now = time(NULL);
	char timeStr[sizeof(TIMESTR) + 1];

	strftime(timeStr, sizeof(TIMESTR),
		"%Y-%m-%dT%H:%M:%S.000Z", localtime(&now));

	struct utsname OSinfo={{0}};
	uname(&OSinfo);

	sprintf(jsonStr,
				"{"					\
				"\"Transmission\":"			\
					"{"				\
					"\"Time\":\"%s\","		\
					"\"Starting\":true,"		\
					"\"Suspended\":%s"		\
					"},"				\
				"\"SysInfo\":"				\
					"{"				\
					"\"Kernel\":\"%s %s\""		\
					"}"				\
				"}",
					timeStr,
					(fSuspended) ? "true" : "false",
					OSinfo.sysname, OSinfo.release);
	size_t fullLen = strlen(jsonStr);
	return (fullLen);
}

size_t JSON_Stringify(char *jsonStr)
{
	time_t now = time(NULL);
	char timeStr[sizeof(TIMESTR) + 1];

	strftime(timeStr, sizeof(TIMESTR),
		"%Y-%m-%dT%H:%M:%S.000Z", localtime(&now));

	if (!fSuspended)
	{
			sprintf(jsonStr,
				"{"					\
				"\"Transmission\":"			\
					"{"				\
					"\"Time\":\"%s\","		\
					"\"Starting\":false,"		\
					"\"Suspended\":%s"		\
					"},"				\
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
					(fSuspended) ? "true" : "false",
					SysLinux.procs,
						SysLinux.totalram,
						SysLinux.freeram,
						SysLinux.sharedram,
						SysLinux.bufferram);
	}
	else
	{
		sprintf(jsonStr,
				"{"					\
				"\"Transmission\":"			\
					"{"				\
					"\"Time\":\"%s\","		\
					"\"Starting\":false,"		\
					"\"Suspended\":%s"		\
					"}"				\
				"}",
					timeStr,
					(fSuspended) ? "true" : "false");
	}
	size_t fullLen = strlen(jsonStr);
	return (fullLen);
}

#define ROOTDIR "http"
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
	}, *walker;
	for (walker = MIME_db; walker->extension != NULL; walker++)
	{
	    if (!strcmp(&filePath[strlen(filePath)-strlen(walker->extension)],
			walker->extension))
	    {
		break;
	    }
	}
	return (walker->fullType);
}

static int callback_http(struct lws *wsi,
			enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	SESSION_HTTP *session = (SESSION_HTTP *) user;
	char *pIn = (char *) in;

	int rc = 0;
	switch (reason) {
	case LWS_CALLBACK_HTTP:
	    if (len > 0)
	    {
		session->filePath = calloc(2048, 1);
		strcat(session->filePath, ROOTDIR);

		if ((len == 1) && (pIn[0] == '/')) {
			strcat(session->filePath, "/index.html");
		} else {
			strncat(session->filePath, pIn, len);
		}
		if (!stat(session->filePath, &session->fileStat)
		&& (session->fileStat.st_size > 0)
		&& (S_ISREG(session->fileStat.st_mode)))
		{
			if (lws_serve_http_file(wsi,
						session->filePath,
						MIME_type(session->filePath),
						NULL, 0) < 0) {
				rc = -1;
			} else {
				rc = 0;
			}
		}
		else
		{
			lws_return_http_status(wsi,
						HTTP_STATUS_NOT_FOUND,
						NULL);
			rc = -1;
		}
		free(session->filePath);
	    }
	    else
	    {
		lws_return_http_status(wsi,
					HTTP_STATUS_BAD_REQUEST, NULL);
		if (lws_http_transaction_completed(wsi)) {
			rc = -1;
		}
	    }
		break;
	case LWS_CALLBACK_HTTP_BODY:
		break;
	case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		{
		lws_return_http_status(wsi, HTTP_STATUS_OK, NULL);
		}
		break;
	case LWS_CALLBACK_HTTP_WRITEABLE:
		break;
	case LWS_CALLBACK_CLOSED_HTTP:
		break;
	default:
		break;
	}
	return (rc);
}


typedef struct
{
	int prefixSum;
	int remainder;
	unsigned char *buffer;
} SESSION_JSON;

static int callback_simple_json(struct lws *wsi,
				enum lws_callback_reasons reason,
				void *user, void *in, size_t len)
{
	SESSION_JSON *session = (SESSION_JSON *) user;

	int rc = 0;
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
      {
	char *jsonStartStr = malloc(sizeof(JSONSTR));
	if (jsonStartStr != NULL)
	{
		size_t jsonStartLen = JSON_Startify(jsonStartStr);
		session->prefixSum = 0;
		session->remainder = jsonStartLen;
		session->buffer = malloc(LWS_SEND_BUFFER_PRE_PADDING
					+ jsonStartLen
					+ LWS_SEND_BUFFER_POST_PADDING);
	    if (session->buffer != NULL) {
		strncpy((char *) &session->buffer[LWS_SEND_BUFFER_PRE_PADDING],
			jsonStartStr,
			jsonStartLen);
	    }
		free(jsonStartStr);
	}
      }
	break;
    case LWS_CALLBACK_RECEIVE:
	if (len > 0)
	{
		char *jsonStr = calloc(len+1, 1);
	    if (jsonStr != NULL)
	    {
		strncpy(jsonStr, in, len);
		const char jsonCmp[2][12+1]=
		{
		  {QUOTES,'R','e','s','u','m','e','B','t','n',QUOTES,'\0'},
		  {QUOTES,'S','u','s','p','e','n','d','B','t','n',QUOTES,'\0'}
		};
		int i;
		for (i = 0; i < 2; i++) {
			if (!strcmp(jsonStr, jsonCmp[i]))
			{
				fSuspended = i;
				break;
			}
		}
		free(jsonStr);
	    }
	}
	break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
      if (!fShutdown)
      {
	if (session->remainder > 0)
	{
		int written = lws_write(wsi,
					&session->buffer[session->prefixSum
						+ LWS_SEND_BUFFER_PRE_PADDING],
					session->remainder,
					LWS_WRITE_TEXT);
		if (written < 0) {
			rc = 1;
			break;
		}
		else {
			session->prefixSum += written;
			session->remainder -= written;
			if (session->remainder) {
				lws_callback_on_writable(wsi);
			}
		}
	}
	else {
		session->prefixSum = 0;
		session->remainder = JSON_Length;
		session->buffer = realloc(session->buffer,
					LWS_SEND_BUFFER_PRE_PADDING
					+ JSON_Length
					+ LWS_SEND_BUFFER_POST_PADDING);
	    if (session->buffer != NULL) {
		strncpy((char *) &session->buffer[LWS_SEND_BUFFER_PRE_PADDING],
			JSON_String,
			JSON_Length);
	    }
	}
      }
      else	// Flush the remaining buffer.
      {
	if (session->remainder > 0)
	{
		int written = lws_write(wsi,
					&session->buffer[session->prefixSum
						+ LWS_SEND_BUFFER_PRE_PADDING],
					session->remainder,
					LWS_WRITE_TEXT);
		if (written < 0) {
			rc = 1;
			break;
		}
		else {
			session->prefixSum += written;
			session->remainder -= written;
			if (session->remainder) {
				lws_callback_on_writable(wsi);
			}
		}
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
	return (rc);
}

static struct lws_protocols protocols[] = \
{
	{"http-only",	callback_http	,	sizeof(SESSION_HTTP),	0},
	{"simple-json", callback_simple_json,	sizeof(SESSION_JSON),	0},
	{NULL	,	NULL	,		0	,		0}
};

int MainLoop(int argc, char *argv[], struct lws_context *ctx)
{
	const struct timespec timeout = {.tv_sec = 0, .tv_nsec = 50000000};
	BC(Flags);
	while (!fShutdown)
	{
	    if (lws_service(ctx, -1) >= 0)
	    {
		Transition();
		switch (Clock) {
		case LOW_LEVEL:
			if (!BT(Flags, Clock)) {
				if (!fSuspended) {
					sysinfo(&SysLinux);
				}
				BS(Flags, Clock);
			}
			break;
		case RISING_EDGE:
			if (!BT(Flags, Clock)) {
				JSON_Length = JSON_Stringify(JSON_String);
				BS(Flags, Clock);
			}
			break;
		case HIGH_LEVEL:
			if (!BT(Flags, Clock)) {
				lws_callback_on_writable_all_protocol(ctx,
							&protocols[1]);
				BS(Flags, Clock);
			}
			break;
		case FALLING_EDGE:
			if (Flags) {
				BC(Flags);
			}
			break;
		}
	    }
	    if (!fShutdown) {
		nanosleep(&timeout, NULL);
	    }
	}
	return (0);
}

void SigHandler(int sig)
{
	fShutdown = 1;
}

int main(int argc, char *argv[])
{
	int rc = 0;

	signal(SIGINT, SigHandler);

	JSON_Length = 1;
	JSON_Length += sizeof(TIMESTR);
	JSON_Length += sizeof(JSONSTR);

	JSON_String = malloc(JSON_Length);
	if (JSON_String != NULL)
	{
		memset(&SysLinux, 0, sizeof(struct sysinfo));

		struct lws_context *ctx = NULL;
		struct lws_context_creation_info ctxInfo;
		memset(&ctxInfo, 0, sizeof(ctxInfo));
		ctxInfo.port = (argc == 2) ? atoi(argv[1]) : 8080;
		ctxInfo.gid = -1;
		ctxInfo.uid = -1;
		ctxInfo.protocols = protocols;

		if ((ctx = lws_create_context(&ctxInfo)) != NULL)
		{
			rc = MainLoop(argc, argv, ctx);

			lws_context_destroy(ctx);
		} else {
			rc = -1;
		}
		free(JSON_String);
	}
	return (rc);
}
