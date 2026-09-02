-- MySQL table-scoped index names (mysql_logical_index_reloptions):
--   1) Unspecified Key_name: ChooseIndexName(); no logical_idxname in reloptions
--   2) Specified Key_name free in schema: physical = Key_name; no logical_idxname
--   3) Specified Key_name taken: construct_unique_index_name(); store logical_idxname
--   4) DROP/RENAME/FORCE INDEX: match physical relname first, then logical_idxname
-- SHOW: COALESCE(logical_idxname, relname) as Key_name; never WITH (logical_idxname=...)
--       in SHOW CREATE TABLE / SHOW INDEX.  pg_get_indexdef / \\d+ show
--       WITH (logical_idxname=...) only when physical name was mangled.
-- Helper: Key_name vs physical relname, and whether logical_idxname is stored.
CREATE OR REPLACE FUNCTION pg_temp.idx_names(tbl name)
RETURNS TABLE(logical_name text, physical_name name, phys_eq_logical boolean,
			  has_logical_idxname boolean, indisunique boolean, indisprimary boolean)
LANGUAGE sql AS $$
	SELECT COALESCE(
			(SELECT opt.option_value
			   FROM pg_catalog.pg_options_to_table(c.reloptions)
					AS opt(option_name, option_value)
			  WHERE opt.option_name = 'logical_idxname'
			  LIMIT 1),
			c.relname::text) AS logical_name,
		   c.relname AS physical_name,
		   (c.relname::text = COALESCE(
			(SELECT opt.option_value
			   FROM pg_catalog.pg_options_to_table(c.reloptions)
					AS opt(option_name, option_value)
			  WHERE opt.option_name = 'logical_idxname'
			  LIMIT 1),
			c.relname::text)) AS phys_eq_logical,
		   EXISTS (
			SELECT 1
			  FROM pg_catalog.pg_options_to_table(c.reloptions)
					AS opt(option_name, option_value)
			 WHERE opt.option_name = 'logical_idxname') AS has_logical_idxname,
		   i.indisunique,
		   i.indisprimary
	  FROM pg_catalog.pg_index i
	  JOIN pg_catalog.pg_class c ON c.oid = i.indexrelid
	  JOIN pg_catalog.pg_class t ON t.oid = i.indrelid
	 WHERE t.relname = tbl
	 ORDER BY i.indisprimary DESC, logical_name, c.oid;
$$;

-- ============================================================
-- 3.1) Design (2): specified Key_name free -> physical == Key_name;
--      no logical_idxname; duplicate Key_name on same table fails.
-- ============================================================
CREATE TABLE test_idx(a int, b int);
ALTER TABLE test_idx ADD INDEX idx_b_ph(b); -- physical == Key_name; no logical_idxname
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_idx');
ALTER TABLE test_idx ADD INDEX idx_b_ph(b); -- error: duplicate Key_name
ALTER TABLE test_idx ADD INDEX idx_b_2(b);  -- success
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_idx');
DROP TABLE test_idx;

-- ============================================================
-- 3.2) CREATE TABLE inline INDEX; duplicate ADD INDEX fails
-- ============================================================
CREATE TABLE test_idx(a int, b int, INDEX idx_b_t(b));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_idx');
ALTER TABLE test_idx ADD INDEX idx_b_t(b);   -- error: duplicate Key_name
ALTER TABLE test_idx ADD INDEX idx_b_t_2(b); -- success
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_idx');
DROP TABLE test_idx;

-- ============================================================
-- 3.3) Design (3): same Key_name on two tables -> second mangled,
--      original Key_name stored as logical_idxname
-- ============================================================
CREATE TABLE test_idx(a int, b int, INDEX idx_b_t(b));
CREATE TABLE test_idx_2(a int, b int, INDEX idx_b_t(b)); -- mangled + logical_idxname
SELECT 'test_idx' AS tbl, logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_idx')
UNION ALL
SELECT 'test_idx_2' AS tbl, logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_idx_2')
ORDER BY 1, logical_name;
SELECT (SELECT physical_name FROM pg_temp.idx_names('test_idx')) <>
	   (SELECT physical_name FROM pg_temp.idx_names('test_idx_2')) AS phys_names_differ;
DROP TABLE test_idx, test_idx_2;

-- ============================================================
-- 3.4) PRIMARY KEY
-- ============================================================
-- No logical_idxname=PRIMARY; physical is table_pkey (SHOW CREATE: PRIMARY KEY)
CREATE TABLE test_pk(a int, b int, PRIMARY KEY (a));
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_pk');
ALTER TABLE test_pk ADD PRIMARY KEY (b); -- error: multiple primary keys
DROP TABLE test_pk;

-- ALTER TABLE ADD PRIMARY KEY
CREATE TABLE test_pk(a int, b int);
ALTER TABLE test_pk ADD PRIMARY KEY (a);
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_pk');
-- DROP INDEX `PRIMARY` still resolves via indisprimary / table_pkey
ALTER TABLE test_pk DROP INDEX `PRIMARY`;
SELECT count(*) AS pk_left FROM pg_temp.idx_names('test_pk') WHERE indisprimary;
ALTER TABLE test_pk ADD PRIMARY KEY (b);
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_pk');
DROP TABLE test_pk;

