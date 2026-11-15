# ReqID:TECC4.0 - V9-DD-1-1 hgdde user data protection regression.
# Author: <wangxp@highgo.com>, 2026-11-11.
# Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# 由 t/009_tecc_hgdde_test.pl 在 SQL 用例跑完之后调用，复用同一个
# 1 coordinator + 2 worker(+085 加入的 spare) 集群，用真实的 pg_dump /
# pg_restore / psql 二进制做断言，因此没有 SQL golden，只有 TAP 断言。
#
# 与 t/tecc_citus_protection.pm（历史，Citus 入口已停用）的关系：
#   · 用例清单沿用 UDP-001…UDP-023，SQL 只把 citus.* 换成 hgdde.*；
#   · 新增 2026-11-09"标记强访插件化"之后必须成立的三条失败关闭语义：
#       ① 归档里带 HG SECLABEL GUARD 守卫项；
#       ② pg_restore 到**没装 hg_sysmac 扩展**的库必须失败（而不是静默还原出
#          "有标记列 + FORCE RLS、却只有恒真策略"的表）；
#       ③ --no-hg-seclabel 是显式去标记，仍允许还原到没有扩展的库。
#
# 前置：hgdde 与 hg_sysmac 都以 shared_preload_libraries 加载、每节点都
# CREATE EXTENSION，且 hg_sysmac.global_on = on（t/009 已写进 postgresql.conf）。
package tecc_hgdde_protection;

use strict;
use warnings;
use TestLib;
use Test::More;
use IPC::Run;

