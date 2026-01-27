#ifndef MYS_MESSAGE_H
#define MYS_MESSAGE_H

#include "server.h"
#include "utils/elog.h"
#include "tcop/dest.h"

extern int  mysql_read_command(StringInfo buf);
extern void mysql_send_error_message(ErrorData *edata);
extern void mysql_send_ready_for_query(CommandDest dest);
extern void mysql_end_command(const QueryCompletion *qc, CommandDest dest, bool force_undecorated_output);
extern void send_eof(int16 warnings, int16 status);
extern void send_ok(uint64 affected_rows, int16 warnings, int16 server_status, uint64 last_insert_id, char *message);
extern void net_send_ok(uint64 affected_rows, int16 warnings, int16 status_flag, uint64 last_insert_id, char *message, bool eof_identifier);
extern void send_column_definition41_packet(mysql_column_definition_t *field);
extern int  mysql_process_command(StringInfo buf);
extern void send_auth_switch_packet(char *method, uint8 *scramble);
extern char *read_switch_response(StringInfo buf);

/* Begin - ReqID:SRS-MYSQL-SQL-LOADDATA */
extern void mysql_receive_copy_begin(CopyFromState cstate);
extern int  mysql_receive_copy_data(StringInfo buf);
extern bool has_client_load_local_capabilities(void);
/* End - ReqID:SRS-MYSQL-SQL-LOADDATA */
#endif