-- Named CONSTRAINT PRIMARY KEY: constraint/index name is pk_sym, no logical_idxname
CREATE TABLE test_pk(a int, b int);
ALTER TABLE test_pk ADD CONSTRAINT pk_sym PRIMARY KEY (a);
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_pk');
DROP TABLE test_pk;

-- ============================================================
-- 3.5) UNIQUE KEY / UNIQUE INDEX
-- ============================================================
CREATE TABLE test_uq(a int, b int);
ALTER TABLE test_uq ADD UNIQUE KEY idx_uq_b(b); -- (2) free: no logical_idxname
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_uq');
ALTER TABLE test_uq ADD UNIQUE KEY idx_uq_b(b); -- error: duplicate Key_name
ALTER TABLE test_uq ADD UNIQUE INDEX idx_uq_a(a); -- success
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_uq');
DROP TABLE test_uq;

-- CREATE TABLE inline UNIQUE; (3) cross-table same Key_name -> mangled + logical_idxname
CREATE TABLE test_uq(a int, b int, UNIQUE KEY idx_uq_t(b));
CREATE TABLE test_uq_2(a int, b int, UNIQUE INDEX idx_uq_t(b));
SELECT 'test_uq' AS tbl, logical_name, phys_eq_logical, has_logical_idxname, indisunique
  FROM pg_temp.idx_names('test_uq')
UNION ALL
SELECT 'test_uq_2' AS tbl, logical_name, phys_eq_logical, has_logical_idxname, indisunique
  FROM pg_temp.idx_names('test_uq_2')
ORDER BY 1, logical_name;
SELECT (SELECT physical_name FROM pg_temp.idx_names('test_uq')) <>
	   (SELECT physical_name FROM pg_temp.idx_names('test_uq_2')) AS phys_names_differ;
DROP TABLE test_uq, test_uq_2;

-- UNIQUE + secondary index: Key_names share one namespace per table
CREATE TABLE test_mix(a int, b int, c int);
ALTER TABLE test_mix ADD UNIQUE KEY idx_mix_a(a);
ALTER TABLE test_mix ADD INDEX idx_mix_b(b);
ALTER TABLE test_mix ADD INDEX idx_mix_a(c); -- error: duplicate Key_name
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname,
	   indisunique, indisprimary
  FROM pg_temp.idx_names('test_mix');
DROP TABLE test_mix;

-- Design (1): unnamed UNIQUE (cols) -> ChooseIndexName; no logical_idxname
CREATE TABLE test_uq_unnamed(a int, b int, UNIQUE (a, b));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname, indisunique
  FROM pg_temp.idx_names('test_uq_unnamed');
DROP TABLE test_uq_unnamed;

-- ============================================================
-- 3.6) Multi-column PRIMARY KEY / UNIQUE KEY / INDEX
-- ============================================================
CREATE TABLE test_mc(a int, b int, c int,
	PRIMARY KEY (a, b),
	UNIQUE KEY uq_ac(a, c),
	INDEX idx_bc(b, c));
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_mc');
-- duplicate multi-column Key_names fail
ALTER TABLE test_mc ADD UNIQUE KEY uq_ac(b, c); -- error
ALTER TABLE test_mc ADD INDEX idx_bc(a, c);     -- error
ALTER TABLE test_mc ADD INDEX idx_abc(a, b, c); -- success
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_mc');

-- ============================================================
-- 3.7) SHOW CREATE TABLE quoting
-- Default: sql_quote_show_create=ON, sql_mode includes ANSI_QUOTES
--   => identifiers quoted with "
-- ============================================================
SHOW sql_quote_show_create;
SELECT unnest(string_to_array(ct, E'\n')) AS line
  FROM ivymy.show_create_table('test_mc') AS t(tn, ct);

-- sql_quote_show_create=ON, ANSI_QUOTES=OFF => backticks
SET sql_mode = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
SELECT unnest(string_to_array(ct, E'\n')) AS line
  FROM ivymy.show_create_table('test_mc') AS t(tn, ct);

-- sql_quote_show_create=OFF => unquoted identifiers
SET sql_quote_show_create = OFF;
SHOW sql_quote_show_create;
SELECT unnest(string_to_array(ct, E'\n')) AS line
  FROM ivymy.show_create_table('test_mc') AS t(tn, ct);

-- restore defaults
SET sql_quote_show_create = ON;
RESET sql_mode;
DROP TABLE test_mc;

-- ============================================================
-- 3.8) construct_unique_index_name(): length rules
--   physical = index_name || relation_name || md5(index_name)
--   if length < 64 (NAMEDATALEN): use as-is
--   if length >= 64: physical = prefix || md5(full_string) (len 63)
-- Force mangling by using the same Key_name on a second table.
-- ============================================================

-- (a) Short names: index+table+md5 < 64, no second hash
CREATE TABLE test_cui_s1(a int, b int, INDEX idx_cui(b));
CREATE TABLE test_cui_s2(a int, b int, INDEX idx_cui(b));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_cui_s1');

