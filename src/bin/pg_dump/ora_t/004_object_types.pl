# Verify that Oracle object inheritance survives dump and restore.

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More;

my $node = get_new_node('object_types');
$node->init;
$node->start;

my $oracle_port = $node->safe_psql('postgres', 'SHOW ivorysql.port');
my $source_connstr = $node->connstr('postgres') . " port=$oracle_port";

$node->safe_psql('postgres', q{
CREATE SCHEMA object_dump;
CREATE PACKAGE object_dump.mode_pkg AS
  marker integer;
END;
/
CREATE TYPE object_dump.parent_t IS OBJECT (
  n integer,
  MEMBER FUNCTION value_plus(p integer DEFAULT 7) RETURN integer
) NOT FINAL;
CREATE TYPE BODY object_dump.parent_t AS
  MEMBER FUNCTION value_plus(p integer) RETURN integer IS
  BEGIN
    RETURN self.n + p;
  END;
END;
/
CREATE TYPE object_dump.child_t UNDER object_dump.parent_t (
  extra integer,
  OVERRIDING MEMBER FUNCTION value_plus(p integer DEFAULT 7) RETURN integer
);
CREATE TYPE BODY object_dump.child_t AS
  OVERRIDING MEMBER FUNCTION value_plus(p integer) RETURN integer IS
  BEGIN
    RETURN self.n + self.extra + p;
  END;
END;
/
}, connstr => $source_connstr);

# The PG port must keep using ordinary PostgreSQL syntax and lookup rules.
for my $sql (
  'CREATE TYPE object_dump.bad_t AS OBJECT (n integer)',
  'CREATE TYPE object_dump.bad_t IS OBJECT (n integer)',
  'CREATE TYPE object_dump.bad_t UNDER object_dump.parent_t (n integer)',
  'CREATE TYPE BODY object_dump.parent_t AS END',
  'SELECT object_dump.parent_t(1)',
  'SELECT object_dump.parent_t.value_plus(ROW(1)::object_dump.parent_t)')
{
  my ($ret, $stdout, $stderr) = $node->psql('postgres', $sql);
  isnt($ret, 0, "PG mode rejects $sql");
}
$node->safe_psql('postgres',
  'CREATE TYPE object_dump.pg_record AS (n integer)');

my $tempdir = TestLib::tempdir;
my $dump = "$tempdir/objects.sql";
$node->command_ok(
  ['pg_dump', '--dbname=' . $source_connstr,
   '--schema=object_dump', '--file=' . $dump],
  'dump object types');

my $sql = slurp_file($dump);
like($sql, qr/CREATE TYPE object_dump\.child_t UNDER object_dump\.parent_t\s*\(/,
  'dump uses Oracle subtype syntax');
unlike($sql, qr/UNDER\s+object_dump\.parent_t\s+AS\s+OBJECT/i,
  'dump does not emit the old subtype syntax');

$node->safe_psql('postgres', 'CREATE DATABASE objects_restored');
my $restore_connstr = $node->connstr('objects_restored') . " port=$oracle_port";
$node->command_ok(
  ['psql', '-X', '--dbname=' . $restore_connstr,
   '--set=ON_ERROR_STOP=1',
   '--file=' . $dump],
  'restore object types');

is($node->safe_psql('objects_restored', q{
SELECT object_dump.parent_t(1).value_plus();
SELECT object_dump.child_t(2, 3).value_plus();
}, connstr => $restore_connstr), "8\n12",
  'restored methods retain inheritance and declaration defaults');

# Connecting through the PG port must not silently omit Oracle objects.
my $archive = "$tempdir/objects.dump";
$node->command_ok(
  ['pg_dump', '--dbname=' . $node->connstr('postgres'),
   '--schema=object_dump', '--format=custom', '--file=' . $archive],
  'archive Oracle objects through the PG port');
$node->safe_psql('postgres', 'CREATE DATABASE archive_restored');
$node->command_ok(
  ['pg_restore', '--exit-on-error',
   '--dbname=' . $node->connstr('archive_restored'), $archive],
  'archive restore switches to Oracle mode through the PG port');
is($node->safe_psql('archive_restored',
  'SELECT object_dump.child_t(2, 3).value_plus()',
  connstr => $node->connstr('archive_restored') . " port=$oracle_port"),
  '12', 'archived methods and defaults survive restoration');

# Keep mode filtering consistent with PACKAGE, including the short aliases.
for my $mode ('pg', 'p', 'oracle', 'o')
{
  my $filtered = "$tempdir/objects_$mode.sql";
  $node->command_ok(
    ['pg_dump', '--dbname=' . $source_connstr, '--schema=object_dump',
     '-M', $mode, '--file=' . $filtered],
    "dump with mode filter $mode");
  my $filtered_sql = slurp_file($filtered);
  like($filtered_sql,
    qr/CREATE(?: OR REPLACE)?(?: NONEDITIONABLE| EDITIONABLE)? PACKAGE object_dump\.mode_pkg/i,
    "$mode filter includes packages");
  like($filtered_sql, qr/CREATE TYPE object_dump\.child_t UNDER/,
    "$mode filter includes object types like packages");
  like($filtered_sql, qr/CREATE TYPE BODY object_dump\.child_t/,
    "$mode filter includes object type bodies like packages");
  like($filtered_sql, qr/CREATE TYPE object_dump\.pg_record AS\s*\(/,
    "$mode filter preserves ordinary composite types");
}

$node->stop;
done_testing();
