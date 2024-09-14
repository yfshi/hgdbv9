/*-------------------------------------------------------------------------
 *
 * pg_dump.h
 *	  Common header file for the pg_dump utility
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/bin/pg_dump/pg_dump.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_DUMP_PG_H
#define PG_DUMP_PG_H

#include "pg_backup.h"
#include "pg_dump.h"





/*
 *	common utility functions
 */

extern TableInfo *getSchemaDataPg(Archive *fout, int *numTablesPtr);
extern void getDumpableObjectsPg(DumpableObject ***objs, int *numObjs);
extern void sortDumpableObjectsPg(DumpableObject **objs, int numObjs,
								DumpId preBoundaryId, DumpId postBoundaryId);
extern void sortDumpableObjectsByTypeNamePg(DumpableObject **objs, int numObjs);

extern Archive *CreateArchivePg(const char *FileSpec, const ArchiveFormat fmt,
									  const int compression, bool dosync, ArchiveMode mode,
									  SetupWorkerPtrType setupDumpWorker);
extern void	ConnectDatabasePg(Archive *AHX,
									const ConnParams *cparams,
									bool isReconnect);
extern void	ProcessArchiveRestoreOptionsPg(Archive *AHX);
extern void	RestoreArchivePg(Archive *AHX);

#endif							/* PG_DUMP_PG_H */
