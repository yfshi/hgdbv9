/* 
 * this file format is utf-8 BOM
 * for linux and windows, always set html charset to utf-8
 */

/* html report template */

/*
 * report style
 */
/* bug#z8277 */
/* no regress test case, test it manually on windows and linux */
const char *html_style = "<html><head><meta charset=\"UTF-8\"><title>Highgo Activity Session Report</title>";

const char *html_header = "<style> table {width: 1000;} table,th,td {border: 1px solid gray; border-collapse: collapse; padding: 1px;} table tr td.value,table tr td.mono{font-family: Monospace;text-align: right}table tr.parent td:not(.relhdr) {background-color: #D8E8C2;}table tr.child td {background-color: #BBDD97; border-top-style: hidden;}table tr:nth-child(even){background-color: #eee;}table tr:nth-child(odd){background-color: #fff;}table tr:hover td:not(.relhdr) {background-color:#d9ffcc}table th {color: white; background-color: #003366;}table tr:target td{background-color: #EBEDFF;}table tr:target td:first-of-type {font-weight: bold;}td.top_sql_query {text-align: left; text-overflow: ellipsis; white-space: nowrap; overflow: hidden;}</style> </head>";

/*
 * report version
 */
const char *html_version_h = "<H2></H2><table><tr><th width=\"200\">按库展示</th><th width=\"200\">发行版本</th><th width=\"100\">数据来源</th></tr>";

const char *html_version_b = "<tr class=\"parent\"><td class=\"value\" style=\"text-align: center\">{{DATABASE_NAME}}</td><td class=\"value\" style=\"text-align: center\">{{DATABASE_VERSION}}</td><td class=\"value\" style=\"text-align: center\">{{DATABASE_SOURCE}}</td></tr></table><H2></H2>";

/*
 * report sample info
 */
const char *html_sample_h = "<H2></H2><table><tr><th width=\"100\">起始时间</th><th width=\"100\">结束时间</th><th width=\"100\">计时时间</th><th width=\"100\">样本总数</th><th width=\"100\">平均会话数</th></tr>";

const char *html_sample_b = "<tr class=\"parent\"><td class=\"value\">{{START_TIME}}</td><td class=\"value\">{{END_TIME}}</td><td class=\"value\">{{DURATION}}</td><td class=\"value\">{{TOTAL_SAMPLE}}</td><td class=\"value\">{{AVG_SESSIONS}}</td></tr></table>";

/* Begin - BUG#z6128 */
const char *html_sample_d = "<ul><li>字段“样本总数”：采集时间内的样本总数</li><li>字段“平均会话数”：数据来源为 MEMORY 时， 样本总数除以计时时间；数据来源为 DB/COMBINE 时，(样本总数除以计时时间)*10。</li></ul>";
/* End - BUG#z6128 */

/*
 * report body
 */
const char *html_sample_report_body = "<H2><a Name=report_main>报告主体</a></H2><ul><li><a HREF=#top_user_events>Top 用户事件</a></li><li><a HREF=#top_sql_command_type>Top SQL 命令类型</a></li><li><a HREF=#top_sql_with_top_events>等待事件高的 Top SQL</a></li><li><a HREF=#top_session>Top 会话</a></li><li><a HREF=#top_phase>Top 执行阶段</a></li><li><a HREF=#activity_over_time>分段时间等待事件统计</a></li><li> <a HREF=#complete_list_sql_text>完整 SQL 列表</a></li></ul>";

/*
 * top event
 */
const char *html_event_t = "<H3> <a NAME=top_user_events>Top 用户事件</a></H3>";

const char *html_event_d = "<ul><li>显示客户端进程中按采样数排名前 10 的等待事件</li><li>字段“样本数”：采集时间内出现的等待事件次数</li><li>字段“活动占比”：事件样本数除以总的采样次数</li><li>字段“平均会话数”：事件采样数除以总的采样时间</li></ul>";

const char *html_event_h = "<table><tr><th width=\"200\">事件类型</th><th width=\"200\">事件名称</th><th width=\"200\">样本数</th><th width=\"200\">活动占比</th><th width=\"200\">平均会话数</th></tr>";

const char *html_event_b = "<tr class=\"parent\"> <td class=\"relhdr\" >{{event_type}}</td> <td class=\"value\" style=\"text-align: left\">{{event_event}}</td> <td class=\"value\">{{event_sample_count}}</td> <td class=\"value\">{{event_percentage}}</td> <td class=\"value\">{{event_avg}}</td>";

const char *html_events_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top sql type
 */
const char *html_sql_type_t = "<H3><a NAME=top_sql_command_type>Top SQL 命令类型</a></H3>";

const char *html_sql_type_d = "<ul><li>显示不同类型 SQL 的信息</li><li>字段“占比”：SQL 类型相同的不同 queryid 数量除以总的采样次数</li><li>字段“平均活跃会话数”：SQL 类型相同的不同 queryid 数量除以总的采样时间</li></ul>";

const char *html_sql_type_h = "<table><tr><th width=\"200\">SQL 类型</th><th width=\"200\">不同的Query ID</th><th width=\"200\">占比</th><th width=\"200\">平均活跃会话数</th></tr>";

const char *html_sql_type_b = "<tr class=\"parent\"> <td class=\"relhdr\" >{{SQL_TYPE}}</td> <td class=\"value\">{{SQL_DIFF}}</td> <td class=\"value\">{{SQL_PERCENTAGE}}</td> <td class=\"value\">{{SQL_AVG}}</td>";

const char *html_sql_type_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top sql with top event
 */
