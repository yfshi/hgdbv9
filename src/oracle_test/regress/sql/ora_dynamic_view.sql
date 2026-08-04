-- check SYS.V$SESSION SYS.V$PROCESS SYS.V$PARAMETER SYS.V$PARAMETER2
-- base before test - create public user -- cannot select
drop user if exists ora_dynamic_view cascade;
create user ora_dynamic_view password 'Highgo123';
-- without permission for public user
SET SESSION AUTHORIZATION ora_dynamic_view;
select * from v$parameter2 limit 1;
select * from v$parameter2 limit 1;
select * from v$parameter2 limit 1;
select * from v$parameter2 limit 1;
-- reset session and clean
RESET SESSION AUTHORIZATION;
drop user if exists ora_dynamic_view cascade;

-- base before test - super user -- can select
-- SYS.V$SESSION
-- default value part1: saddr audsid command ownerid taddr server schemaname schema# terminal sql_address sql_child_number
select saddr, audsid, command, ownerid, taddr, server, schemaname, schema#, terminal, sql_address, sql_child_number from sys.v$session limit 1;
-- default value part2: prev_sql_addr prev_hash_value prev_sql_id prev_child_number prev_exec_start prev_exec_id
select prev_sql_addr, prev_hash_value, prev_sql_id, prev_child_number, prev_exec_start, prev_exec_id from sys.v$session limit 1;
-- default value part3: module module_hash action action_hash client_info fixed_table_sequence row_wait_obj# row_wait_file# row_wait_block# row_wait_row# top_level_call#
select module, module_hash, action, action_hash, client_info, fixed_table_sequence, row_wait_obj#, row_wait_file#, row_wait_block#, row_wait_row#, top_level_call# from sys.v$session limit 1;
-- default value part4: last_call_et pdml_enabled failover_type failover_method failed_over pdml_status pddl_status pq_status
select last_call_et, pdml_enabled, failover_type, failover_method, failed_over, pdml_status, pddl_status, pq_status from sys.v$session limit 1;
-- default value part5: current_queue_duration client_identifier blocking_instance final_blocking_session_status final_blocking_instance final_blocking_session seq# event#
select current_queue_duration, client_identifier, blocking_instance, final_blocking_session_status, final_blocking_instance, final_blocking_session, seq#, event# from sys.v$session limit 1;
-- default value part6: p1text p1 p1raw p2text p2 p2raw p3text p3 p3raw
select p1text, p1, p1raw, p2text, p2, p2raw, p3text, p3, p3raw from sys.v$session limit 1;
-- default value part7: wait_time seconds_in_wait state wait_time_micro time_remaining_micro total_time_waited_micro heur_time_waited_micro time_since_last_wait_micro
select wait_time, seconds_in_wait, state, wait_time_micro, time_remaining_micro, total_time_waited_micro, heur_time_waited_micro, time_since_last_wait_micro from sys.v$session limit 1;
-- default value part8: service_name sql_trace sql_trace_waits sql_trace_binds sql_trace_plan_stats session_edition_id creator_addr creator_serial#
select service_name, sql_trace, sql_trace_waits, sql_trace_binds, sql_trace_plan_stats, session_edition_id, creator_addr, creator_serial# from sys.v$session limit 1;
-- default value part9: ecid sql_translation_profile_id pga_tunable_mem shard_ddl_status con_id external_name plsql_debugger_connected
select ecid, sql_translation_profile_id, pga_tunable_mem, shard_ddl_status, con_id, external_name, plsql_debugger_connected from sys.v$session limit 1;
-- support columns are values that cannot bt precisely observed, no test

-- SYS.V$PROCESS
-- default value part1: stid execution_type serial# terminal traceid tracefile background latchwait latchspin
select stid, execution_type, serial#, terminal, traceid, tracefile, background, latchwait, latchspin from sys.v$process limit 1;
-- default value part2: pga_used_mem pga_alloc_mem pga_freeable_mem pga_max_mem numa_default numa_curr con_id
select pga_used_mem, pga_alloc_mem, pga_freeable_mem, pga_max_mem, numa_default, numa_curr, con_id from sys.v$process limit 1;
-- support columns are values that cannot bt precisely observed, no test

-- SYS.V$PARAMETER
-- default value: ispdb_modifiable isinstance_modifiable isadjusted isdeprecated con_id
select ispdb_modifiable, isinstance_modifiable, isadjusted, isdeprecated, con_id from v$parameter limit 1;

-- support bool - array_nulls
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'array_nulls';
set array_nulls='off';
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'array_nulls';
reset array_nulls;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'array_nulls';

-- support string - application_name
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'application_name';
set application_name='check_app';
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'application_name';
reset application_name;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'application_name';

-- support integer - lock_timeout
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'lock_timeout';
set lock_timeout=1000;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'lock_timeout';
reset lock_timeout;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'lock_timeout';

-- support enum - xmloption
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'xmloption';
set xmloption='document';
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'xmloption';
reset xmloption;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'xmloption';

-- support real - seq_page_cost
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'seq_page_cost';
set seq_page_cost=103.1415926;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'seq_page_cost';
reset seq_page_cost;
select name, type, value, display_value, default_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, update_comment, isbasic from v$parameter where name = 'seq_page_cost';

-- v$parameter2
-- default value: ispdb_modifiable isinstance_modifiable isadjusted isdeprecated con_id
select ispdb_modifiable, isinstance_modifiable, isadjusted, isdeprecated, con_id from v$parameter2 limit 1;

-- support not enum with ordinal null
-- bool - array_nulls
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'array_nulls';
-- string - application_name
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'application_name';
-- integer - lock_timeout
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'lock_timeout';
-- real - seq_page_cost
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'seq_page_cost';

-- support enum with ordinal not null
-- enum - client_min_messages
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'client_min_messages';
set client_min_messages = 'debug5';
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'client_min_messages';
set client_min_messages = 'warning';
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'client_min_messages';
reset client_min_messages;
select type, value, display_value, isdefault, isses_modifiable, issys_modifiable, ismodified, description, ordinal, update_comment, isbasic from v$parameter2 where name = 'client_min_messages';