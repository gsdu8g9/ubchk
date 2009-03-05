#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unbound.h>

#define QNAME_MAX 255
#define QUEUE_MAX 10

/* This is called when resolution is completed */
void mycallback(void* mydata, int err, struct ub_result* result)
{
	int* qlen = (int*)mydata;
	(*qlen)--;
	if(err != 0) {
		printf("resolve error: %s\n", ub_strerror(err));
		return;
	}
	/* show first result */
	if(result->havedata)
		printf("The address of %s is %s\n", result->qname, 
			inet_ntoa(*(struct in_addr*)result->data[0]));

	ub_resolve_free(result);
}

int main(void)
{
	struct ub_ctx* ctx;
	volatile int qlen = 0;
	int retval;
	char qname[QNAME_MAX];
	char *moredata, *nl;

	/* basic checks */
	if(QUEUE_MAX < 1) {
		printf("error: queue length must be >0\n");
		return 1;
	}

	/* create context */
	ctx = ub_ctx_create();
	if(!ctx) {
		printf("error: could not create unbound context\n");
		return 1;
	}

	/* we keep running, lets do something while waiting */
	moredata = (char*)-1;
	do {
		/* queue has room && more data on stdin? */
		if(qlen < QUEUE_MAX && moredata) {
			/* read and prepare qname from stdin */
			moredata = fgets(qname, QNAME_MAX, stdin);
			if(moredata != NULL) {
				nl = strrchr(qname, '\n');
				if(nl) *nl = '\0';

				/* add async query to queue */
				retval = ub_resolve_async(ctx, qname, 1, 1, (void*)&qlen, mycallback, NULL);
				if(retval != 0) {
					printf("resolve error for %s: %s\n", qname, ub_strerror(retval));
					continue;
				}
				qlen++;
			}
		} else {
			/* queue is full || eof stdin reached */
			usleep(100000); /* wait 1/10 of a second */
			retval = ub_process(ctx);
			if(retval != 0) {
				printf("resolve error: %s\n", ub_strerror(retval));
				return 1;
			}
		}
	} while(qlen || moredata);

	ub_ctx_delete(ctx);
	return 0;
}
