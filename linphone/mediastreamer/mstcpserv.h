#ifndef mstcpserver_h
#define mstcpserver_h

#include "msfilter.h"

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


struct _MSTcpServ{
	MSFilter parent;
	MSQueue *q_inputs[1];
	fd_set set;
	int maxfd;
	int asock;
};

typedef struct _MSTcpServ MSTcpServ;

struct _MSTcpServClass{
	MSFilterClass parent;
};

typedef struct _MSTcpServClass MSTcpServClass;

MSFilter *ms_tcp_serv_new();
#define MS_TCP_SERV(o)	((MSTcpServ*)(o))


#endif

