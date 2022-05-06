#!/bin/bash

function build_linux
{
	if [ -x ../configure ]
	then
		cd ../
		#cp src/include/dbbase_version.sh.sample src/include/dbbase_version.sh
		#if [ $# = 1 ] ; then
		#	str1=$1
		#	str2=${str1##*_}
		#	str1=${str1%_*}
		#	str1=${str1##*.}
		#	sed -i "s/9999/$str1/g" src/include/dbbase_version.sh
		#	sed -i "s/TEST/$str2/g" src/include/dbbase_version.sh
		#fi
		#export LD_LIBRARY_PATH=`pwd`/dep/ssl/lib:$LD_LIBRARY_PATH
		./configure --prefix=$install_dir_linux $config_linux
		make clean
		make world
		make install-world

		source_path=`pwd`
		cpu_arch=`arch`
		cd ../

		if [ "$build_repmgr" -eq "1" ] ; then
			export PG_CONFIG=$install_dir_linux/bin/pg_config
			git clone git@192.168.1.153:myzhen/repmgr-5.1.0.git
			cd repmgr-5.1.0
			./configure && make clean && make CFLAGS+=-DXXDB && make install CFLAGS+=-DXXDB && make install-doc
			cd ../
			rm -rf repmgr-5.1.0
			unset PG_CONFIG
		fi

		if [ "$build_pglogical" -eq "1" ] ; then
			export PG_CONFIG=$install_dir_linux/bin/pg_config
			git clone git@192.168.1.153:myzhen/pglogical2.3.2_for_DBbase.git
			cd pglogical2.3.2_for_DBbase
			make clean && make && make install
			cd ../
			rm -rf pglogical2.3.2_for_DBbase
			unset PG_CONFIG
		fi

		if [ "$build_pg_probackup" -eq "1" ] ; then
			git clone git@192.168.1.153:db/pg_probackup.git
			cd pg_probackup
			export original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			make USE_PGXS=1 top_srcdir=$source_path clean && make USE_PGXS=1 top_srcdir=$source_path && make USE_PGXS=1 top_srcdir=$source_path install
			cd ../
			rm -rf pg_probackup
			export PATH=$original_path
		fi

		if [ "$build_pgbackrest" -eq "1" ] ; then
			git clone git@192.168.1.153:db/pgbackrest.git
			cd pgbackrest/src
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			./configure --prefix=$install_dir_linux
			make clean && make && make install
			cd ../../
			rm -rf pgbackrest
			export PATH=$original_path
		fi

		if [ "$build_pg_jieba" -eq "1" ] ; then
			# red-pg only supports v1.0.1 version about pg_jieba
			git clone git@192.168.1.153:db/pg_jieba.git
			cd pg_jieba
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			make USE_PGXS=1 clean && make USE_PGXS=1 && make USE_PGXS=1 install
			cd ../
			rm -rf pg_jieba
			export PATH=$original_path
		fi

		if [ "$build_pg_zhparser" -eq "1" ] ; then
			git clone git@192.168.1.153:db/scws-1.2.3.git
			cd scws-1.2.3
			./configure
			sudo autoreconf -ivf
			sudo make install
			cd ../
			sudo rm -rf scws-1.2.3

			git clone git@192.168.1.153:db/zhparser.git
			cd zhparser
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			make && make install
			cd ../
			rm -rf zhparser
			export PATH=$original_path
		fi

		if [ "$build_sql_firewall" -eq "1" ] ; then
			git clone git@192.168.1.153:db/sql_firewall.git
			cd sql_firewall
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			make USE_PGXS=1 && make USE_PGXS=1 install
			cd ../
			rm -rf sql_firewall
			export PATH=$original_path
		fi

		if [ "$build_pgagent" -eq "1" ] ; then
			cd $source_path
			git clone git@192.168.1.153:db/pgagent.git
			cd pgagent/build
			make
			make install
			cp pgagent ../../install/bin/
			cd ../../
			rm -rf pgagent
			cd ../
		fi

		if [ "$build_pg_bulkload" -eq "1" ] ; then
			cd $source_path
			git clone git@192.168.1.153:db/pg_bulkload.git
			cd pg_bulkload
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			make distclean
			make
			make install
			export PATH=$original_path
			cd ../
			rm -rf pg_bulkload
			cd ../
		fi

		if [ "$build_node_exporter" -eq "1" ] ; then
			cp $source_path/dep/$cpu_arch/node_exporter $install_dir_linux/bin
			echo "**************** Executable program \"node_exporter\" installation completed ***************"
			sleep 5
		fi

		if [ "$build_postgres_exporter" -eq "1" ] ; then
			cp $source_path/dep/$cpu_arch/postgres_exporter $install_dir_linux/bin
			echo "**************** Executable program \"postgres_exporter\" installation completed ***************"
			sleep 5
		fi
		if [ "$build_sql_odbc" -eq "1" ] ; then
			git clone git@192.168.1.153:myzhen/rp-odbc.git
			cd rp-odbc
			odbc_path=`pwd`
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			./bootstrap
			./configure --prefix=$odbc_path/psqlodbc
			make && make install
			cp $odbc_path/psqlodbc/lib/psqlodbca.so $install_dir_linux/lib/
			cp $odbc_path/psqlodbc/lib/psqlodbcw.so $install_dir_linux/lib/
			cd ../
			rm -rf rp-odbc
			export PATH=$original_path
		fi
		if [ "$build_sql_oci" -eq "1" ] ; then
			git clone git@192.168.1.153:myzhen/oci.git
			cd oci/src/
			oci_path=`pwd`
			chmod 777 configure
			./configure --prefix=$oci_path/oci/ --with-libpq=$install_dir_linux CFLAGS="-O2"
			make && make install
			cp $oci_path/oci/lib/libclntsh.so $install_dir_linux/lib/
			mkdir -p $install_dir_linux/include/oci
			cp -R $oci_path/../include/linux/ $install_dir_linux/include/oci/linux/
			cp -R $oci_path/../include/win32/ $install_dir_linux/include/oci/win/
			cd ../../
			rm -rf oci
		fi
		if [ "$build_postgresql_anonymizer" -eq "1" ] ; then
			git clone git@192.168.1.153:bejin/postgresql_anonymizer.git
			cd postgresql_anonymizer
			original_path=$PATH
			export PATH=$install_dir_linux/bin:$PATH
			make clean && make && make install
			cd ../
			rm -rf postgresql_anonymizer
			export PATH=$original_path
		fi
		cd $source_path
		make check-world
	else
		echo "file configure does not exist"
		exit
	fi
}

