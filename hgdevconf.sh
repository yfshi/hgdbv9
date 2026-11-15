#!/bin/sh

./configure --prefix=$HGDB_HOME \
--enable-nls="zh_CN zh_TW" \
--with-libxml \
--with-libxslt \
--with-ossp-uuid \
--with-perl \
--with-tcl \
--with-python \
--enable-tap-tests \
--with-gssapi \
--with-pam \
--with-uuid=e2fs \
--enable-debug \
--enable-cassert \
--enable-depend \
--enable-root \
--with-openssl \
--with-sqlite \
--with-icu \
--with-pgport=5866 \
