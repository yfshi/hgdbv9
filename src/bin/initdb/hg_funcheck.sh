#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")/.." && pwd)

# Begin - SRS-SECURITY-AUTHENTICATION-METHOD
function initsha256 {
	if ! command -v sha256sum > /dev/null 2>&1; then
		echo "sha256sum command not found!"
		exit 2
	fi
	sha256=$(cat `find $SCRIPT_DIR/bin -type f` `find $SCRIPT_DIR/lib -type f` \
		`find $SCRIPT_DIR/include -type f` `find $SCRIPT_DIR/share -type f` | sha256sum | awk '{print $1}')
	echo ${sha256} > $SCRIPT_DIR/etc/.sha256
}
# End - SRS-SECURITY-AUTHENTICATION-METHOD

# Begin - SRS-SECURITY-AUTHENTICATION-METHOD
function checksha256 {
	if ! command -v sha256sum > /dev/null 2>&1; then
		echo "sha256sum command not found!"
		exit 2
	fi
	oldsha256=$(cat $SCRIPT_DIR/etc/.sha256)
	newsha256=$(cat `find $SCRIPT_DIR/bin -type f` `find $SCRIPT_DIR/lib -type f` \
		`find $SCRIPT_DIR/include -type f` `find $SCRIPT_DIR/share -type f` | sha256sum | awk '{print $1}')

	if [ "${newsha256}" = "${oldsha256}" ]
	then
		echo "Consistency check successful"
		echo "Initialize SHA256 value: ${oldsha256}"
		echo "current SHA256 value: ${newsha256}"
		exit 1
	else
		echo "Consistency check failed"
		echo "Initialize SHA256 value: ${oldsha256}"
		echo "current SHA256 value: ${newsha256}"
		exit 2
	fi
}
# End - SRS-SECURITY-AUTHENTICATION-METHOD
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
	# Begin - SRS-SECURITY-AUTHENTICATION-METHOD
	echo "-i, --init Initialize SHA256 value"
	echo "-c, --check check SHA256 value"
	# End - SRS-SECURITY-AUTHENTICATION-METHOD
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
			initsha256;; # SRS-SECURITY-AUTHENTICATION-METHOD
		--init)
			initsha256;; # SRS-SECURITY-AUTHENTICATION-METHOD
		-c)
			checksha256;; # SRS-SECURITY-AUTHENTICATION-METHOD
		--check)
			checksha256;; # SRS-SECURITY-AUTHENTICATION-METHOD
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