-- mangled: idx_cui || test_cui_s2 || md5('idx_cui'), length 50 < 64; has logical_idxname
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_cui_s2');
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_cui', 'test_cui_s2') AS phys_matches_fn
  FROM pg_temp.idx_names('test_cui_s2')
 WHERE logical_name = 'idx_cui';

-- DROP / resolve still uses Key_name (design 4: logical_idxname after physical)
ALTER TABLE test_cui_s2 DROP INDEX idx_cui;
SELECT count(*) AS idx_left FROM pg_temp.idx_names('test_cui_s2');
DROP TABLE test_cui_s1, test_cui_s2;

-- (b) Long names: index+table+md5 >= 64, second hash -> length 63
CREATE TABLE tbl_abcdefghijkl(a int, b int,
	INDEX idx_abcdefghijkl(b));
CREATE TABLE tbl_abcdefghijkm(a int, b int,
	INDEX idx_abcdefghijkl(b));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('tbl_abcdefghijkl');

-- rehash: prefix || md5(full_string), physical length 63; has logical_idxname
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('tbl_abcdefghijkm');
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_abcdefghijkl', 'tbl_abcdefghijkm')
	   AS phys_matches_fn,
	   length(physical_name::text) AS phys_len
  FROM pg_temp.idx_names('tbl_abcdefghijkm')
 WHERE logical_name = 'idx_abcdefghijkl';
ALTER TABLE tbl_abcdefghijkm DROP INDEX idx_abcdefghijkl;
SELECT count(*) AS idx_left
  FROM pg_temp.idx_names('tbl_abcdefghijkm');
DROP TABLE tbl_abcdefghijkl, tbl_abcdefghijkm;

-- ============================================================
-- 3.9) ivymy.table_level_index_name() SQL wrapper
--   Result matches construct_unique_index_name() / mangled physical relname.
--   Short: index+table+md5 < 64; long: rehash len 63; >64 chars errors.
-- ============================================================

-- (a) Short names
CREATE TABLE test_fn_s1(a int, b int, INDEX idx_fn(b));
CREATE TABLE test_fn_s2(a int, b int, INDEX idx_fn(b));
SELECT ivymy.table_level_index_name('idx_fn', 'test_fn_s2') AS from_fn;
SELECT ivymy.table_level_index_name('idx_fn', 'test_fn_s2') =
       physical_name::text AS matches_physical
  FROM pg_temp.idx_names('test_fn_s2')
 WHERE logical_name = 'idx_fn';
DROP TABLE test_fn_s1, test_fn_s2;

-- (b) Long names (rehash -> length 63)
CREATE TABLE tbl_fn_abcdefghij(a int, b int,
	INDEX idx_fn_abcdefghij(b));
CREATE TABLE tbl_fn_abcdefghik(a int, b int,
	INDEX idx_fn_abcdefghij(b));
SELECT ivymy.table_level_index_name('idx_fn_abcdefghij', 'tbl_fn_abcdefghik') AS from_fn;
SELECT length(ivymy.table_level_index_name('idx_fn_abcdefghij', 'tbl_fn_abcdefghik')) AS phys_len;
SELECT ivymy.table_level_index_name('idx_fn_abcdefghij', 'tbl_fn_abcdefghik') =
       physical_name::text AS matches_physical
  FROM pg_temp.idx_names('tbl_fn_abcdefghik')
 WHERE logical_name = 'idx_fn_abcdefghij';
DROP TABLE tbl_fn_abcdefghij, tbl_fn_abcdefghik;

-- (c) SQL Key_name longer than NAMEDATALEN-1: scanner truncates to 63 bytes.
-- (1) first use: truncated name free -> physical == logical, no tl_idxname
CREATE TABLE t_trunc1(a int,
    INDEX aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(a));
SELECT length(logical_name) AS log_len, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_trunc1');

-- (2) same Key_name on another table: conflict -> mangle + tl_idxname
CREATE TABLE t_trunc2(a int,
    INDEX aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(a));
SELECT physical_name, has_logical_idxname
  FROM pg_temp.idx_names('t_trunc2');

-- (3) third table, same Key_name: physical MD5 differs from (2)
CREATE TABLE t_trunc3(a int,
    INDEX aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(a));
SELECT physical_name, has_logical_idxname
  FROM pg_temp.idx_names('t_trunc3');

-- DROP INDEX by the same Key_name (truncated to 63): unmangled and mangled
ALTER TABLE t_trunc1 DROP INDEX aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;
ALTER TABLE t_trunc2 DROP INDEX aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;
ALTER TABLE t_trunc3 DROP INDEX aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;
SELECT (SELECT count(*) FROM pg_temp.idx_names('t_trunc1')) +
       (SELECT count(*) FROM pg_temp.idx_names('t_trunc2')) +
       (SELECT count(*) FROM pg_temp.idx_names('t_trunc3')) AS idx_left;
DROP TABLE t_trunc1, t_trunc2, t_trunc3;

-- ============================================================
-- 3.10) Design (1)/(2)/(3): named vs unnamed INDEX Key_name
--   INDEX idx_name(cols): (2) free -> physical == Key_name, no logical_idxname;
--                         (3) taken -> construct_unique_index_name + logical_idxname
--   INDEX (cols): (1) ChooseIndexName(); no logical_idxname; never mangle
-- ============================================================
CREATE TABLE test_named(f1 int, f2 int, INDEX idx_name(f2));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_named');

