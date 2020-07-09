#include <libevpower/msgqueue.h>
#include "libevpower.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <mqueue.h>

struct mq_attr attr;
#define FILEMODE (S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP)

/*
 * Save name of queue by yourself!
 * return 0 if sucess
 * return -1 if unsucess
 * */
int mqcreate(int flag, int maxmsg, long int msgsize, const char *name)
{
	int flags;
	mqd_t mqd;

	if (maxmsg < 0 || maxmsg > 10 || msgsize > 8192 || msgsize < 0) {
		debug_msg("params exceed");
		return -1;
	}

	flags = O_RDWR | O_CREAT | flag;

	attr.mq_maxmsg = maxmsg;
	attr.mq_msgsize = msgsize;
    
	if ((attr.mq_maxmsg != 0 && attr.mq_msgsize == 0) || (attr.mq_maxmsg == 0 && attr.mq_msgsize !=0)){
		debug_msg("must specify both maxmsg and msgsize");
		return -1;
	}

	mqd = mq_open(name, flags, FILEMODE, (attr.mq_maxmsg != 0) ? &attr : NULL);
	debug_msg("%d  %d", mqd, errno);
	mq_close(mqd);
	return 0;
}

int mqsend(const char *name, char *msg, int msglen, unsigned int prio)
{
	int ret;
	mqd_t mqd;
	struct mq_attr attr;

	if (strlen(msg)!= msglen || !name || !msg) {
		debug_msg("params wrong");
		return -1;
	}

	if ((mqd = mq_open(name, O_WRONLY)) == -1){
		debug_msg("open error: %d", errno);
		return -1;
	}
	mq_getattr(mqd, &attr);
	ret = mq_send(mqd, msg, msglen, prio);
	if (errno != NULL && ret != NULL)
		debug_msg("ret: %d, errno: %s", ret, errno);
	mq_close(mqd);
	return ret;
}
/*
 * be aware msg may contains nonsense characters
 * */

char *mqreceive(const char *name)
{
	int flags;
	mqd_t mqd;
	ssize_t n;
	unsigned int prio;
	char *msg;
	struct mq_attr attr;
	flags = O_RDONLY | O_NONBLOCK;
	mqd = mq_open(name, flags);
	mq_getattr(mqd, &attr);

	msg = malloc(attr.mq_msgsize);

	n = mq_receive(mqd, msg, attr.mq_msgsize, &prio);
	debug_msg("read %ld bytes,buffer = %s,length = %d,priority = %u\n", (long)n, msg, strlen(msg), prio);
	mq_close(mqd);
	return msg;
}
/*
 * result pointer must be freed after calling.
 * */

char *mqreceive_timed(const char *name, int lenth, int tval)
{
	int flags;
	mqd_t mqd;
	ssize_t n;
	unsigned int prio;
	char *msg, *result;
	struct mq_attr attr;
	struct timespec abs_timeout;
	flags = O_RDONLY;
	clock_gettime(CLOCK_REALTIME, &abs_timeout);
	abs_timeout.tv_sec+=tval;

	mqd = mq_open(name, flags);
	if ( mqd < 0 || mq_getattr(mqd, &attr) < 0)
		goto exit;

	if (lenth > attr.mq_msgsize)
		goto exit;

	msg = malloc(attr.mq_msgsize);
	n = mq_timedreceive(mqd, msg, attr.mq_msgsize, &prio, &abs_timeout);
	if (n < 0) {
		free(msg);
		goto exit;
	}
	debug_msg("read %ld bytes, buffer = %s, length = %d, priority = %u\n", (long)n, msg, strlen(msg), prio);
	//only copy and send valid message
	result = strndup(msg, n);
	free(msg);
	mq_close(mqd);
	debug_msg("result:%s %d",result, strlen(result));
	return result;
exit:
	mq_close(mqd);
	return NULL;
}
