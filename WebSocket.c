#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <libwebsockets.h>

#define	COUNT 20
static int cTimer=COUNT;
static int fSuspended=0;
static struct sysinfo sysLinux;

static int callback_http(struct libwebsocket_context *that,
			 struct libwebsocket *wsi,
			 enum libwebsocket_callback_reasons reason,
			 void *user, void *in, size_t len)
{
	return(0);
}

static int callback_simple_json(struct libwebsocket_context *that,
				struct libwebsocket *wsi,
				enum libwebsocket_callback_reasons reason,
				void *user, void *in, size_t len)
{

	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
		break;
		case LWS_CALLBACK_RECEIVE:
			if(len > 0)
			{
				char *jsonStr=calloc(len+1, 1);
				strncpy(jsonStr, in, len);
				const char jsonCmp[2][12+1]=
				{
					{'"','R','e','s','u','m','e','B','t','n','"','\0'},
					{'"','S','u','s','p','e','n','d','B','t','n','"','\0'}
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
		break;
		case LWS_CALLBACK_SERVER_WRITEABLE:
		break;
		default:
			if(!len && (reason == LWS_CALLBACK_USER+1))
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
						0x22,0x22,
						fSuspended,
						0x22,0x22,
							0x22,0x22,
							sysLinux.procs,
							0x22,0x22,
								0x22,0x22,
								sysLinux.totalram,
								0x22,0x22,
								sysLinux.freeram,
								0x22,0x22,
								sysLinux.sharedram,
								0x22,0x22,
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
	}
	return(0);
}

static struct libwebsocket_protocols protocols[]=
{
	{"http-only", callback_http, 0},
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
				libwebsocket_callback_all_protocol(&protocols[1], LWS_CALLBACK_USER+1);
			}
		}
		libwebsocket_context_destroy(context);
		return(0);
	}
	else
		return(-1);
}
