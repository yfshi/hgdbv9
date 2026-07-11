# contrib/ivorysql_my/Makefile

MODULE_big = ivorysql_my

override CPPFLAGS += -I./src/include

OBJS = \
	$(WIN32RES) \
	src/ivorysql_my.o \
	src/protocol/message.o \
	src/protocol/mqformat.o \
	src/protocol/server.o \
	src/protocol/printtup.o \
	src/protocol/auth/auth.o \
	src/protocol/auth/caching_sha2.o \
	src/protocol/auth/native_password.o \
	src/protocol/auth/sm3.o \
	src/case_sensitive/casesensitive.o \
	src/case_sensitive/casefunc.o \
	src/datatype/strtoll.o \
	src/datatype/myyear.o \
	src/datatype/mysenum.o \
	src/datatype/mysdate.o \
	src/datatype/mysint1.o \
	src/datatype/mysint2.o \
	src/datatype/mysint3.o \
	src/datatype/mysint4.o \
	src/datatype/mysint8.o \
	src/datatype/mysuint1.o \
	src/datatype/mysuint2.o \
	src/datatype/mysuint3.o \
	src/datatype/mysuint4.o \
	src/datatype/mysuint8.o \
	src/datatype/mysint1_index.o \
	src/datatype/mysuint1_index.o \
	src/datatype/mysuint2_index.o \
	src/datatype/mysuint4_index.o \
	src/datatype/mysuint8_index.o \
	src/datatype/mysfloat.o \
	src/datatype/mysnumeric.o \
	src/datatype/anyblob.o \
	src/datatype/anybinary.o \
	src/datatype/binary.o \
	src/datatype/varbinary.o \
	src/datatype/binary_gin.o \
	src/datatype/mystext.o	\
	src/datatype/mysbit.o \
	src/datatype/hex.o	\
	src/charset/charset.o \
	src/charset/ctype-bin.o \
	src/charset/ctype-mb.o \
	src/charset/ctype-simple.o \
	src/charset/ctype-utf8.o \
	src/charset/ctype-gbk.o \
	src/charset/ctype-gb2312.o \
	src/charset/ctype-gb18030.o \
	src/charset/ctype-big5.o \
	src/charset/ctype-eucjpms.o \
	src/charset/ctype-euc_kr.o \
	src/charset/ctype-cp932.o \
	src/charset/ctype-latin1.o \
	src/charset/ctype-sjis.o \
	src/charset/ctype-ujis.o \
	src/charset/ctype-extra.o \
	src/builtin_functions/datetime_func.o \
	src/builtin_functions/character_func.o \
	src/builtin_functions/interval.o \
	src/builtin_functions/mys_locale.o \
	src/builtin_functions/network_func.o \
	src/builtin_functions/agg_field_type.o \
	src/set/set_user_var.o \
	src/set/set_global_session_var.o \
	src/set/set_names.o \
	src/set/set_charset.o \
	src/show/show.o \
	src/information_schema/mys_information_schema.o \
	src/builtin_functions/crypto_func.o

EXTENSION = ivorysql_my

# Begin - ReqID:SRS-MISC-SQLMERGE
# SQL(s) which need to be generated only if which is specified here.
DATA = ivorysql_my--1.0.sql

all: gensql

# Begin - ReqID:SRS-MYSQL-CASE
install: install-data
# End - ReqID:SRS-MYSQL-CASE

clean: cleansql

# SQL(s) merge using perl
gensql: gensql.pl
	$(PERL) $< $(patsubst ivorysql_my--%.sql, %, $(DATA))

# Begin - ReqID:SRS-MYSQL-CASE
install-data:
	$(INSTALL_DATA) $(srcdir)/preload_mys_misc.sql '$(DESTDIR)$(datadir)/preload_mys_misc.sql'
# End- ReqID:SRS-MYSQL-CASE

# Clean the generated SQL(s).
cleansql:
	rm -f $(DATA)
# End - ReqID:SRS-MISC-SQLMERGE

# REGRESS_OPTS mainly used for installcheck or debug.
#REGRESS_OPTS = --port=3306
MYS_REGRESS = numeric_to_times \
			  cast_to_number \
			  create_table \
			  mys_information_schema \
			  hex_literal \
			  bit_literal \
			  pipes_as_concat \
			  rename \
			  multi_update \
			  mys_enum	\
			  mys_text \
			  mys_tinytext \
			  mys_mediumtext \
			  mys_longtext \
			  mys_uint1		\
			  mys_uint2		\
			  mys_uint3		\
			  mys_uint4		\
			  mys_uint8		\
			  test_binary \
			  test_varbinary \
			  test_blob \
			  test_tinyblob \
			  test_mediumblob \
			  test_longblob \
			  test_conversion \
			  insert_update \
			  multi_delete \
			  replace \
			  multi_partition \
			  casesensitive \
			  casesensitive0 \
			  casesensitive1 \
			  casesensitive2 \
			  insert_updatecase \
			  multi_updatecase \
			  multi_deletecase \
			  mys_use_case \
			  mys_year	\
			  mys_date \
			  mys_datetime \
			  mys_timestamp \
			  mys_time	\
			  show_charset \
			  mys_int1	\
			  mys_int2	\
			  mys_int3 \
			  mys_int4	\
			  mys_int8	\
			  mys_float \
			  mys_numeric \
			  mys_bit \
			  mys_copy \
			  mys-char-varchar \
			  character_func \
			  time_func	\
			  time_func2 \
			  interval \
			  period \
			  flow_control \
			  network_func \
			  weight_string \
			  operator \
			  aggregates_without_group_by \
			  set-user-var \
			  set_global_session_var \
			  set_charset \
			  set_names \
			  show_create_table \
			  mys_show \
			  create_index \
			  mys_do \
			  mys_select \
			  user \
			  namespace \
			  prepare \
			  select_into \
			  load_data \
			  not_full_group \
			  ansi_quotes \
			  keyword_unreserved_batch \
			  select_regexp_rlike \
			  autocommit \
			  autocommit2 \
			  having_alias \
			  mys_alter_modify \
			  mys_alter_modify_multi \
			  order_by_alias \
			  mys_select_hint \
			  charset_binary \
			  sql_mode \
			  proc \
			  hex_serialization_deserialization \
			  mysql_timestamp \
			  insert_duplicate \
			  case_when \
			  create_collation_from


MYS_JDBC_REGRESS =	proto_jdbc \
					proto_jdbc_8_0_28 \
					proto_jdbc_9_4_0


ifdef USE_PGXS
	PG_CONFIG = pg_config
	PGXS := $(shell $(PG_CONFIG) --pgxs)
	include $(PGXS)
else
	subdir = contrib/ivorysql_my
	top_builddir = ../..
	include $(top_builddir)/src/Makefile.global
	include $(top_srcdir)/contrib/contrib-global.mk
endif
