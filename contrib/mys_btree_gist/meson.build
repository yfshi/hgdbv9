# Copyright (c) 2022-2023, PostgreSQL Global Development Group

mys_btree_gist_sources = files(
  'btree_gist.c',
  'btree_date.c'
  'btree_datetime.c',
  'btree_interval.c'
  'btree_utils_num.c',
)

if host_system == 'windows'
  btree_gist_sources += rc_lib_gen.process(win32ver_rc, extra_args: [
    '--NAME', 'mys_btree_gist',
    '--FILEDESC', 'mys_btree_gist - B-tree equivalent GiST operator classes',])
endif

mys_btree_gist = shared_module('mys_btree_gist',
  mys_btree_gist_sources,
  c_pch: pch_postgres_h,
  kwargs: contrib_mod_args,
)
contrib_targets += mys_btree_gist

install_data(
  'mys_btree_gist.control',
  'mys_btree_gist--1.0.sql',
  kwargs: contrib_data_args,
)

tests += {
  'name': 'mys_btree_gist',
  'sd': meson.current_source_dir(),
  'bd': meson.current_build_dir(),
  'regress': {
    'sql': [
      'init',
      'date',
      'datetime',
    ],
  },
}
