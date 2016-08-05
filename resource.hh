/*
 * resource.h
 *
 *  Created on: 2013/09/28
 *      Author: owner
 */

#ifndef RESOURCE_H_
#define RESOURCE_H_

#include "Cvc.hh"

void TakeSnapshot(rusage * theSnapshot_p);

char * PrintProgress(rusage * theLastSnapshot_p, string theHeading = "");


#endif /* RESOURCE_H_ */
