#include "LocklessQueue.h"

int main()
{
	LocklessQueueSingleConsumerMultipleProducers<int, 512> l;
	int args[3] = { 1, 2, 3 };
	l.push(args, 3);
}