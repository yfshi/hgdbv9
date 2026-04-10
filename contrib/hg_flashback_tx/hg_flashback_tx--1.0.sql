CREATE SCHEMA IF NOT EXISTS hg_flashback_tx;

/*
 * FLASHBACK_TRANSACTION_QUERY
 * Minimal table definition for flashback transaction query metadata.
 */
CREATE TABLE hg_flashback_tx.flashback_transaction_query
(
	xid					int8,
	start_scn			int8,
	start_timestamp		timestamptz,
	commit_scn			int8,
	commit_timestamp	timestamptz,
	logon_user			varchar(128),
	"undo_change#"		int8,
	operation			varchar(32),
	table_name			varchar(256),
	table_owner			varchar(386),
	row_id				varchar(128),
	undo_sql			varchar(4000)
);

CREATE INDEX flashback_transaction_query_xid_idx
	ON hg_flashback_tx.flashback_transaction_query (xid);

CREATE INDEX flashback_transaction_query_commit_timestamp_idx
	ON hg_flashback_tx.flashback_transaction_query (commit_timestamp);
