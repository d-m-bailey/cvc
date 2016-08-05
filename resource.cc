/*
 * resource.c
 *
 *  Created on: 2013/09/27
 *      Author: oscn000041
 */

#include "resource.hh"

void TakeSnapshot(rusage * theSnapshot_p) {

	getrusage(RUSAGE_SELF, theSnapshot_p);
}

char * PrintProgress(rusage * theLastSnapshot_p, string theHeading) {
	rusage currentSnapshot;
	static char myString[1024];

	TakeSnapshot(&currentSnapshot);

	sprintf(myString, "%sUsage: Time: %ld  Memory: %ld  I/O: %ld  Swap: %ld", theHeading.c_str(), currentSnapshot.ru_utime.tv_sec,
			currentSnapshot.ru_maxrss, currentSnapshot.ru_inblock + currentSnapshot.ru_oublock,
			currentSnapshot.ru_nswap);

	theLastSnapshot_p->ru_idrss = currentSnapshot.ru_idrss;
	theLastSnapshot_p->ru_inblock = currentSnapshot.ru_inblock;
	theLastSnapshot_p->ru_isrss = currentSnapshot.ru_isrss;
	theLastSnapshot_p->ru_ixrss = currentSnapshot.ru_ixrss;
	theLastSnapshot_p->ru_majflt = currentSnapshot.ru_majflt;
	theLastSnapshot_p->ru_maxrss = currentSnapshot.ru_maxrss;
	theLastSnapshot_p->ru_minflt = currentSnapshot.ru_minflt;
	theLastSnapshot_p->ru_msgrcv = currentSnapshot.ru_msgrcv;
	theLastSnapshot_p->ru_msgsnd = currentSnapshot.ru_msgsnd;
	theLastSnapshot_p->ru_nivcsw = currentSnapshot.ru_nivcsw;
	theLastSnapshot_p->ru_nsignals = currentSnapshot.ru_nsignals;
	theLastSnapshot_p->ru_nswap = currentSnapshot.ru_nswap;
	theLastSnapshot_p->ru_nvcsw = currentSnapshot.ru_nvcsw;
	theLastSnapshot_p->ru_oublock = currentSnapshot.ru_oublock;
	theLastSnapshot_p->ru_stime = currentSnapshot.ru_stime;
	theLastSnapshot_p->ru_utime = currentSnapshot.ru_utime;

	return(myString);
}
