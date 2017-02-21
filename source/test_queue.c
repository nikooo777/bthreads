#include "tqueue.h"

int main(int argc, char **argv)
{

	TQueue *root = malloc(sizeof(TQueue));
	root->next = NULL;
	root->thread = NULL;

	__bthread_private* tmpThread = malloc(sizeof(__bthread_private));

	tqueue_enqueue(&root, tmpThread);
	tqueue_enqueue(&root, tmpThread);
	tqueue_enqueue(&root, tmpThread);

	printf("test");

	return 0;
}

