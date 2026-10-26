/* Column encryption uses an authenticated extended-varlena envelope. */
#ifndef COLUMNENC_H
#define COLUMNENC_H
#include "access/htup.h"
#include "utils/relcache.h"
extern bool RelationHasEncryptedColumns(Relation rel);
extern bool RelationNeedsColumnEncryption(Relation rel);
extern bool ColumnTupleNeedsTransform(Relation rel, HeapTuple tuple);
extern void CheckColumnEncryption(Oid typid);
extern struct varlena *ColumnEncryptDatum(struct varlena *value);
extern struct varlena *ColumnDecryptDatum(struct varlena *value);
extern bool ColumnDatumIsEncrypted(struct varlena *value);
#endif
