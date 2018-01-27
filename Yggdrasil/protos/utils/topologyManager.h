/*
 * topologyManager.h
 *
 *  Created on: 27/11/2017
 *      Author: akos
 */

#ifndef TOPOLOGYMANAGER_H_
#define TOPOLOGYMANAGER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"
#include "core/utils/queue.h"
#include "core/utils/utils.h"
#include "lightkone.h"

void * topologyManager_init(void * args);

#endif /* TOPOLOGYMANAGER_H_ */
