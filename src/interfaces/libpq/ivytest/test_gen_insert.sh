#!/bin/sh

anonly_str="\"begin "
i=1
#map testlibpq_prepare_anonlymous_parameter.c MAX_NARGS
max_nargs=2000
while [ $i -lt $max_nargs ];
do
	anonly_str=$anonly_str"insert into test_stmt values("
	j=1;
	while [ $j -le 10 ];
do
        if [ $j -eq 10 ]; then
		anonly_str=$anonly_str"\$$i"
	else
		anonly_str=$anonly_str"\$$i,"
	fi
	j=`expr $j + 1`
	i=`expr $i + 1`
done
	anonly_str=$anonly_str");"
done

anonly_str=$anonly_str"end;\""
echo "const char *anonymous_paramete_str = $anonly_str;" > anonly.h
