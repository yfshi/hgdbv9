# -*-perl-*- hey - emacs - this is a perl file

# Copyright (c) 2022, PostgreSQL Global Development Group

#
# Script that provides batch regression testing.
#
# src/tools/msvc/bat_regress.pl
#
use strict;
use warnings;

our $config;
import sys;
use Cwd;
use File::Path qw(rmtree);
use TestLib qw(get_free_port);

if(@ARGV != 1){
	die "please enter the installation  directory!\n";
}
my $startdir = getcwd();
my $pg_port = get_free_port();
my $ivy_port = get_free_port();
my $my_port = get_free_port(); # bug#6805
print "$startdir\n";
print "$ARGV[0]\n";

my $installdir = $ARGV[0];
print "install dir is : $installdir\n";

my @vcregress_pg = ("check","recoverycheck","bincheck","upgradecheck","tdecheck");
foreach $a (@vcregress_pg){
	system("vcregress_pg.bat $a");
	my $status = $? >> 8;
	exit $status if $status;
	print "=======end vcregress_pg.bat $a regression================\n";
}

chdir($installdir) or die "can not change install dir!";
rmtree("data");
chdir("bin") or die "can not change bin dir!";
system("initdb.exe -D $installdir/data -m pg --encoding=UTF-8");	#BUGM0000294
open(my $conf_file, '>>', "$installdir/data/postgresql.conf") or error_exit("can not append postgresql.conf");
print $conf_file "\n# Configuration added by pg_regress\n\n";
print $conf_file "audit.track_settings = off\n";
print $conf_file "port = $pg_port\n";
print $conf_file "ivorysql.port = $ivy_port\n";
close($conf_file);
my $status1 = $? >> 8;
exit $status1 if $status1;
system("pg_ctl.exe -D $installdir/data -l $installdir/log_pg.txt start");	#BUGM0000294
my $status2 = $? >> 8;
exit $status2 if $status2;
chdir "$startdir";
$ENV{PGPORT} = "$pg_port";
$ENV{PATH} = "$installdir/bin;$ENV{PATH}";

my @vcregress_pg_install = ("installcheck","plcheck","contribcheck","modulescheck","ecpgcheck","isolationcheck");
foreach $a (@vcregress_pg_install){
	system("vcregress_pg.bat $a");
	my $status3 = $? >> 8;
	exit $status3 if $status3;		#BUGM0000294
	print "=======end vcregress_pg.bat $a regression================\n";
}
chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change to bin dir!";
system("pg_ctl.exe -D $installdir/data stop");
my $status4 = $? >> 8;
exit $status4 if $status4;
chdir($installdir) or die "can not change install dir!";
rmtree("data");
rmtree("log_pg.txt");	#BUGM0000294
chdir "$startdir";
delete $ENV{PGPORT};


my @vcregress_ora_pg = ("check","recoverycheck","bincheck","upgradecheck");
foreach $a (@vcregress_ora_pg){
	system("vcregress_ora_pg.bat $a");
	my $status5 = $? >> 8;
	exit $status5 if $status5;
	print "=======end vcregress_ora_pg.bat $a regression================\n";
}

chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("initdb.exe -D $installdir/data -m oracle --encoding=UTF-8");	#BUGM0000294
open(my $conf_file, '>>', "$installdir/data/postgresql.conf") or error_exit("can not append postgresql.conf");
print $conf_file "\n# Configuration added by pg_regress\n\n";
print $conf_file "audit.track_settings = off\n";
print $conf_file "port = $pg_port\n";
print $conf_file "ivorysql.port = $ivy_port\n";
close($conf_file);
my $status6 = $? >> 8;
exit $status6 if $status6;
system("pg_ctl.exe -D $installdir/data -l $installdir/log_pg_ora.txt start -o \"-c ivorysql.enable_ora_default_collation=on\"");	#BUGM0000294
my $status7 = $? >> 8;
exit $status7 if $status7;
chdir "$startdir";
$ENV{PGPORT} = "$pg_port";

my @vcregress_ora_pg_install = ("installcheck","plcheck","contribcheck","modulescheck","ecpgcheck","isolationcheck");
foreach $a (@vcregress_ora_pg_install){
	system("vcregress_ora_pg.bat $a");
	my $status8 = $? >> 8;
	exit $status8 if $status8;		#BUGM0000294
	print "=======end vcregress_ora_pg.bat $a regression================\n";
}
chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("pg_ctl.exe -D $installdir/data stop");
my $status9 = $? >> 8;
exit $status9 if $status9;
chdir($installdir) or die "can not change install dir!";
rmtree("data");
rmtree("log_pg_ora.txt");	#BUGM0000294
chdir "$startdir";
delete $ENV{PGPORT};

# Begin - ReqID:SRS-MISC-MYSQL-FRAME 
my @vcregress_mys_pg = ("check","recoverycheck","bincheck","upgradecheck");
foreach $a (@vcregress_mys_pg){
	system("vcregress_mys_pg.bat $a");
	my $status15 = $? >> 8;
	exit $status15 if $status15;
	print "=======end vcregress_mys_pg.bat $a regression================\n";
}

chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("initdb.exe -D $installdir/data -m mysql --encoding=UTF-8");
open(my $conf_file, '>>', "$installdir/data/postgresql.conf") or error_exit("can not append postgresql.conf");
print $conf_file "\n# Configuration added by pg_regress\n\n";
print $conf_file "audit.track_settings = off\n";
print $conf_file "port = $pg_port\n";
print $conf_file "ivorysql.port = $ivy_port\n";
close($conf_file);
my $status16 = $? >> 8;
exit $status16 if $status16;
system("pg_ctl.exe -D $installdir/data start -o \"-c mysql.regress_test_default_value=on\"");
my $status17 = $? >> 8;
exit $status17 if $status17;
chdir "$startdir";
$ENV{PGPORT} = "$pg_port";

my @vcregress_mys_pg_install = ("installcheck","plcheck","contribcheck","modulescheck","ecpgcheck","isolationcheck");
foreach $a (@vcregress_mys_pg_install){
	system("vcregress_mys_pg.bat $a");
	my $status18 = $? >> 8;
#	exit $status18 if $status18;
	print "=======end vcregress_mys_pg.bat $a regression================\n";
}
chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("pg_ctl.exe -D $installdir/data stop");
my $status19 = $? >> 8;
exit $status19 if $status19;
chdir($installdir) or die "can not change install dir!";
rmtree("data");
chdir "$startdir";
delete $ENV{PGPORT};
# End - ReqID:SRS-MISC-MYSQL-FRAME

my @vcregress_ora = ("check","recoverycheck","bincheck","upgradecheck");
foreach $a (@vcregress_ora){
	system("vcregress_ora.bat $a");
	my $status10 = $? >> 8;
	exit $status10 if $status10;
	print "=======end vcregress_ora.bat $a regression================\n";
}

chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("initdb.exe -D $installdir/data -c normal --encoding=UTF-8");	#BUGM0000294
open(my $conf_file, '>>', "$installdir/data/postgresql.conf") or error_exit("can not append postgresql.conf");
print $conf_file "\n# Configuration added by pg_regress\n\n";
print $conf_file "audit.track_settings = off\n";
print $conf_file "port = $pg_port\n";
print $conf_file "ivorysql.port = $ivy_port\n";
close($conf_file);
my $status11 = $? >> 8;
exit $status11 if $status11;
system("pg_ctl.exe -D $installdir/data -l $installdir/log_ora.txt start -o \"-c ivorysql.enable_ora_default_collation=on\"");	#BUGM0000294
my $status12 = $? >> 8;
exit $status12 if $status12;
chdir "$startdir";
$ENV{PGPORT} = "$ivy_port";

my @vcregress_ora_install = ("installcheck","plcheck","modulescheck","ecpgcheck","isolationcheck","contribcheck");	#BUGM0000294
foreach $a (@vcregress_ora_install){
	system("vcregress_ora.bat $a");
	my $status13 = $? >> 8;
	exit $status13 if $status13;
	print "=======end vcregress_ora.bat $a regression================\n";
}
chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("pg_ctl.exe -D $installdir/data stop");
my $status14 = $? >> 8;
exit $status14 if $status14;
chdir($installdir) or die "can not change install dir!";
rmtree("data");
rmtree("log_ora.txt");	#BUGM0000294
chdir "$startdir";
# Begin - ReqID:SRS-MISC-MYSQL-FRAME 
delete $ENV{PGPORT};

my @vcregress_mys = ();
foreach $a (@vcregress_mys){
	system("vcregress_mys.bat $a");
	my $status20 = $? >> 8;
	exit $status20 if $status20;
	print "=======end vcregress_mys.bat $a regression================\n";
}

chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("initdb.exe -D $installdir/data -m mysql --encoding=UTF-8");
open(my $conf_file, '>>', "$installdir/data/postgresql.conf") or error_exit("can not append postgresql.conf");
print $conf_file "\n# Configuration added by pg_regress\n\n";
print $conf_file "audit.track_settings = off\n";
print $conf_file "port = $pg_port\n";
print $conf_file "ivorysql.port = $ivy_port\n";
print $conf_file "mysql.port = $my_port\n"; 	# bug#6805
print $conf_file "mysql.protocol_type = pg\n";	# bug#6805
close($conf_file);
my $status21 = $? >> 8;
exit $status21 if $status21;
system("pg_ctl.exe -D $installdir/data -l $installdir/log_mys.txt start -o \"-c mysql.regress_test_default_value=on\"");
my $status22 = $? >> 8;
exit $status22 if $status22;
chdir "$startdir";
$ENV{PGPORT} = "$my_port";

my @vcregress_mys_install = ("contribcheck");
foreach $a (@vcregress_mys_install){
	system("vcregress_mys.bat $a");
	my $status23 = $? >> 8;
	# exit $status23 if $status23;
	print "=======end vcregress_mys.bat $a regression================\n";
}
chdir($installdir) or die "can not change install dir!";
chdir("bin") or die "can not change bin dir!";
system("pg_ctl.exe -D $installdir/data stop");
my $status24 = $? >> 8;
exit $status24 if $status24;
chdir($installdir) or die "can not change install dir!";
rmtree("data");
rmtree("log_mys.txt");
chdir "$startdir";
# End - ReqID:SRS-MISC-MYSQL-FRAME
print "===============================finish all regress check========================================\n";