-- Unnamed: ChooseIndexName = test_unnamed1_f2_idx; free -> equal; no logical_idxname
CREATE TABLE test_unnamed1(f1 int, f2 int, INDEX (f2));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_unnamed1');

-- Another table: ChooseIndexName = test_unnamed2_f2_idx (also free)
CREATE TABLE test_unnamed2(f1 int, f2 int, INDEX (f2));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_unnamed2');
ALTER TABLE test_unnamed2 DROP INDEX test_unnamed2_f2_idx;
SELECT count(*) AS idx_left FROM pg_temp.idx_names('test_unnamed2');
DROP TABLE test_unnamed1, test_unnamed2;

-- Same table: INDEX+INDEX same cols are distinct, not merged;
-- ChooseIndexName yields table_col_idx and table_col_idx1; no logical_idxname
CREATE TABLE test_unnamed3(f1 int, f2 int, INDEX (f2), INDEX (f2));
SELECT logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_unnamed3')
 ORDER BY logical_name;
DROP TABLE test_named, test_unnamed3;

-- INDEX + UNIQUE KEY / PRIMARY KEY on same cols: keep both
CREATE TABLE test_idx_uq(f1 int, INDEX (f1), UNIQUE KEY uq_f1(f1));
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_idx_uq')
 ORDER BY logical_name;
DROP TABLE test_idx_uq;
CREATE TABLE test_idx_pk(f1 int, PRIMARY KEY (f1), INDEX (f1));
SELECT logical_name, phys_eq_logical, has_logical_idxname, indisunique, indisprimary
  FROM pg_temp.idx_names('test_idx_pk')
 ORDER BY logical_name;
DROP TABLE test_idx_pk;

-- System name taken in schema: ChooseIndexName suffixes _idx1 (not mangle);
-- still design (1): no logical_idxname
CREATE TABLE test_unnamed_occ(f1 int, INDEX test_unnamed4_f2_idx(f1));
CREATE TABLE test_unnamed4(f1 int, f2 int, INDEX (f2));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_unnamed4');
ALTER TABLE test_unnamed4 DROP INDEX test_unnamed4_f2_idx1;
SELECT count(*) AS idx_left FROM pg_temp.idx_names('test_unnamed4');
DROP TABLE test_unnamed4, test_unnamed_occ;

-- ============================================================
-- 3.11) ALTER TABLE RENAME INDEX|KEY
--   UpdateLogicalIndexName: physical = newname if free in schema (drop logical_idxname),
--   else construct_unique_index_name() + store logical_idxname.  Cannot rename PRIMARY.
-- ============================================================

-- (a) New name free in schema: Key_name == physical; no logical_idxname after rename
CREATE TABLE test_rn1(a int, b int, INDEX idx_old(b));
ALTER TABLE test_rn1 RENAME INDEX idx_old TO idx_new;
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_rn1');
-- Old Key_name no longer resolves; new Key_name does
ALTER TABLE test_rn1 DROP INDEX idx_old;
ALTER TABLE test_rn1 DROP INDEX idx_new;
SELECT count(*) AS idx_left FROM pg_temp.idx_names('test_rn1');
DROP TABLE test_rn1;

-- (b) New name occupied in schema: Key_name updated, physical mangled + logical_idxname
CREATE TABLE test_rn_hold(a int, b int, INDEX idx_taken(b));
CREATE TABLE test_rn2(a int, b int, INDEX idx_src(b));
ALTER TABLE test_rn2 RENAME INDEX idx_src TO idx_taken;
SELECT logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_rn2');
SELECT (SELECT physical_name FROM pg_temp.idx_names('test_rn2')) <>
	   (SELECT physical_name FROM pg_temp.idx_names('test_rn_hold')) AS phys_names_differ;
ALTER TABLE test_rn2 DROP INDEX idx_taken;
SELECT count(*) AS idx_left FROM pg_temp.idx_names('test_rn2');
DROP TABLE test_rn2, test_rn_hold;

-- (c) Source index already mangled: RENAME finds it by Key_name (logical_idxname)
CREATE TABLE test_rn_a(a int, b int, INDEX idx_shared(b));
CREATE TABLE test_rn_b(a int, b int, INDEX idx_shared(b));
SELECT logical_name, phys_eq_logical, has_logical_idxname FROM pg_temp.idx_names('test_rn_b');
ALTER TABLE test_rn_b RENAME INDEX idx_shared TO idx_renamed;
-- idx_renamed is free: physical unmangles; logical_idxname removed
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_rn_b');
ALTER TABLE test_rn_b DROP INDEX idx_renamed;
DROP TABLE test_rn_a, test_rn_b;

-- (d) Duplicate Key_name on the same table
CREATE TABLE test_rn3(a int, b int, INDEX idx_x(a), INDEX idx_y(b));
ALTER TABLE test_rn3 RENAME INDEX idx_x TO idx_y;
DROP TABLE test_rn3;

