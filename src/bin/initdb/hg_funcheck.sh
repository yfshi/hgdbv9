#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")/.." && pwd)

function initmd5 {
	if ! command -v md5sum > /dev/null 2>&1; then
		echo "md5sum command not found!"
		exit 2
	fi
	md5=$(cat `find $SCRIPT_DIR/bin -type f` `find $SCRIPT_DIR/lib -type f` `find $SCRIPT_DIR/include -type f` `find $SCRIPT_DIR/share -type f` | md5sum | awk '{print $1}')
	echo ${md5} > $SCRIPT_DIR/etc/.md5
}

function checkmd5 {
	if ! command -v md5sum > /dev/null 2>&1; then
		echo "md5sum command not found!"
		exit 2
	fi
	oldmd5=$(cat $SCRIPT_DIR/etc/.md5)
	newmd5=$(cat `find $SCRIPT_DIR/bin -type f` `find $SCRIPT_DIR/lib -type f` `find $SCRIPT_DIR/include -type f` `find $SCRIPT_DIR/share -type f` | md5sum | awk '{print $1}')

	if [ "${newmd5}" = "${oldmd5}" ]
	then
		echo "Consistency check successful"
		echo "Initialize MD5 value: ${oldmd5}"
		echo "current MD5 value: ${newmd5}"
		exit 1
	else
		echo "Consistency check failed"
		echo "Initialize MD5 value: ${oldmd5}"
		echo "current MD5 value: ${newmd5}"
		#echo "check failed!" > $SCRIPT_DIR/etc/.checkfailedi
		exit 2
	fi
}

function usage {
	echo "hg_check [option]..."
	echo "General options"
	echo "-i, --init  Initialize MD5 value"
	echo "-c, --check check MD5 value"
	echo "======================================"
}

if [ ! -d $SCRIPT_DIR/etc ]
then
	mkdir -p $SCRIPT_DIR/etc
fi

if [ -n $1 ]
then
	case "$1" in
		-i)
			initmd5;;
		--init)
			initmd5;;
		-c)
			checkmd5;;
		--check)
			checkmd5;;	
		-h)
			echo "======================================"
			echo "--help or -h  How do we use it."
			usage;;
		--help)
			echo "======================================"
			echo "--help or -h  How do we use it."
			usage;;
		*)	echo "======================================"
			echo "Error,please use arg --help or -h  to select usage.";;
	esac
	shift
else
	echo "======================================"
	echo "Error,please use arg --help or -h  to select ho to use."
	echo "======================================"
fi