const char *html_sql_event_t = "<H3><a NAME=top_sql_with_top_events>等待事件高的 Top SQL</a></H3>";

const char *html_sql_event_d = " <ul><li>显示出现次数最多的 10 条 SQL 语句，以及对应次数最高的等待事件</li><li>字段“活动占比”：SQL 语句采样数除以总的采样次数</li><li>字段“事件占比”：等待事件采样数除以 SQL 语句采样数</li></ul>";

const char *html_sql_event_h = "<table><tr><th width=\"200\">Query Id</th><th width=\"200\">采样数</th><th width=\"200\">活动占比</th><th width=\"200\">事件名称</th><th width=\"200\">事件次数</th><th width=\"200\">事件占比</th></tr>";

const char *html_sql_event_b = " <tr class=\"parent\"><td class=\"relhdr\" style=\"text-align: left\"><a href=\"#{{QUERY_REF}}\">{{QUERY_ID}}</a></td><td class=\"value\">{{QUERY_COUNTS}}</td><td class=\"value\">{{QUERY_PRE}}</td><td class=\"value\" style=\"text-align: left\">{{QUERY_EVENT}}</td><td class=\"value\">{{QUERY_EVENT_COUNTS}}</td><td class=\"value\">{{QUERY_EVENT_AVG}}</td></tr>";

const char *html_sql_event_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top session
 */
const char *html_session_t = "<H3><a NAME=top_session>Top 会话</a></H3>";

/* Begin - BUG#z6128 */
const char *html_session_d = "<ul><li>显示会话中出现次数最高的 event 信息</li><li>字段“会话占比”：会话采样数除以总的采样次数</li><li>字段“平均会话数”：会话采样数除以总的采样时间</li><li>字段“事件名称”：会话采样中，出现次数最多的时间</li><li>字段“事件占比”：当前事件的采样数除以当前会话的总采样数</li></ul>";
/* End - BUG#z6128 */

const char *html_session_h = "<table><tr><th width=\"200\">会话 ID</th><th width=\"200\">会话占比</th><th width=\"200\">平均会话数</th><th width=\"200\">事件名称</th><th width=\"200\">事件占比</th>";

const char *html_session_b = "<tr class=\"parent\"><td class=\"relhdr\">{{SESSION_ID}}</td><td class=\"value\">{{SESSION_PRE}}</td><td class=\"value\">{{SESSION_AVG}}</td><td class=\"value\" style=\"text-align:left\">{{SESSION_EVENT}}</td><td class=\"value\">{{SESSION_EVENT_PRE}}</td></tr>";

const char *html_session_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top phase
 */
const char *html_phase_t = "<H3><a NAME=top_phase>Top 执行阶段</a></H3>";

const char *html_phase_d = "<ul><li>显示各个阶段的信息</li><li>字段“阶段次数”：开始和结束时间内次数</li><li>字段“阶段占比”：不同阶段的采样数除以总的采样次数</li><li>字段“平均会话数”：不同阶段的采样数除以总的采样时间</li></ul>";

const char *html_phase_h = "<table><tr><th width=\"200\">执行阶段</th><th width=\"200\">阶段次数</th><th width=\"200\">阶段占比</th><th width=\"200\">平均会话数</th>";

const char *html_phase_b = "<tr class=\"parent\"><td class=\"relhdr\">{{PHASE_NAME}}</td><td class=\"value\">{{PHASE_COUNT}}</td><td class=\"value\">{{PHASE_AVG}}</td><td class=\"value\" style=\"text-align:left\">{{PHASE_PRE}}</td></tr>";

const char *html_phase_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * over time info
 */
const char *html_over_time_t = "<H3><a NAME=activity_over_time>分段时间等待事件统计</a></H3>";

const char *html_over_time_d = " <ul><li>显示按时间段划分的等待事件信息</li><li>字段“采样数”：开始和结束时间内采样次数</li><li>字段“事件类型”：采样区间内事件次数排名前三的等待事件</li><li>字段“事件数”：采样区间内的等待事件次数</li><li>字段“事件占比”：事件次数除以采样区间内总的等待事件次数</li></ul>";

const char *html_over_time_h = "<table><tr><th width=\"200\">开始时间</th><th width=\"200\">结束时间</th><th width=\"200\">采样数</th><th width=\"200\">事件类型</th><th width=\"200\">事件数</th><th width=\"200\">事件占比</th></tr>";

const char *html_over_time_b = "<tr class=\"parent\"><td class=\"value\">{{ACTIVITY_START}}</td><td class=\"value\">{{ACTIVITY_END}}</td><td class=\"value\">{{ACTIVITY_COUNTS}}</td><td class=\"value\" style=\"text-align: left\">{{EVENT_NAME}}</td><td class=\"value\">{{EVENT_COUNT}}</td><td class=\"value\">{{EVENT_PRE}}</td></tr>";

const char *html_over_time_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * sql list
 */
const char *html_sql_list_t = "<H3><a NAME=complete_list_sql_text>完整 SQL 列表</a></H3>";

const char *html_sql_list_d = "<ul><li>显示SQL对应的具体信息</li></ul>";

const char *html_sql_list_h = "<table><tr><th width=\"200\">SQL ID</th><th width=\"300\">SQL 语句</th></tr>";

const char *html_sql_list_b = "<tr id=\"{{SQL_TEXT_REF}}\" class=\"parent\"><td class=\"relhdr\" style=\"text-align: left\">{{SQL_TEXT_ID}}</td><td class=\"value\" style=\"text-align: left\">{{SQL_TEXT_TEXT}}</td></tr>";

const char *html_sql_list_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * report end
 */
const char *html_end = "</body></html>";