-- (e) Cannot rename PRIMARY (old or new Key_name): MySQL ER 1280
CREATE TABLE test_rn_pk(a int PRIMARY KEY, b int, INDEX idx_b(b));
ALTER TABLE test_rn_pk RENAME INDEX `PRIMARY` TO idx_pk;
ALTER TABLE test_rn_pk RENAME INDEX idx_b TO `PRIMARY`;
DROP TABLE test_rn_pk;

-- (f) RENAME KEY synonym
CREATE TABLE test_rn4(a int, b int, INDEX idx_k(b));
ALTER TABLE test_rn4 RENAME KEY idx_k TO idx_k2;
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('test_rn4');
DROP TABLE test_rn4;

-- 3.12) Partitioned table: parent keeps named Key_name (2: no logical_idxname);
--      children use ChooseIndexName only (1: no parent logical_idxname inherit)
CREATE TABLE test_part(a int) PARTITION BY RANGE (a);
CREATE TABLE test_part_p1 PARTITION OF test_part FOR VALUES FROM (0) TO (10);
CREATE TABLE test_part_p2 PARTITION OF test_part FOR VALUES FROM (10) TO (20);
ALTER TABLE test_part ADD INDEX idx_k(a);

-- parent: physical == Key_name; no logical_idxname
SELECT x.physical_name AS physical_name,
       x.logical_name  AS logical_name,
       x.phys_eq_logical,
       x.has_logical_idxname,
       'test_part'::text AS on_table
  FROM pg_temp.idx_names('test_part') x
 WHERE x.indisprimary = false
 ORDER BY 1;

-- children: ChooseIndexName physical; no logical_idxname
SELECT p.relname AS on_table,
       x.physical_name AS physical_name,
       x.logical_name  AS logical_name,
       x.phys_eq_logical,
       x.has_logical_idxname
  FROM (SELECT relname
          FROM pg_catalog.pg_class
         WHERE relname LIKE 'test_part_p%') p
  CROSS JOIN LATERAL pg_temp.idx_names(p.relname) x
 WHERE x.indisprimary = false
 ORDER BY 1;

DROP TABLE test_part;

-- ============================================================
-- 3.13) Design (4) + EXPLAIN: FORCE INDEX when Key_name != physical
--   Resolve: physical relname first, then logical_idxname.
--   enable_seqscan/bitmapscan=off so the index path is chosen stably
--   on a tiny table (otherwise Seq Scan wins on cost).
-- ============================================================
CREATE TABLE t_plan1(a int, b int, INDEX idx_k(b));
CREATE TABLE t_plan2(a int, b int, INDEX idx_k(b));

INSERT INTO t_plan1(a,b) VALUES (1,1),(2,1),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1),(9,1),(10,1);
INSERT INTO t_plan2(a,b) VALUES (1,1),(2,1),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1),(9,1),(10,1);
ANALYZE t_plan1;
ANALYZE t_plan2;

SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_plan2');

SET mysql.enable_indexhint = on;
SET enable_seqscan = off;
SET enable_bitmapscan = off;

-- (4) Key_name via logical_idxname when physical is mangled
EXPLAIN (COSTS OFF)
SELECT * FROM t_plan2 FORCE INDEX (idx_k) WHERE b = 1;

-- (4) physical relname also resolves (matched first)
EXPLAIN (COSTS OFF)
SELECT * FROM t_plan2
  FORCE INDEX (idx_kt_plan256d30bec2e8353bf0ab55dc1c7fbd3d1) WHERE b = 1;
-- physical == table_level_index_name('idx_k','t_plan2')
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_k', 't_plan2') AS phys_matches_fn
  FROM pg_temp.idx_names('t_plan2');

RESET enable_bitmapscan;
RESET enable_seqscan;
RESET mysql.enable_indexhint;
DROP TABLE t_plan1, t_plan2;

-- ============================================================
-- 3.14) More EXPLAIN / hint cases when logical != physical
--   USE / IGNORE / enable_indexhint=off / DML / no-hint / PRIMARY /
--   missing name / same Key_name on two tables.
-- ============================================================
CREATE TABLE t_hint1(a int, b int, INDEX idx_h(b));
CREATE TABLE t_hint2(a int, b int, INDEX idx_h(b));
INSERT INTO t_hint1(a,b) VALUES (1,1),(2,1),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1),(9,1),(10,1);
INSERT INTO t_hint2(a,b) VALUES (1,1),(2,1),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1),(9,1),(10,1);
ANALYZE t_hint1;
ANALYZE t_hint2;

-- mangled physical on t_hint2 (has logical_idxname):
-- physical == table_level_index_name('idx_h','t_hint2')
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_hint2');
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_h', 't_hint2') AS phys_matches_fn
  FROM pg_temp.idx_names('t_hint2');

SET mysql.enable_indexhint = on;

-- IGNORE INDEX by logical Key_name: do not use that index (Seq Scan)
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint2 IGNORE INDEX (idx_h) WHERE b = 1;

-- Force index paths for the remaining Index Scan assertions
SET enable_seqscan = off;
SET enable_bitmapscan = off;

-- USE INDEX by logical Key_name
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint2 USE INDEX (idx_h) WHERE b = 1;

