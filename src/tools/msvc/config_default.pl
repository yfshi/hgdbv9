
# Copyright (c) 2021, PostgreSQL Global Development Group

# Configuration arguments for vcbuild.
use strict;
use warnings;

our $config = {
	asserts => 0,    # --enable-cassert
	#/* Begin - Bug#M0000315 */
	administrator => 1,		  # --enable-root
	#/* End - Bug#M0000315 */

	# blocksize => 8,         # --with-blocksize, 8kB by default
	# wal_blocksize => 8,     # --with-wal-blocksize, 8kB by default
	# max_funarg => 100,      # --with-max-funarg, 100 by default
	#/* Begin - ReqID:SRS-SQL-PACKAGE */
	cursor_unname => 0,		# --enable-cursor-unname
	#/* End - ReqID:SRS-SQL-PACKAGE */
	ldap      => 1,        # --with-ldap
	#/* Begin - ReqID:SRS-MISC-LICENSE */
	license   => 0,		   # --with-license
	#/* End - ReqID:SRS-MISC-LICENSE */
	#/* Begin - ReqID:SRS-Performance-NUMA-DATASTRUCTURE */
	numa	  => undef,	   # --enable-numa
	#/* End - ReqID:SRS-Performance-NUMA-DATASTRUCTURE */
	# Begin - ReqID:SRS-Performance-GUC
	internal_guc => undef,
	# End - ReqID:SRS-Performance-GUC
	extraver  => undef,    # --with-extra-version=<string>
	gss       => undef,    # --with-gssapi=<path>
	icu       => undef,    # --with-icu=<path>
	lz4       => undef,    # --with-lz4=<path>
	nls       => undef,    # --enable-nls=<path>
	tap_tests => undef,    # --enable-tap-tests
	tcl       => undef,    # --with-tcl=<path>
	perl      => undef,    # --with-perl=<path>
	python    => undef,    # --with-python=<path>
	openssl   => undef,    # --with-ssl=openssl with <path>
	uuid      => undef,    # --with-uuid=<path>
	xml       => undef,    # --with-libxml=<path>
	xslt      => undef,    # --with-libxslt=<path>
	iconv     => undef,    # (not in configure, path to iconv)
	zlib      => undef     # --with-zlib=<path>
	
};

1;
