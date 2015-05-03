#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <libwebsockets.h>

#define	COUNT 20
static int timer=COUNT;

static int callback_http(struct libwebsocket_context *that, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
	if(len > 0)
		printf("callback_http(reason[%d], len[%zd])\n", reason, len);
	return(0);
}

static int callback_dumb_increment(struct libwebsocket_context *that, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{

	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
			if(len > 0)
				printf("callback_dumb_increment(reason[LWS_CALLBACK_ESTABLISHED], len[%zd])\n", len);
		break;
		case LWS_CALLBACK_RECEIVE:
			if(len > 0)
				printf("callback_dumb_increment(reason[LWS_CALLBACK_RECEIVE], len[%zd])\n", len);
		break;
		case LWS_CALLBACK_USER:
			if(len > 0)
				printf("callback_dumb_increment(reason[LWS_CALLBACK_USER], len[%zd])\n", len);
		break;
		case LWS_CALLBACK_SERVER_WRITEABLE:
			if(len > 0)
				printf("callback_dumb_increment(reason[LWS_CALLBACK_SERVER_WRITEABLE], len[%zd])\n", len);
		break;
		default:
			if(!len && (reason == LWS_CALLBACK_USER+1))
			{
				char si[128];
				struct sysinfo sysLinux;
				if(!sysinfo(&sysLinux))
					sprintf(si, "{%cProcesses%c:%hu, %cMemory%c:{%cTotal%c:%14lu, %cFree%c:%14lu, %cShared%c:%14lu, %cBuffer%c:%14lu}}",
						0x22,0x22,sysLinux.procs, 0x22,0x22, 0x22,0x22,sysLinux.totalram, 0x22,0x22,sysLinux.freeram, 0x22,0x22,sysLinux.sharedram, 0x22,0x22,sysLinux.bufferram);
				else
					sprintf(si, "{%cProcesses%c:0, %cMemory%c:{%cTotal%c:0, %cFree%c:0, %cShared%c:0, %cBuffer%c:0}}",
						0x22,0x22, 0x22,0x22, 0x22,0x22, 0x22,0x22, 0x22,0x22, 0x22,0x22);
				size_t li=strlen(si);

				unsigned char *buf = (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + li + LWS_SEND_BUFFER_POST_PADDING);
				strncpy((char *) &buf[LWS_SEND_BUFFER_PRE_PADDING], si, li);
				libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], li, LWS_WRITE_TEXT);
				free(buf);
			}
		break;
	}
	return(0);
}

static struct libwebsocket_protocols protocols[]=
{
	{	"http-only",			callback_http,			0},
	{	"dumb-increment-protocol",	callback_dumb_increment,	0},
	{	NULL,				NULL,				0}
};

int main(int argc, char *argv[])
{
	struct libwebsocket_context *context;
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof info);
	info.port=(argc == 2) ? atoi(argv[1]) : 8080;
	info.gid=-1;
	info.uid=-1;
	info.protocols=protocols;
	if((context=libwebsocket_create_context(&info)) != NULL)
	{
		while(1)	// SIGTERM
		{
			libwebsocket_service(context, 50);
			timer--;
			if(!timer)
			{
				timer=COUNT;
				libwebsocket_callback_all_protocol(&protocols[1], LWS_CALLBACK_USER+1);
			}
		}
		libwebsocket_context_destroy(context);
		return(0);
	}
	else
		return(-1);
}
