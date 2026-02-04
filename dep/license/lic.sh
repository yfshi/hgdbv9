#!/usr/bin/bash

CPU_ARCH=`uname -m`
current_path=`pwd`
work_path=/dep/license/$CPU_ARCH

# First,accurately match
case "$CPU_ARCH" in
	loongarch64)
		GLIBC_VER=`getconf GNU_LIBC_VERSION | awk '{print $2}'`		
	;;
	*)
	;;
esac

for file in `ls $current_path/$work_path`
do
	if [ "$file" == "$GLIBC_VER" ]; then
		echo "$file"
		exit
	fi
done


# Secondly, fuzzy matching
for file in `ls $current_path/$work_path`
do
	is_exist=`ldd $current_path/$work_path/$file/lib/liblicense.so | grep "not found" | wc -l`
	if [ $is_exist -ne 0 ]; then
		continue;
	else
		echo "$file"
		exit;
	fi
done
