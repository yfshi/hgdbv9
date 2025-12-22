
# Copyright (c) 2021, PostgreSQL Global Development Group

# Configuration arguments for vcbuild.
use strict;
use warnings;

our $config = {
	asserts => 0,    # --enable-cassert
	#/* Begin - Bug#M0000315 */
	administrator => 0,		  # --enable-root
	#/* End - Bug#M0000315 */

	# blocksize => 8,         # --with-blocksize, 8kB by default
	# wal_blocksize => 8,     # --with-wal-blocksize, 8kB by default
	# max_funarg => 100,      # --with-max-funarg, 100 by default
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
	#/* Begin - BUG#Z00004967 */
	hg_product_name =>undef, # --with-hg_product_name
	hg_major_version =>undef, # --with-hg_major_version
	hg_extral_version =>undef, # --with-hg_extral_version
	hg_project_id =>undef, # --with-hg_project_id
	hg_patch_str =>undef, # --with-hg_patch_str
	#/* End - BUG#Z00004967 */
	#/* Begin - BUG#Z00005371 */
	hg_hgdb_version =>undef, #--with-hg_hgdb_version
	#/* End - BUG#Z00005371 */
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
	zlib      => undef,    # --with-zlib=<path>
	gmssl	  => undef,	   # --with-gmssl
	sqlite3	  => undef	   # --with-sqlite
};

1;
