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

#ifndef PG_DUMP_ORACLE_H
#define PG_DUMP_ORACLE_H

#include "pg_backup.h"
#include "pg_dump.h"





/*
 *	common utility functions
 */

extern TableInfo *getSchemaDataOracle(Archive *fout, int *numTablesPtr);
extern void getDumpableObjectsOracle(DumpableObject ***objs, int *numObjs);
extern void sortDumpableObjectsOracle(DumpableObject **objs, int numObjs,
								DumpId preBoundaryId, DumpId postBoundaryId);
extern void sortDumpableObjectsByTypeNameOracle(DumpableObject **objs, int numObjs);

/*
 * version specific routines
 */
extern NamespaceInfo *getNamespaces(Archive *fout, int *numNamespaces);
extern ExtensionInfo *getExtensions(Archive *fout, int *numExtensions);
extern TypeInfo *getTypes(Archive *fout, int *numTypes);
extern FuncInfo *getFuncs(Archive *fout, int *numFuncs);
/* Begin - ReqID:SRS-SQL-PACKAGE */
extern PkgInfo *getPackages(Archive *fout, int *numPkgs);
/* End - ReqID:SRS-SQL-PACKAGE */
extern SynInfo *getSynonyms(Archive *fout, int *numSyns);	/* ReqID:SRS-SQL-SYNONYM */
extern AggInfo *getAggregates(Archive *fout, int *numAggregates);
extern OprInfo *getOperators(Archive *fout, int *numOperators);
extern AccessMethodInfo *getAccessMethods(Archive *fout, int *numAccessMethods);
extern OpclassInfo *getOpclasses(Archive *fout, int *numOpclasses);
extern OpfamilyInfo *getOpfamilies(Archive *fout, int *numOpfamilies);
extern CollInfo *getCollations(Archive *fout, int *numCollations);
extern ConvInfo *getConversions(Archive *fout, int *numConversions);
extern TableInfo *getTables(Archive *fout, int *numTables);
extern void getOwnedSeqs(Archive *fout, TableInfo tblinfo[], int numTables);
extern InhInfo *getInherits(Archive *fout, int *numInherits);
extern void getPartitioningInfo(Archive *fout);
extern void getIndexes(Archive *fout, TableInfo tblinfo[], int numTables);
extern void getExtendedStatistics(Archive *fout);
extern void getConstraints(Archive *fout, TableInfo tblinfo[], int numTables);
extern RuleInfo *getRules(Archive *fout, int *numRules);
extern void getTriggers(Archive *fout, TableInfo tblinfo[], int numTables);
extern ProcLangInfo *getProcLangs(Archive *fout, int *numProcLangs);
extern CastInfo *getCasts(Archive *fout, int *numCasts);
extern TransformInfo *getTransforms(Archive *fout, int *numTransforms);
extern void getTableAttrs(Archive *fout, TableInfo *tbinfo, int numTables);
extern TSParserInfo *getTSParsers(Archive *fout, int *numTSParsers);
extern TSDictInfo *getTSDictionaries(Archive *fout, int *numTSDicts);
extern TSTemplateInfo *getTSTemplates(Archive *fout, int *numTSTemplates);
extern TSConfigInfo *getTSConfigurations(Archive *fout, int *numTSConfigs);
extern FdwInfo *getForeignDataWrappers(Archive *fout,
									   int *numForeignDataWrappers);
extern ForeignServerInfo *getForeignServers(Archive *fout,
											int *numForeignServers);
extern DefaultACLInfo *getDefaultACLs(Archive *fout, int *numDefaultACLs);
extern void getExtensionMembership(Archive *fout, ExtensionInfo extinfo[],
								   int numExtensions);
extern void processExtensionTables(Archive *fout, ExtensionInfo extinfo[],
								   int numExtensions);
extern EventTriggerInfo *getEventTriggers(Archive *fout, int *numEventTriggers);
extern void getPolicies(Archive *fout, TableInfo tblinfo[], int numTables);
extern PublicationInfo *getPublications(Archive *fout,
										int *numPublications);
extern void getPublicationTables(Archive *fout, TableInfo tblinfo[],
								 int numTables);
extern void getSubscriptions(Archive *fout);

extern TableInfo *getTables(Archive *fout, int *numTables);

extern Archive *CreateArchiveOracle(const char *FileSpec, const ArchiveFormat fmt,
									  const int compression, bool dosync, ArchiveMode mode,
									  SetupWorkerPtrType setupDumpWorker);
extern void	ConnectDatabaseOracle(Archive *AHX,
									const ConnParams *cparams,
									bool isReconnect);
extern void	ProcessArchiveRestoreOptionsOracle(Archive *AHX);
extern void	RestoreArchiveOracle(Archive *AHX);

#endif							/* PG_DUMP_ORACLE_H */
