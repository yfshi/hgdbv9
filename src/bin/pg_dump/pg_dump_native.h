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

#ifndef PG_DUMP_NATIVE_H
#define PG_DUMP_NATIVE_H

#include "pg_backup.h"
#include "pg_dump.h"




/*
 *	common utility functions
 */

extern TableInfo *getSchemaDataNative(Archive *fout, int *numTablesPtr);
extern void getDumpableObjectsNative(DumpableObject ***objs, int *numObjs);
extern void sortDumpableObjectsNative(DumpableObject **objs, int numObjs,
								DumpId preBoundaryId, DumpId postBoundaryId);
extern void sortDumpableObjectsByTypeNameNative(DumpableObject **objs, int numObjs);

extern bool shouldPrintColumn(const DumpOptions *dopt, const TableInfo *tbinfo, int colno);

/*
 * version specific routines
 */



extern Archive *CreateArchiveNative(const char *FileSpec, const ArchiveFormat fmt,
									  const int compression, bool dosync, ArchiveMode mode,
									  SetupWorkerPtrType setupDumpWorker);
extern void	ConnectDatabaseNative(Archive *AHX,
									const ConnParams *cparams,
									bool isReconnect);
extern void	ProcessArchiveRestoreOptionsNative(Archive *AHX);
extern void	RestoreArchiveNative(Archive *AHX);





#endif							/* PG_DUMP_NATIVE_H */
