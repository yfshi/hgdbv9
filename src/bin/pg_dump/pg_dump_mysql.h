/*-------------------------------------------------------------------------
 *
 * pg_dump_mysql.h
 *	  Common header file for the pg_dump utility
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/bin/pg_dump/pg_dump_mysql.h
 *
 * add the file for requirement "SRS-MYSQL-PGDUMP"
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_DUMP_MYSQL_H
#define PG_DUMP_MYSQL_H

#include "pg_backup.h"
#include "pg_dump.h"





/*
 *	common utility functions
 */

extern TableInfo *getSchemaDataMysql(Archive *fout, int *numTablesPtr);
extern void getDumpableObjectsMysql(DumpableObject ***objs, int *numObjs);
extern void sortDumpableObjectsMysql(DumpableObject **objs, int numObjs,
								DumpId preBoundaryId, DumpId postBoundaryId);
extern void sortDumpableObjectsByTypeNameMysql(DumpableObject **objs, int numObjs);

extern Archive *CreateArchiveMysql(const char *FileSpec, const ArchiveFormat fmt,
									  const int compression, bool dosync, ArchiveMode mode,
									  SetupWorkerPtrType setupDumpWorker);
extern void	ConnectDatabaseMysql(Archive *AHX,
									const ConnParams *cparams,
									bool isReconnect);
extern void	ProcessArchiveRestoreOptionsMysql(Archive *AHX);
extern void	RestoreArchiveMysql(Archive *AHX);

#endif							/* PG_DUMP_MYSQL_H */
