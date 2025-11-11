#ifndef MYS_PRINTTUP_H
#define MYS_PRINTTUP_H

#include "postgres.h"

#include "tcop/dest.h"

DestReceiver *mysql_printtup_create_DR(CommandDest dest);

#endif