-- FORCE INDEX FOR JOIN by logical Key_name (same resolve path as FORCE INDEX)
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint2 FORCE INDEX FOR JOIN (idx_h) WHERE b = 1;

-- Same Key_name on t_hint1 (physical == logical)
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint1 FORCE INDEX (idx_h) WHERE b = 1;

-- UPDATE / DELETE target with FORCE INDEX (logical Key_name)
EXPLAIN (COSTS OFF)
UPDATE t_hint2 FORCE INDEX (idx_h) SET a = a WHERE b = 1;
EXPLAIN (COSTS OFF)
DELETE FROM t_hint2 FORCE INDEX (idx_h) WHERE b = 1;

-- PRIMARY Key_name (no logical_idxname stored)
CREATE TABLE t_hint_pk(a int PRIMARY KEY, b int);
INSERT INTO t_hint_pk VALUES (1,1),(2,1),(3,1);
ANALYZE t_hint_pk;
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint_pk FORCE INDEX (`PRIMARY`) WHERE a = 1;
DROP TABLE t_hint_pk;

RESET enable_bitmapscan;
RESET enable_seqscan;

-- No hint: optimizer chooses freely (name mismatch must not break planning)
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint2 WHERE b = 1;

-- Missing Key_name still errors
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint2 FORCE INDEX (no_such_idx) WHERE b = 1;

-- enable_indexhint=off: logical name still resolves (no error); hint not applied
SET mysql.enable_indexhint = off;
EXPLAIN (COSTS OFF)
SELECT * FROM t_hint2 FORCE INDEX (idx_h) WHERE b = 1;

RESET mysql.enable_indexhint;
DROP TABLE t_hint1, t_hint2;

-- ============================================================
-- 3.15) SHOW / LIKE / ALTER INDEX SET when Key_name != physical
--   SHOW INDEX / SHOW CREATE TABLE / showstatistics expose
--   Key_name via COALESCE(logical_idxname, relname), not mangled physical.
--   CREATE TABLE LIKE INCLUDING INDEXES preserves Key_name.
--   ALTER INDEX SET must keep logical_idxname; direct SET of it fails.
-- ============================================================
CREATE TABLE t_meta1(a int, b int, INDEX idx_meta(b));
CREATE TABLE t_meta2(a int, b int, INDEX idx_meta(b));

SELECT logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_meta2');
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_meta', 't_meta2') AS phys_matches_fn
  FROM pg_temp.idx_names('t_meta2')
 WHERE logical_name = 'idx_meta';

-- SHOW INDEX (ivymy.show_indexes): Key_name is logical
SELECT "Key_name", "Column_name", "Non_unique", "Index_type"
  FROM ivymy.show_indexes('t_meta2')
 ORDER BY 1, 2;

 SHOW INDEX FROM t_meta1;
 SHOW INDEX FROM t_meta2;

-- information_schema-style statistics view
SELECT index_name, column_name, non_unique
  FROM mys_information_schema.showstatistics
 WHERE table_name = 't_meta2'
 ORDER BY 1, 2;

-- SHOW CREATE TABLE: KEY uses logical name, never mangled physical
SELECT (ct LIKE '%KEY "idx_meta"%') AS has_logical_key,
       (ct NOT LIKE '%idx_metat_meta2%') AS no_physical_in_create,
       (ct NOT LIKE '%logical_idxname%') AS no_logical_idxname_in_create
  FROM ivymy.show_create_table('t_meta2') AS t(tn, ct);

SHOW CREATE TABLE t_meta1;
SHOW CREATE TABLE t_meta2;
DROP TABLE t_meta1, t_meta2;

-- CREATE TABLE LIKE INCLUDING INDEXES: preserve Key_name;
-- dest physical is mangled because src already owns the free name.
CREATE TABLE t_like_src(a int, b int, INDEX idx_like(b));
CREATE TABLE t_like_dst (LIKE t_like_src INCLUDING INDEXES);
SELECT 't_like_src' AS tbl, logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_like_src')
UNION ALL
SELECT 't_like_dst' AS tbl, logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_like_dst')
 ORDER BY 1;
-- DROP by Key_name on the mangled clone
ALTER TABLE t_like_dst DROP INDEX idx_like;
SELECT count(*) AS dst_idx_left FROM pg_temp.idx_names('t_like_dst');
DROP TABLE t_like_dst, t_like_src;

-- ============================================================
-- 3.16) Design (4): DROP INDEX ON / multi-table JOIN hints
--   Resolve mangled index by Key_name (logical_idxname after physical miss).
-- ============================================================
CREATE TABLE t_drop1(a int, b int, INDEX idx_drop(b));
CREATE TABLE t_drop2(a int, b int, INDEX idx_drop(b));
SELECT logical_name, phys_eq_logical, has_logical_idxname FROM pg_temp.idx_names('t_drop2');

-- MySQL DROP INDEX ... ON table: resolve by Key_name when physical mangled
DROP INDEX idx_drop ON t_drop2;
SELECT count(*) AS t2_left FROM pg_temp.idx_names('t_drop2');
SELECT count(*) AS t1_left FROM pg_temp.idx_names('t_drop1');

