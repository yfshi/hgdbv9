
# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More;

my $node = get_new_node('main');

$node->init;
$node->start;

$node->safe_psql(
	'postgres',
	q{
		SET ivorysql.compatible_mode TO mysql;
		CREATE SEQUENCE public.test_int2_seq AS smallint START WITH 1;
		CREATE SEQUENCE public.test_int4_seq AS integer START WITH 1;
		CREATE SEQUENCE public.test_int8_seq AS bigint START WITH 1;
		CREATE SEQUENCE public.test_uint1_seq AS ivymy.uint1 START WITH 1;
		CREATE SEQUENCE public.test_uint2_seq AS ivymy.uint2 START WITH 1;
		CREATE SEQUENCE public.test_uint3_seq AS ivymy.uint3 START WITH 1;
		CREATE SEQUENCE public.test_uint4_seq AS ivymy.uint4 START WITH 1;
		CREATE SEQUENCE public.test_uint8_seq AS ivymy.uint8 START WITH 1;
		CREATE SEQUENCE public.test_tinyint_seq AS tinyint START WITH 1;
		CREATE SEQUENCE public.test_mediumint_seq AS mediumint START WITH 1;
		CREATE SEQUENCE public.test_real_seq AS real START WITH 1;
		CREATE SEQUENCE public.test_double_precision_seq
			AS double precision START WITH 1;
	});

$node->command_like(
	[
		'pg_dump',
		'--schema-only',
		'--quote-all-identifiers',
		'postgres'
	],
	qr{
		(?=.*CREATE\ SEQUENCE\ "public"\."test_int2_seq"
		\n\s+AS\ smallint)
		(?=.*CREATE\ SEQUENCE\ "public"\."test_int4_seq"
		\n\s+AS\ integer)
		(?=.*CREATE\ SEQUENCE\ "public"\."test_int8_seq"
		\n\s+START\ WITH\ 1)
		(?=.*CREATE\ SEQUENCE\ "public"\."test_uint1_seq"
		\n\s+AS\ "ivymy"\."uint1")
		(?=.*CREATE\ SEQUENCE\ "public"\."test_uint2_seq"
		\n\s+AS\ "ivymy"\."uint2")
		(?=.*CREATE\ SEQUENCE\ "public"\."test_uint3_seq"
		\n\s+AS\ "ivymy"\."uint3")
		(?=.*CREATE\ SEQUENCE\ "public"\."test_uint4_seq"
		\n\s+AS\ "ivymy"\."uint4")
		(?=.*CREATE\ SEQUENCE\ "public"\."test_uint8_seq"
		\n\s+AS\ "ivymy"\."uint8")
		(?=.*CREATE\ SEQUENCE\ "public"\."test_tinyint_seq"
		\n\s+AS\ tinyint)
		(?=.*CREATE\ SEQUENCE\ "public"\."test_mediumint_seq"
		\n\s+AS\ mediumint)
		(?=.*CREATE\ SEQUENCE\ "public"\."test_real_seq"
		\n\s+AS\ real)
		(?=.*CREATE\ SEQUENCE\ "public"\."test_double_precision_seq"
		\n\s+AS\ double\ precision)
	}xs,
	'dump sequences with quoted type identifiers');

done_testing();