sub run
{
    my ($coord, $tmp_dir, $worker) = @_;
    my ($pass, $total) = (0, 0);
    my $tap = Test::More->builder->output;
    my $emit = sub {
        my ($ok, $name, $detail) = @_;
        $total++;
        $pass++ if $ok;
        print $tap ($ok ? 'ok' : 'not ok'), " - hgdde: protection: $name\n";
        if (!$ok) {
            $detail //= '';
            $detail =~ s/\n/\n# /g;
            print $tap "# $detail\n";
        }
        return $ok;
    };
    my $sql_ok = sub {
        my ($sql, $name, $expected) = @_;
        my ($out, $err) = ('', '');
        my $rc = $coord->psql('postgres', $sql, stdout => \$out, stderr => \$err);
        return $emit->($rc == 0 && (!defined($expected) || $out eq $expected),
                      $name, "rc=$rc stdout=[$out] stderr=[$err]");
    };
    my $command = sub {
        my ($args, $name, $success, $pattern) = @_;
        local $ENV{PGAPPNAME} = $args->[0];
        my ($out, $err) = ('', '');
        my $ok = IPC::Run::run($args, '>', \$out, '2>', \$err);
        my $matches = !defined($pattern) || "$out$err" =~ $pattern;
        $emit->(($success ? $ok : !$ok) && $matches, $name,
                "stdout=[$out] stderr=[$err]");
        return ($ok, $out);
    };

    local %ENV = %ENV;
    $ENV{PGHOST} = 'localhost';
    $ENV{PGPORT} = $coord->port;
    $ENV{PGDATABASE} = 'postgres';
    $ENV{PGUSER} = $coord->safe_psql('postgres', 'SELECT current_user');
    my $archive = "$tmp_dir/hgdde_protection.dump";
    my $data = "$tmp_dir/hgdde_protection_data.dump";
    my $insert = "$tmp_dir/hgdde_protection_insert.dump";
    my $plain = "$tmp_dir/hgdde_protection_plain.dump";
    my $schema = "$tmp_dir/hgdde_protection_schema.dump";
    my $directory = "$tmp_dir/hgdde_protection_directory";
    my $sql_file = "$tmp_dir/hgdde_protection_labels.sql";
    my $nomac = 'hgdde_protection_nomac';

    return ($pass, $total) unless $sql_ok->(<<'SQL', 'setup distributed row labels');
SET hgdde.shard_count = 4;
CREATE SCHEMA protection_test;
CREATE ROLE protection_reader LOGIN;
SELECT pg_catalog.set_user_seclabel('protection_reader'::regrole, 's10:c1,c2');
CREATE TABLE protection_test.records (id int, payload text);
SELECT create_distributed_table('protection_test.records', 'id');
SELECT pg_catalog.apply_row_policy('protection_test.records'::regclass, 'apply');
SELECT pg_catalog.set_table_seclabel('protection_test.records'::regclass, 's5:c1');
SELECT pg_catalog.set_column_seclabel('protection_test.records'::regclass, 2, 's5:c1');
GRANT USAGE ON SCHEMA protection_test TO protection_reader;
GRANT SELECT ON protection_test.records TO protection_reader;
INSERT INTO protection_test.records
 (id, payload, __hg_seclabel_level__, __hg_seclabel_cats__) VALUES
 (1, E'a\t\\b\n', 5, '{1}'), (2, 'high', 15, '{1,2}'), (3, NULL, -1, '{}');
SQL

    $sql_ok->(q{SELECT count(*) = count(*) FILTER (WHERE result = 'off')
                   FROM run_command_on_all_nodes('SHOW hg_sysmac.global_export_label')},
              'export switch remains off on every node', 't');

    # A worker exporting labels must not override the coordinator's decision.
    $worker->safe_psql('postgres', 'ALTER SYSTEM SET hg_sysmac.global_export_label = on');
    $worker->safe_psql('postgres', 'SELECT pg_reload_conf()');
    my $worker_on = 0;
    for (1 .. 50) {
        if ($worker->safe_psql('postgres', 'SHOW hg_sysmac.global_export_label') eq 'on') {
            $worker_on = 1;
            last;
        }
        $worker->safe_psql('postgres', 'SELECT pg_sleep(0.1)');
    }
    $emit->($worker_on, 'worker export switch changed for mismatch test');
    my ($native_out, $native_err) = ('', '');
    my $native_rc = $coord->psql('postgres',
        'COPY protection_test.records TO STDOUT WITH (FORMAT csv, HEADER true)',
        stdout => \$native_out, stderr => \$native_err);
    $emit->($native_rc == 0 && $native_out =~ /^id,payload\n/ &&
            $native_out !~ /__hg_seclabel_/,
            'native distributed COPY honors coordinator export-off with worker export-on',
            "rc=$native_rc stdout=[$native_out] stderr=[$native_err]");
    for my $columns ('', ' (id)') {
        my ($binary_out, $binary_err) = ('', '');
        my $binary_rc = $coord->psql('postgres',
            "COPY protection_test.records$columns TO STDOUT WITH (FORMAT binary)",
            stdout => \$binary_out, stderr => \$binary_err);
        my $expected_fields = $columns eq '' ? 2 : 1;
        $emit->($binary_rc == 0 && length($binary_out) > 21 &&
                substr($binary_out, 0, 11) eq "PGCOPY\n\377\r\n\0" &&
                unpack('n', substr($binary_out, 19, 2)) == $expected_fields,
                "binary distributed COPY exports $expected_fields requested business fields",
                "rc=$binary_rc bytes=" . length($binary_out) . " stderr=[$binary_err]");
    }
    $worker->safe_psql('postgres', 'ALTER SYSTEM SET hg_sysmac.global_export_label = off');
    $worker->safe_psql('postgres', 'SELECT pg_reload_conf()');
    for (1 .. 50) {
        last if $worker->safe_psql('postgres', 'SHOW hg_sysmac.global_export_label') eq 'off';
        $worker->safe_psql('postgres', 'SELECT pg_sleep(0.1)');
    }
    $emit->($worker->safe_psql('postgres', 'SHOW hg_sysmac.global_export_label') eq 'off',
            'worker export switch restored');

    my ($ok) = $command->(['pg_dump', '-Fc', '-L', '-n', 'protection_test', '-f', $archive],
                         'privileged full backup with labels and export off', 1);
    return ($pass, $total) unless $ok;
    my (undef, $toc) = $command->(['pg_restore', '-l', '-v', $archive], 'archive TOC readable', 1);
    $emit->($toc =~ /CLEAN HG SECLABEL/ && $toc =~ /HG POLICY/ &&
            $toc =~ /HG SECURITY LABEL/ && $toc =~ /HG ROW SECURITY/,
            'archive contains optional cleanup and separate sysmac entries', $toc);
    # ReqID:TECC4.0 - 标记强访插件化之后，归档还要带一条"目标端必须真的在强制
    # 标记"的守卫项，否则纯 SQL 脚本会还原出无保护的表。
    $emit->($toc =~ /HG SECLABEL GUARD/,
            'archive carries the fail-closed seclabel guard', $toc);
    my @cleanup_ids;
    my $cleanup_chain_ok = 1;
    while ($toc =~ /^(\d+);[^\n]*CLEAN HG SECLABEL[^\n]*\n;\s*depends on:([^\n]*)/mg) {
        my ($id, $dependencies) = ($1, $2);
        $cleanup_chain_ok &&= $dependencies =~ /(?:^|\s)\Q$cleanup_ids[-1]\E(?:\s|$)/
            if @cleanup_ids;
        push @cleanup_ids, $id;
    }
    $emit->(@cleanup_ids == 5 && $cleanup_chain_ok,
            'five cleanup commands have an explicit dependency chain', $toc);
    $command->(['pg_dump', '-Fc', '-L', '-s', '-n', 'protection_test', '-f', $schema],
              'schema-only backup preserves optional cleanup recipe', 1);
    my (undef, $schema_toc) = $command->(['pg_restore', '-l', '-v', $schema],
                                       'schema-only archive TOC readable', 1);
    $emit->($schema_toc =~ /CLEAN HG SECLABEL/,
            'schema-only archive contains label cleanup', $schema_toc);

    $command->(['pg_dump', '-Fc', '-L', '-a', '-t', 'protection_test.records', '-f', $data],
              'distributed COPY data backup includes both label fields', 1);
    $command->(['pg_dump', '-Fc', '-L', '--inserts', '-a', '-t', 'protection_test.records',
               '-f', $insert], 'distributed INSERT backup includes hidden columns explicitly', 1);
    $command->(['pg_dump', '-Fc', '-n', 'protection_test', '-f', $plain],
              'default backup excludes labels', 1);
    $command->(['pg_dump', '-Fd', '-j', '2', '-L', '-n', 'protection_test', '-f', $directory],
              'parallel directory backup preserves row and object labels', 1);
    $command->(['pg_dump', '-Fp', '-L', '-n', 'protection_test', '-f', $sql_file],
              'plain SQL backup preserves labels without executing cleanup', 1);
    my (undef, $plain_sql) = $command->(['pg_restore', '-f', '-', $plain],
                                      'default archive produces SQL', 1);
    $emit->($plain_sql !~ /__hg_seclabel_|hg_sysmac_row_|SECURITY LABEL FOR sysmac/,
            'default backup omits label schema, indexes, policies and object labels', $plain_sql);
    my $label_sql = do {
        my $content = '';
        if (open(my $fh, '<', $sql_file)) {
            local $/;
            $content = <$fh>;
            close $fh;
        }
        $content;
    };
    $emit->($label_sql =~ /HG SECLABEL GUARD|DO \$hg_seclabel\$/ && $label_sql =~ /CLEAN HG SECLABEL/,
            'plain SQL script carries the guard and the inert cleanup recipe', $label_sql);
    $command->(['pg_dump', '-L', '-U', 'protection_reader', '-t', 'protection_test.records',
               '-f', "$tmp_dir/hgdde_protection_denied.sql"],
              'ordinary login cannot export labels', 0, qr/permission denied to dump mandatory/);
    $command->(['pg_dump', '-L', '--role=protection_reader', '-t', 'protection_test.records',
               '-f', "$tmp_dir/hgdde_protection_role_denied.sql"],
              'effective --role cannot export labels', 0, qr/permission denied to dump mandatory/);
    {
        # ReqID:TECC4.0 - application_name cannot authorize explicit COPY labels.
        local $ENV{PGAPPNAME} = 'pg_dump';
        my ($out, $err) = ('', '');
        my $rc = $coord->psql('postgres',
            'COPY protection_test.records (id, __hg_seclabel_level__) TO STDOUT',
            stdout => \$out, stderr => \$err);
        $emit->($rc != 0 && $err =~ /permission denied to export mandatory/,
                'spoofed pg_dump application name cannot bypass COPY export-off', $err);
    }

    $sql_ok->(q{CREATE VIEW protection_test.label_view AS SELECT
                __hg_seclabel_level__ AS security_level FROM protection_test.records},
              'user view explicitly depends on a label column');
    $command->(['pg_dump', '-Fc', '-n', 'protection_test',
               '-f', "$tmp_dir/hgdde_protection_dependency.dump"],
              'default backup rejects user dependencies on omitted columns', 0,
              qr/depends on a row security label column/);
    $sql_ok->('DROP VIEW protection_test.label_view', 'remove dependency test view');

    my $rows = $coord->safe_psql('postgres', q{SELECT id, payload,
        __hg_seclabel_level__, __hg_seclabel_cats__ FROM protection_test.records ORDER BY id});
    for my $backup ($data, $insert) {
        return ($pass, $total) unless $sql_ok->('TRUNCATE protection_test.records',
                                               'empty existing distributed target');
        ($ok) = $command->(['pg_restore', '-e', '-d', 'postgres', '-a', $backup],
                          'data-only restore through distributed COPY/INSERT', 1);
        return ($pass, $total) unless $ok;
        $sql_ok->(q{SELECT id, payload, __hg_seclabel_level__, __hg_seclabel_cats__
                    FROM protection_test.records ORDER BY id}, 'row label values round trip', $rows);
        $sql_ok->(q{SET ROLE protection_reader;
                    SELECT id::text FROM protection_test.records ORDER BY id},
                  'restored shards enforce row visibility', "1\n3");
    }

    # Restore into an already distributed table: cleanup must reach the shards
    # through hgdde's DDL propagation (DROP POLICY / ALTER TABLE ... are
    # distributed DDL in hgdde), not just change a local table.
    $sql_ok->('TRUNCATE protection_test.records', 'empty distributed target for label opt-out');
    ($ok) = $command->(['pg_restore', '-e', '-d', 'postgres', '-a', '-j', '2',
                       '--no-hg-seclabel', $data],
                      'data-only opt-out propagates cleanup to existing shards', 1);
    return ($pass, $total) unless $ok;
    # 逗"分布式表上的聚合"取数据一律改成"取行 + Perl 侧比较"：这台机器上的 hgdde
    # 快照对分布式表连 SELECT count(*) 都还不能执行（报 could not be rendered for
    # the shard），用聚合断言的话失败原因是环境而不是本特性。run_command_on_* 的
    # 输出是本地的，聚合仍然可用（下面 278/303 两处）。
    my $shard_state = $coord->safe_psql('postgres', q{SELECT success::int || '|' ||
                coalesce(result, '<null>')
                FROM run_command_on_shards('protection_test.records', $q$
                SELECT (CASE WHEN NOT c.relrowsecurity THEN 'n' ELSE 'R' END) ||
                (CASE WHEN NOT c.relforcerowsecurity THEN 'n' ELSE 'F' END) ||
                (CASE WHEN NOT EXISTS (SELECT 1 FROM pg_attribute
                    WHERE attrelid = c.oid AND NOT attisdropped
                      AND attname ~ '^__hg_seclabel_') THEN 'n' ELSE 'L' END) ||
                (CASE WHEN NOT EXISTS (SELECT 1 FROM pg_policy
                    WHERE polrelid = c.oid AND polname ~ '^hg_sysmac_row_')
                 THEN 'n' ELSE 'Y' END)
                FROM pg_class c WHERE c.oid = '%s'::regclass$q$)});
    my @shard_states = grep { length } split /\n/, $shard_state;
    $emit->(scalar(@shard_states) && !(grep { !/^1\|nnnn$/ } @shard_states),
            'active shards lose labels, system policies and MAC RLS flags', $shard_state);
    $sql_ok->('SELECT id::text FROM protection_test.records ORDER BY id',
              'distributed opt-out retains business data', "1\n2\n3");

    # 逻辑备份的范围与 hgdde 一致：不保存集群拓扑/分布键，全结构恢复后按部署
    # 流程重新 create_distributed_table()。
    for my $mode ('keep', 'omit', 'default', 'directory_keep', 'directory_omit', 'plain_keep') {
        return ($pass, $total) unless $sql_ok->('DROP SCHEMA protection_test CASCADE',
                                               "drop source schema before $mode restore");
        my @args = ('pg_restore', '-e', '-d', 'postgres');
        push @args, '--no-hg-seclabel', '-j', '2' if $mode eq 'omit' || $mode eq 'directory_omit';
        push @args, '-j', '2' if $mode eq 'directory_keep';
        push @args, $mode eq 'default' ? $plain : $mode =~ /^directory_/ ? $directory : $archive;
        @args = ('psql', '-X', '-v', 'ON_ERROR_STOP=1', '-f', $sql_file) if $mode eq 'plain_keep';
        ($ok) = $command->(\@args, "full $mode restore", 1);
        return ($pass, $total) unless $ok;
        $sql_ok->(q{SELECT create_distributed_table('protection_test.records', 'id')},
                  "redistribute $mode restored table");
        if ($mode eq 'keep' || $mode eq 'directory_keep' || $mode eq 'plain_keep') {
            $sql_ok->(q{SELECT id, payload, __hg_seclabel_level__, __hg_seclabel_cats__
                        FROM protection_test.records ORDER BY id}, 'full restore preserves label data', $rows);
            $sql_ok->(q{SELECT bool_and(success AND result = 't')
                        FROM run_command_on_shards('protection_test.records', $q$
                        SELECT relrowsecurity AND relforcerowsecurity AND
                          (SELECT count(*) = 1 FROM pg_policy WHERE polrelid = c.oid
                           AND polname = 'hg_sysmac_row_all')
                          FROM pg_class c WHERE c.oid = '%s'::regclass$q$)},
                      'every restored shard has forced RLS and one placeholder', 't');
            $sql_ok->(q{SELECT pg_catalog.get_table_seclabel('protection_test.records'::regclass),
                               pg_catalog.get_column_seclabel('protection_test.records'::regclass, 2)},
                      'full restore preserves object labels', 's5:c1|s5:c1');
            my $node_labels = $coord->safe_psql('postgres', q{SELECT success::int || '|' ||
                        coalesce(result, '<null>')
                        FROM run_command_on_all_nodes($q$
                        SELECT pg_catalog.current_setting('port') || '=' ||
                        pg_catalog.get_table_seclabel('protection_test.records'::regclass)
                        || '/' || pg_catalog.get_column_seclabel('protection_test.records'::regclass, 2)$q$)});
            my @node_label_rows = grep { length } split /\n/, $node_labels;
            $emit->(scalar(@node_label_rows) &&
                    !(grep { !m{^1\|[0-9]+=s5:c1/s5:c1$} } @node_label_rows),
                    'restored table and column labels agree on coordinator and all workers',
                    $node_labels);
        } else {
            $sql_ok->(q{SELECT count(*) FROM pg_attribute
                        WHERE attrelid = 'protection_test.records'::regclass
                        AND NOT attisdropped AND attname ~ '^__hg_seclabel_'},
                      "$mode restore removes coordinator label columns", '0');
            $sql_ok->(q{SELECT bool_and(success AND result = 't')
                        FROM run_command_on_shards('protection_test.records', $q$
                        SELECT NOT relrowsecurity AND NOT relforcerowsecurity AND
                          NOT EXISTS (SELECT 1 FROM pg_attribute WHERE attrelid = c.oid
                            AND NOT attisdropped AND attname ~ '^__hg_seclabel_')
                          FROM pg_class c WHERE c.oid = '%s'::regclass$q$)},
                      "$mode restore removes labels and RLS on every shard", 't');
            $sql_ok->('SELECT id::text FROM protection_test.records ORDER BY id',
                      "$mode restore retains business rows", "1\n2\n3");
            $sql_ok->(q{SELECT count(*) FROM pg_seclabel WHERE provider = 'sysmac'
                        AND objoid = 'protection_test.records'::regclass
                        AND classoid = 'pg_class'::regclass},
                      "$mode restore omits table and column sysmac labels", '0');
        }
    }

    # ReqID:TECC4.0 - 插件化之后"目标端有没有强访"不再由产品二进制保证。
    # 一个没建 hg_sysmac 扩展的库必须被 pg_restore 拒绝（否则会还原出
    # 带标记列 + FORCE RLS、却只有恒真策略的表）。
    # "把数据搬到没有 MAC 的库"走的是**导出侧** opt-out：默认导出（不加 -L）本来就
    # 不含任何标记列，这种归档在任何库都装得上。恢复侧的 --no-hg-seclabel 只是
    # "照常装载、再按清理配方把标记列/策略/开关去掉"，它要求目标端装了 hg_sysmac
    # —— 归档里的建表语句带着 DEFAULT current_user_level()，目标端没有那个函数时
    # 连建表都过不去。
    $sql_ok->("CREATE DATABASE $nomac", 'create target database without MAC extension');
    $command->(['pg_restore', '-d', $nomac, $archive],
              'restore with labels refuses a target without hg_sysmac', 0,
              qr/hg_sysmac/);
    ($ok) = $command->(['pg_restore', '-e', '-d', $nomac, $plain],
                      'export-side opt-out archive loads into a target without hg_sysmac', 1);
    if ($ok) {
        my $nomac_sql = 'SELECT count(*) FROM pg_attribute a '
                      . 'JOIN pg_class c ON c.oid = a.attrelid '
                      . "WHERE c.relname = 'records' AND c.relnamespace = "
                      . "(SELECT oid FROM pg_namespace WHERE nspname = 'protection_test') "
                      . "AND NOT a.attisdropped AND a.attname ~ '^__hg_seclabel_'";
        $emit->($coord->safe_psql($nomac, $nomac_sql) eq '0',
                'opt-out leaves no label columns on a target without hg_sysmac');
        $emit->($coord->safe_psql($nomac,
                "SELECT id::text FROM protection_test.records ORDER BY id") eq "1\n2\n3",
                'opt-out keeps business rows on a target without hg_sysmac');
    }
    # 纯 SQL 脚本没有 pg_restore 的连接可用，靠归档里的守卫项在目标端失败：
    # psql 默认不因单条语句失败而中止，所以这里必须用 ON_ERROR_STOP=1（也就是
    # 用户数据保护文档记载的恢复方式）。先把上一轮 opt-out 留下的对象清掉，
    # 否则脚本会在 CREATE SCHEMA 处先失败，断言就跑偏了。
    $coord->safe_psql($nomac, 'DROP SCHEMA IF EXISTS protection_test CASCADE');
    $command->(['psql', '-X', '-v', 'ON_ERROR_STOP=1', '-d', $nomac, '-f', $sql_file],
              'plain SQL guard fails on a target without hg_sysmac', 0,
              qr/mandatory access control/);
    $sql_ok->("DROP DATABASE $nomac", 'drop target database without MAC extension');

    $sql_ok->(q{SELECT pg_catalog.apply_row_policy('protection_test.records'::regclass, 'apply');
                CREATE POLICY business_read ON protection_test.records FOR SELECT USING (id > 0)},
              'ordinary RLS policy alongside sysmac');
    $command->(['pg_dump', '-Fc', '-L', '-n', 'protection_test', '-f', $archive],
              'backup mixed ordinary and mandatory policies', 1);
    $sql_ok->('DROP SCHEMA protection_test CASCADE', 'drop mixed-policy source');
    $command->(['pg_restore', '-e', '-d', 'postgres', '--no-hg-seclabel', $archive],
              'restore mixed policies without sysmac', 1);
    $sql_ok->(q{SELECT count(*) = 1 FROM pg_policy
                WHERE polrelid = 'protection_test.records'::regclass AND polname = 'business_read'},
              'ordinary policy retained', 't');
    $sql_ok->(q{SELECT relrowsecurity AND relforcerowsecurity FROM pg_class
                WHERE oid = 'protection_test.records'::regclass},
              'ordinary RLS flags retained', 't');
    $sql_ok->('DROP SCHEMA protection_test CASCADE', 'drop mixed-policy restored schema');
    $command->(['pg_restore', '-e', '-s', '-d', 'postgres', '--no-hg-seclabel', $schema],
              'schema-only opt-out executes cleanup without a data entry', 1);
    $sql_ok->(q{SELECT NOT relrowsecurity AND NOT relforcerowsecurity AND
                  NOT EXISTS (SELECT 1 FROM pg_attribute WHERE attrelid = c.oid
                    AND NOT attisdropped AND attname ~ '^__hg_seclabel_')
                  FROM pg_class c WHERE c.oid = 'protection_test.records'::regclass},
              'schema-only restore removes labels and mandatory RLS', 't');
    $sql_ok->('SELECT count(*) FROM protection_test.records',
              'schema-only restore contains no business rows', '0');
    $sql_ok->('DROP SCHEMA protection_test CASCADE; DROP ROLE protection_reader',
              'cleanup test objects');
    # ReqID:TECC4.0 - quoted semicolons must not be mistaken for SQL boundaries.
    $sql_ok->(q{CREATE SCHEMA protection_edge;
                CREATE TABLE protection_edge."records;quoted" (id int, payload text);
                SELECT create_distributed_table('protection_edge."records;quoted"', 'id');
                SELECT apply_row_policy('protection_edge."records;quoted"'::regclass, 'apply');
                INSERT INTO protection_edge."records;quoted" (id,payload) VALUES(1,'retained')},
              'setup distributed table with semicolon in quoted name');
    my $edge = "$tmp_dir/hgdde_protection_edge.dump";
    $command->(['pg_dump', '-Fc', '-L', '-n', 'protection_edge', '-f', $edge],
              'backup quoted distributed table', 1);
    $sql_ok->('DROP SCHEMA protection_edge CASCADE', 'drop quoted source');
    $command->(['pg_restore', '-e', '-j', '2', '--no-hg-seclabel', '-d', 'postgres', $edge],
              'parallel cleanup handles quoted semicolons without textual SQL splitting', 1);
    $sql_ok->(q{SELECT count(*)=1 AND min(payload)='retained' FROM protection_edge."records;quoted"},
              'quoted-name cleanup retains business data', 't');
    $sql_ok->(q{SELECT count(*)=0 FROM pg_attribute
                WHERE attrelid='protection_edge."records;quoted"'::regclass
                AND NOT attisdropped AND attname ~ '^__hg_seclabel_'},
              'quoted-name cleanup removes both row label columns', 't');
    # ReqID:TECC4.0 - 这条视图只为在协调节点造出"用户对象依赖标记列"，
    # 好让下面的 --no-hg-seclabel 清理必须因为依赖而失败；它不需要在 worker 上存在。
    # hgdde 目前传播不了建在这种"带分号的标识符"上的视图（报
    # `failed to propagate view DDL to metadata workers`；同样是视图、表名不带分号时正常），
    # 那是插件侧的转义/拆分缺口，与本特性无关，故这里关掉传播只在协调节点建。
    $sql_ok->(q{SET hgdde.enable_ddl_propagation = off;
                SELECT apply_row_policy('protection_edge."records;quoted"'::regclass,'apply');
                CREATE VIEW protection_edge.label_dependency AS
                SELECT id, payload, __hg_seclabel_level__ AS security_level
                  FROM protection_edge."records;quoted";
                SET hgdde.enable_ddl_propagation = on},
              'create user-owned dependency on a label column');
    $command->(['pg_dump', '-Fc', '-L', '-n', 'protection_edge', '-f', $edge],
              'backup explicit user dependency with labels', 1);
    $sql_ok->('DROP SCHEMA protection_edge CASCADE', 'drop dependency source');
    # ReqID:TECC4.0 - 同上：恢复视图这一步同样要关掉传播，视图才建得出来；
    # 依赖照样成立，清理配方必须因为它而失败。
    {
        local $ENV{PGOPTIONS} = '-c hgdde.enable_ddl_propagation=off';
        $command->(['pg_restore', '-e', '--no-hg-seclabel', '-d', 'postgres', $edge],
                   'cleanup refuses to cascade through a user-owned view', 0, qr/depend/);
    }
    $sql_ok->(q{SELECT id::text || '|' || payload FROM protection_edge.label_dependency},
              'failed cleanup preserves user view and its business rows', '1|retained');
    $sql_ok->('DROP SCHEMA protection_edge CASCADE', 'cleanup quoted-name and dependency fixtures');
    return ($pass, $total);
}

1;