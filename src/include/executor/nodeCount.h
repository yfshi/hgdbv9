/*-------------------------------------------------------------------------
 *
 * nodeCount.h
 *
 * src/include/executor/nodeCount.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODECOUNT_H
#define NODECOUNT_H

#include "nodes/execnodes.h"

extern int ExecCountSlotsCount(Count *node);
extern CountState *ExecInitCount(Count *node, EState *estate, int eflags);
extern void ExecEndCount(CountState *node);
extern void ExecReScanCount(CountState *node);

#endif   /* NODECOUNT_H */
