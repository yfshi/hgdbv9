/* Begin - ReqID:SRS-Performance-GUC */
#ifndef INTERNAL_H
#define INTERNAL_H

#include "hg_misc.h"
#include "storage/ipc.h"

/* GUC parameters */
extern int  heap_page_repair_interval;
extern Oid  special_index_oid;
extern bool get_snapshot_fast_release_lwlock;
extern int  clog_multiple_number;

extern bool enable_autoset; /* OA#1640 */

extern void FastReleaseProcArrayLockAcquire(LWLockMode mode);
extern void FastReleaseProcArrayLockRelease(void);
extern void HeapPagePruneWithThreshold(Relation relation, Buffer buffer,
                                       struct GlobalVisState *vistest, 
                                       TransactionId limited_xmin,
                                       TimestampTz limited_ts);
extern bool SkipSpecialIndexOid(IndexScanDesc scan);
extern Size CLOGShmemBuffersHook(void);
extern bool PDBGIfPrintLog(void);
extern void PDBGAutoset(AutosetMode mode);

#endif /* INTERNAL_H */