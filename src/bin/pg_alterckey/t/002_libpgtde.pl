
# Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.

use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 3;

my $tempdir = TestLib::tempdir;

command_like(
	[ 'test_libpgtde_demo', '-D', $tempdir ],
	qr/Passed all tests./,
	'test libpgtde interfaces');
