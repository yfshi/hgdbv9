/*-------------------------------------------------------------------------
 *
 * ora_indexcond.h
 *	  prototypes for ora_indexcond.c.
 *
 *
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/optimizer/ora_indexcond.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ORA_INDEXCOND_H
#define ORA_INDEXCOND_H


#include "nodes/pathnodes.h"


extern Oid	ora_indexcond_oct_oid;

extern Node * take_oracasetrans_away(Node *clause);
extern bool is_oct_inside(Node *node, Node **inner_var);


#endif							/* ORA_INDEXCOND_H */