-- Bare DROP INDEX when Key_name is unique in schema
DROP INDEX idx_drop;
SELECT count(*) AS t1_after_bare FROM pg_temp.idx_names('t_drop1');
DROP TABLE t_drop1, t_drop2;

-- Multi-table JOIN: same Key_name, second table mangled; FORCE INDEX by Key_name
CREATE TABLE t_join1(a int, b int, INDEX idx_j(b));
CREATE TABLE t_join2(a int, b int, INDEX idx_j(b));
INSERT INTO t_join1(a,b) VALUES (1,1),(2,1),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1),(9,1),(10,1);
INSERT INTO t_join2(a,b) VALUES (1,1),(2,1),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1),(9,1),(10,1);
ANALYZE t_join1;
ANALYZE t_join2;

SELECT logical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_join2');
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_j', 't_join2') AS phys_matches_fn
  FROM pg_temp.idx_names('t_join2')
 WHERE logical_name = 'idx_j';

SET mysql.enable_indexhint = on;
SET enable_seqscan = off;
SET enable_bitmapscan = off;

EXPLAIN (COSTS OFF)
SELECT * FROM t_join1 FORCE INDEX (idx_j)
  INNER JOIN t_join2 FORCE INDEX (idx_j) ON t_join1.b = t_join2.b
 WHERE t_join1.b = 1;

EXPLAIN (COSTS OFF)
SELECT * FROM t_join1 j1 USE INDEX (idx_j)
  INNER JOIN t_join2 j2 USE INDEX (idx_j) ON j1.b = j2.b
 WHERE j1.b = 1;

RESET enable_bitmapscan;
RESET enable_seqscan;
RESET mysql.enable_indexhint;
DROP TABLE t_join1, t_join2;

-- ============================================================
-- 3.17) logical_idxname in catalog and pg_get_indexdef
--   logical_idxname= is stored only when physical relname is mangled.
--   When physical == Key_name, reloptions omit logical_idxname; pg_get_indexdef
--   has no WITH clause until a real AM option (e.g. fillfactor) is set.
-- ============================================================

CREATE TABLE t_dhide(a int, b int, INDEX idx_dhide(b));
SELECT count(*) AS logical_idxname_rows
  FROM pg_catalog.pg_class t
  JOIN pg_catalog.pg_index i ON i.indrelid = t.oid
  JOIN pg_catalog.pg_class c ON c.oid = i.indexrelid
  CROSS JOIN pg_catalog.pg_options_to_table(c.reloptions) AS o(option_name, option_value)
 WHERE t.relname = 't_dhide'
   AND c.relname = 'idx_dhide'
   AND o.option_name = 'logical_idxname';

SELECT pg_get_indexdef(c.oid) AS indexdef,
       pg_get_indexdef(c.oid) NOT LIKE '%logical_idxname%' AS no_logical_idxname,
       pg_get_indexdef(c.oid) NOT LIKE '%WITH%' AS no_with_when_unmangled
  FROM pg_catalog.pg_class t
  JOIN pg_catalog.pg_index i ON i.indrelid = t.oid
  JOIN pg_catalog.pg_class c ON c.oid = i.indexrelid
 WHERE t.relname = 't_dhide'
   AND c.relname = 'idx_dhide';

\d+ idx_dhide

ALTER INDEX idx_dhide SET (fillfactor = 70);
SELECT pg_get_indexdef(c.oid) AS indexdef,
       pg_get_indexdef(c.oid) LIKE '%fillfactor=70%' AS shows_fillfactor,
       pg_get_indexdef(c.oid) NOT LIKE '%logical_idxname%' AS no_logical_idxname
  FROM pg_catalog.pg_class t
  JOIN pg_catalog.pg_index i ON i.indrelid = t.oid
  JOIN pg_catalog.pg_class c ON c.oid = i.indexrelid
 WHERE t.relname = 't_dhide'
   AND c.relname = 'idx_dhide';

\d+ idx_dhide
DROP TABLE t_dhide;

-- mangled physical index: logical_idxname stored and visible in pg_get_indexdef
CREATE TABLE t_dhide1(a int, b int, INDEX idx_dh(b));
CREATE TABLE t_dhide2(a int, b int, INDEX idx_dh(b));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_dhide2');
SELECT pg_get_indexdef(c.oid) LIKE '%logical_idxname%' AS shows_logical_idxname
  FROM pg_catalog.pg_class t
  JOIN pg_catalog.pg_index i ON i.indrelid = t.oid
  JOIN pg_catalog.pg_class c ON c.oid = i.indexrelid
 WHERE t.relname = 't_dhide2'
   AND c.relname = ivymy.table_level_index_name('idx_dh', 't_dhide2')::name;

-- \d+ uses physical relname (== table_level_index_name)
SELECT format('%s', physical_name) AS phys_for_describe
  FROM pg_temp.idx_names('t_dhide2');
\d+ idx_dht_dhide2ac93e1f6c6a10419cb9132a4d27aadab
DROP TABLE t_dhide1, t_dhide2;

