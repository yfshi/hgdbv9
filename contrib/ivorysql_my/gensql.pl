#!/usr/bin/perl
#----------------------------------------------------------------------
#
# File: gensql.pl
#
# Abstract:
#    Perl script that generates ivorysql_my--x.x.sql
#    specified by DATA in Makefile usings SQL files specified
#    in ivorysql_my_merge_sqls.
#
# Authored by xuzhuang@highgo.com, 20221011.
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# Identification:
#     contrib/ivorysql_my/gensql.pl
#
#----------------------------------------------------------------------

use strict;
use warnings;
use Carp;

use File::Spec;

my @sql_set;

sql_merge();

# The subroutine which does sql merge.
#
# usage example : 
#     <src/datatype/datatype> is specified in ivorysql_my_merge_sqls
#     <ivorysql_my--1.0.sql> is specified in Makefile
#     <ivorysql_my--1.0.sql> will be generated
#     using <src/datatype/datatype--1.0.sql>
#
# NOTE : THE VERSION IS SPECIFIED IN MAKEFILE (DATA).
sub sql_merge
{
	# Read information from ivorysql_my_merge_sqls.
	#     There are SQLs specified which need to be merged.
	open(INFO, "<", File::Spec->rel2abs("ivorysql_my_merge_sqls"))
		|| croak "Could not open file ivorysql_my_merge_sqls: $!";
	while (<INFO>)
	{
		# Delete the last tailing "\n" of this line.
		chomp($_);
		push @sql_set, "$_";
	}
	close INFO;

	foreach my $v (@ARGV)
	{
		# Delete ivorysql_my--x.x(--x.x).sql files generated before.
		unlink(File::Spec->rel2abs("ivorysql_my--$v.sql"))
			if (-e File::Spec->rel2abs("ivorysql_my--$v.sql"));

		# Open(create if necessary) ivorysql_my--x.x(--x.x).sql.
		open(OUTFILE, ">ivorysql_my--$v.sql")
			|| croak "Could not open ivorysql_my--$v.sql : $!";

		# Write "extension file loading information" into it.
		# Create extension when this version is "x.x".
		if ($v =~ /^\d+\.\d+$/)
		{
			print OUTFILE '\echo Use "CREATE EXTENSION ivorysql_my"';
			print OUTFILE " to load this file. \\quit\n";
		}
		# Update extension when this version is "x.x--x.x".
		elsif ($v =~ /^\d+\.\d+\-\-\d+\.\d+$/)
		{
			my $up2ver = (split("--", $v))[1];
			print OUTFILE '\echo Use "ALTER EXTENSION ivorysql_my UPDATE';
			print OUTFILE " TO \'$up2ver\'\" to load this file. \\quit\n";
		}
		else
		{
			croak "Invalid argument.\n";
		}

		# Write the contents of sql files
		#     (src/subdirectory/filename--<version>.sql)
		#     specified in ivorysql_my_merge_sqls,
		#     into ivorysql_my--<version>.sql.
		foreach my $set (@sql_set)
		{
			if (-e "$set--$v.sql")
			{
				# Add a newline.
				print OUTFILE "\n";
				# Open a sql file.
				open(INFILE, "<", File::Spec->rel2abs("$set--$v.sql"));
				while (my $line = <INFILE>)
				{
					# Delete the last tailing "\n" of this line.
					chomp($line);
					# Write.
					print OUTFILE "$line\n";
				}
				close INFILE;
			}
		}

		close OUTFILE;
	}
}