function config_windows
{
	if [ x"$1" = x"x86" ]
	then
		# win32
		vcvarsall="\"$vcvarsall\" x86"
		export PATH=$path_win32:$PATH
		perl -e "system('perl gen_config_default.pl')"
		#perl gen_config_default.pl --with-hadoop-fdw
		perl -e "system('$vcvarsall && config win32 c:\buildenv')"
		
	else
		# win64
		vcvarsall="\"$vcvarsall\" x64"
		export PATH=$path_win64:$PATH
		perl -e "system('perl gen_config_default.pl')"
		#perl gen_config_default.pl --with-hadoop-fdw
		perl -e "system('$vcvarsall && config x64 c:\buildenv')"
	fi
}

function windows_replace_version_num
{
	perl -e "system('$vcvarsall && replaceVersion $1')"
}

function install_windows
{
	if [ x"$1" = x"x86" ]
	then
		# win32
		perl -e "system('$vcvarsall && instal install_win32')"
	else
		# win64
		perl -e "system('$vcvarsall && instal install')"
	fi
}

function build_windows
{
	if [ -f ../src/tools/msvc/config.bat ]
	then
		cd ../src/tools/msvc/
		config_windows $1
		perl -e "system('$vcvarsall && setenv')"
		perl -e "system('$vcvarsall && clean dist')"
		windows_replace_version_num $2
		perl -e "system('$vcvarsall && build')"
		install_windows $1
		perl -e "system('$vcvarsall && vcregress check')"
	else
		echo "file config.bat does not exist"
		exit
	fi
}

# main function
. ./config.conf
osname=$(uname -s)
if [ $osname = "Linux" ]
then
	. ./compile_control.conf
	build_linux $@
else
	build_windows $@
fi