-- ============================================================
-- 3.18) Design (4): DROP/RENAME resolve physical first, then logical_idxname
--   Mangled index: DROP/RENAME by physical relname and by Key_name both work.
-- ============================================================
CREATE TABLE t_lu1(a int, b int, INDEX idx_lu(b));
CREATE TABLE t_lu2(a int, b int, INDEX idx_lu(b));
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_lu2');

-- DROP by Key_name (logical_idxname path after physical miss on this table)
CREATE TABLE t_lu3(a int, b int, INDEX idx_lu(b));
SELECT has_logical_idxname FROM pg_temp.idx_names('t_lu3');
ALTER TABLE t_lu3 DROP INDEX idx_lu;
SELECT count(*) AS lu3_left FROM pg_temp.idx_names('t_lu3');
DROP TABLE t_lu3;

-- DROP by physical mangled name (physical path matched first)
CREATE TABLE t_lu4(a int, b int, INDEX idx_lu(b));
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_lu', 't_lu4') AS phys_matches_fn
  FROM pg_temp.idx_names('t_lu4');
ALTER TABLE t_lu4 DROP INDEX idx_lut_lu4a7e1b7f676edb39b729442dc1e3e35aa;
SELECT count(*) AS lu4_left FROM pg_temp.idx_names('t_lu4');
DROP TABLE t_lu4;

-- RENAME by Key_name (logical_idxname); free new name drops logical_idxname
CREATE TABLE t_lu5(a int, b int, INDEX idx_lu(b));
ALTER TABLE t_lu5 RENAME INDEX idx_lu TO idx_lu_new;
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_lu5');
-- RENAME source by physical name when still mangled
CREATE TABLE t_lu6a(a int, b int, INDEX idx_lu6(b));
CREATE TABLE t_lu6b(a int, b int, INDEX idx_lu6(b));
SELECT physical_name::text =
	   ivymy.table_level_index_name('idx_lu6', 't_lu6b') AS phys_matches_fn
  FROM pg_temp.idx_names('t_lu6b');
ALTER TABLE t_lu6b RENAME INDEX idx_lu6t_lu6bc72b23cc71779e0c78c06db0512b10b9
	TO idx_lu6_rn;
SELECT logical_name, physical_name, phys_eq_logical, has_logical_idxname
  FROM pg_temp.idx_names('t_lu6b');
ALTER TABLE t_lu6b DROP INDEX idx_lu6_rn;
DROP TABLE t_lu6a, t_lu6b;
ALTER TABLE t_lu5 DROP INDEX idx_lu_new;
DROP TABLE t_lu5;

-- After holder dropped, Key_name still resolves on remaining mangled index
DROP TABLE t_lu1;
ALTER TABLE t_lu2 DROP INDEX idx_lu;
SELECT count(*) AS lu2_left FROM pg_temp.idx_names('t_lu2');
DROP TABLE t_lu2;

-- ============================================================
-- 3.19) view referential_constraints and key_column_usage
-- ============================================================

-- the same schema ，UNIQUE Key_name mangled
CREATE TABLE t_kcu_1(a int, UNIQUE KEY uk_x(a));
CREATE TABLE t_kcu_2(a int, UNIQUE KEY uk_x(a));
CREATE TABLE t_kcu_3(a int PRIMARY KEY, b int, UNIQUE KEY uk_x(b));
ALTER TABLE t_kcu_3 ADD CONSTRAINT fk_x FOREIGN KEY (b) REFERENCES t_kcu_2(a);
-- key_column_usage：UNIQUE showing uk_x，not mangled phy name
SELECT CONSTRAINT_NAME, TABLE_NAME, COLUMN_NAME
  FROM mys_information_schema.key_column_usage
 WHERE TABLE_NAME IN ('t_kcu_2','t_kcu_3')
 ORDER BY TABLE_NAME, CONSTRAINT_NAME, ORDINAL_POSITION;
-- referential_constraints：UNIQUE_CONSTRAINT_NAME = uk_x
SELECT CONSTRAINT_NAME, UNIQUE_CONSTRAINT_NAME, TABLE_NAME, REFERENCED_TABLE_NAME
  FROM mys_information_schema.referential_constraints
 WHERE TABLE_NAME = 't_kcu_3';

----------- bug#8941
---- need return  id = 1, status = 1
CREATE TEMPORARY TABLE hg_force_index_a
(
    id INT NOT NULL,
    status INT NOT NULL,
    INDEX idx_status(status)
);
CREATE TEMPORARY TABLE hg_force_index_b
(
    id INT NOT NULL,
    status INT NOT NULL,
    INDEX idx_status(status)
);

INSERT INTO hg_force_index_a(id, status) VALUES (1, 1), (2, 0);
INSERT INTO hg_force_index_b(id, status) VALUES (1, 1), (2, 0);

SHOW CREATE TABLE hg_force_index_a;
SHOW CREATE TABLE hg_force_index_b;

SELECT id, status
  FROM hg_force_index_a FORCE INDEX (idx_status)
  WHERE status = 1;

SELECT id, status
  FROM hg_force_index_b FORCE INDEX (idx_status)
  WHERE status = 1;

DROP TABLE hg_force_index_a;
DROP TABLE hg_force_index_b;
