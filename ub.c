#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unbound.h>

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
	int i = 0;

	/* create context */
	ctx = ub_ctx_create();
	if(!ctx) {
		printf("error: could not create unbound context\n");
		return 1;
	}

	/* asynchronous query for webserver */
	retval = ub_resolve_async(ctx, "www.nlnetlabs.nl", 1, 1, (void*)&qlen, mycallback, NULL);
	if(retval != 0) {
		printf("resolve error: %s\n", ub_strerror(retval));
		return 1;
	}
	qlen++;
	retval = ub_resolve_async(ctx, "www.slashdot.org", 1, 1, (void*)&qlen, mycallback, NULL);
	if(retval != 0) {
		printf("resolve error: %s\n", ub_strerror(retval));
		return 1;
	}
	qlen++;
	//retval = ub_resolve_async(ctx, "www.nlnetlabs.nl", 1, 1, (void*)&qlen, mycallback, NULL);

	/* we keep running, lets do something while waiting */
	while(qlen) {
		usleep(100000); /* wait 1/10 of a second */
		printf("time passed (%d) .. qlen is %d\n", i++, qlen);
		retval = ub_process(ctx);
		if(retval != 0) {
			printf("resolve error: %s\n", ub_strerror(retval));
			return 1;
		}
	}
	printf("done\n");

	ub_ctx_delete(ctx);
	return 0;
}
