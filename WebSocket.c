#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <libwebsockets.h>

#define	QUOTES '"'
#define	COUNT 20
static int cTimer=COUNT;
static int fSuspended=0;
static struct sysinfo sysLinux;

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

	if(len > 0)
		lwsl_notice("HTTP: reason[%d] , len[%zd] , in[%s] , user[%p]\n", reason, len, pIn, user);

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
					libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_NOT_FOUND, NULL);
					rc=-1;
				}
				free(session->filePath);
			}
			else
			{
				libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_BAD_REQUEST, NULL);
				if(lws_http_transaction_completed(wsi))
					rc=-1;
			}
		break;
		case LWS_CALLBACK_HTTP_BODY:
		{
			lwsl_notice("HTTP: post data\n");
		}
		break;
		case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		{
			lwsl_notice("HTTP: complete post data\n");
			libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_OK, NULL);
		}
		break;
		case LWS_CALLBACK_HTTP_WRITEABLE:
		break;
		case LWS_CALLBACK_CLOSED_HTTP:
			lwsl_notice("HTTP closed connection\n");
		break;
		default:
		break;
	}
	return(rc);
}

static int callback_simple_json(struct libwebsocket_context *ctx,
				struct libwebsocket *wsi,
				enum libwebsocket_callback_reasons reason,
				void *user, void *in, size_t len)
{
	if(len > 0)
		lwsl_notice("JSON: reason[%d] , len[%zd] , in[%s] , user[%p]\n", reason, len, (char *)in, user);

	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
			lwsl_notice("JSON established connection\n");
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
		case LWS_CALLBACK_USER:
			if(!len)
			{
				if(!fSuspended)
					sysinfo(&sysLinux);

				char *jsonStr=malloc(256);
				sprintf(jsonStr,"{"\
						"%cSuspended%c:%d,"\
						"%cSysInfo%c:"\
							"{"\
							"%cProcesses%c:%hu,"\
							"%cMemory%c:"\
								"{"\
								"%cTotal%c:%14lu,"\
								"%cFree%c:%14lu,"\
								"%cShared%c:%14lu,"\
								"%cBuffer%c:%14lu"\
								"}"\
							"}"\
						"}",
						QUOTES,QUOTES,
						fSuspended,
						QUOTES,QUOTES,
							QUOTES,QUOTES,
							sysLinux.procs,
							QUOTES,QUOTES,
								QUOTES,QUOTES,
								sysLinux.totalram,
								QUOTES,QUOTES,
								sysLinux.freeram,
								QUOTES,QUOTES,
								sysLinux.sharedram,
								QUOTES,QUOTES,
								sysLinux.bufferram);
				size_t jsonLen=strlen(jsonStr);

				unsigned char *buffer=(unsigned char*) malloc(	LWS_SEND_BUFFER_PRE_PADDING
										+ jsonLen
										+ LWS_SEND_BUFFER_POST_PADDING);
				strncpy((char *) &buffer[LWS_SEND_BUFFER_PRE_PADDING], jsonStr, jsonLen);
				libwebsocket_write(wsi, &buffer[LWS_SEND_BUFFER_PRE_PADDING], jsonLen, LWS_WRITE_TEXT);
				free(buffer);
				free(jsonStr);
			}
		break;
		case LWS_CALLBACK_SERVER_WRITEABLE:
		break;
		case LWS_CALLBACK_CLOSED:
			lwsl_notice("JSON closed connection\n");
		break;
		default:
		break;
	}
	return(0);
}

static struct libwebsocket_protocols protocols[]=
{
	{"http-only", callback_http, sizeof(SESSION_HTTP)},
	{"simple-json", callback_simple_json, 0},
	{NULL, NULL, 0}
};

int main(int argc, char *argv[])
{
	memset(&sysLinux, 0, sizeof(struct sysinfo));
	struct libwebsocket_context *context;
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	info.port=(argc == 2) ? atoi(argv[1]) : 8080;
	info.gid=-1;
	info.uid=-1;
	info.protocols=protocols;
	if((context=libwebsocket_create_context(&info)) != NULL)
	{
		while(1)	// SIGTERM
		{
			libwebsocket_service(context, 50);
			cTimer--;
			if(!cTimer)
			{
				cTimer=COUNT;
				libwebsocket_callback_all_protocol(&protocols[1], LWS_CALLBACK_USER);
			}
		}
		libwebsocket_context_destroy(context);
		return(0);
	}
	else
		return(-1);
}
