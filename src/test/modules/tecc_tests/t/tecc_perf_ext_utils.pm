package tecc_perf_ext_utils;

use strict;
use warnings;
use Exporter 'import';
use IPC::Run qw(run);
use JSON::PP;
use Time::HiRes qw(gettimeofday tv_interval);

our @EXPORT = qw(
    perf_ext_slurp
    perf_ext_run_checked
    perf_ext_find_jmeter
    perf_ext_load_scenarios
    perf_ext_filter_scenarios
    perf_ext_init_schema
    perf_ext_init_data
    perf_ext_rows_from_label
    perf_ext_build_sql
    perf_ext_psql_timing
    perf_ext_explain_time
    perf_ext_write_jmx
    perf_ext_run_jmeter
    perf_ext_save_reports
);

sub perf_ext_slurp {
    my ($file) = @_;
    open(my $fh, '<', $file) or die "Cannot read $file: $!";
    local $/;
    my $text = <$fh>;
    close $fh;
    return $text;
}

sub _write_file {
    my ($file, $text) = @_;
    open(my $fh, '>', $file) or die "Cannot write $file: $!";
    print $fh $text;
    close $fh;
}

sub _tap {
    return Test::More->builder->output;
}

sub _project {
    return $ENV{PERF_EXT_PROJECT} // 'perf_ext';
}

sub _print_excerpt {
    my ($label, $stdout, $stderr) = @_;
    my $tap = _tap();
    my $text = ($stdout // '') . ($stderr // '');
    my @lines = split /\n/, $text;

    print $tap "# $label output excerpt:\n";
    for my $i (0 .. $#lines) {
        last if $i >= 80;
        print $tap "#   $lines[$i]\n";
    }
    print $tap "#   ...\n" if @lines > 80;
}

sub perf_ext_run_checked {
    my ($cmd, $label) = @_;
    my ($stdout, $stderr);
    my $ok = run($cmd, '>', \$stdout, '2>', \$stderr);

    if (!$ok) {
        _print_excerpt($label, $stdout, $stderr);
        die "$label failed\n";
    }
    return ($stdout, $stderr);
}

sub perf_ext_find_jmeter {
    return $ENV{PERF_EXT_JMETER_BIN} if $ENV{PERF_EXT_JMETER_BIN};

    my ($out, $err);
    if (run(['which', 'jmeter'], '>', \$out, '2>', \$err)) {
        chomp $out;
        return $out if $out && -x $out;
    }
    return undef;
}

sub perf_ext_load_scenarios {
    my ($module_dir) = @_;
    my $file = "$module_dir/" . _project() . "/config/scenarios.json";
    return decode_json(perf_ext_slurp($file));
}

sub perf_ext_filter_scenarios {
    my ($scenarios) = @_;
    my $filter = $ENV{TECC_TEST} // '';
    my %want;

    return @$scenarios unless $filter;
    for my $item (split /,/, $filter) {
        $item =~ s/\.sql$//;
        $want{$item} = 1;
    }
    return grep { $want{$_->{name}} } @$scenarios;
}

sub perf_ext_rows_from_label {
    my ($label) = @_;

    return 10000000 if !defined $label || $label eq '10m';
    return 30000000 if $label eq '30m';
    return 50000000 if $label eq '50m';
    return $1 * 1000000 if $label =~ /^(\d+)m$/;
    return 0 + $label;
}

sub perf_ext_init_schema {
    my ($node, $module_dir, $key_dir) = @_;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');
    my $ddl = perf_ext_slurp("$module_dir/" . _project() . "/sql/perf_ext_ddl.sql");
    my $tmp = "/tmp/tecc_perf_ext_ddl_$$.sql";

    $ddl =~ s/%KEKPATH%/$key_dir/g;
    _write_file($tmp, $ddl);
    perf_ext_run_checked([$psql, '-k', 'on', '-v', 'ON_ERROR_STOP=1',
                          '-X', '-d', $connstr, '-f', $tmp],
                         'perf_ext DDL');
    unlink $tmp;
}

sub _q {
    my ($value) = @_;
    $value =~ s/'/''/g;
    return "'$value'";
}

sub _row_values {
    my ($i) = @_;
    my $salary = sprintf('%.2f', 45000 + ($i % 200000) * 0.73);
    my $bonus = sprintf('%.2f', ($i % 50000) * 0.11);
    my $score = sprintf('%.3f', 60 + ($i % 4000) / 100.0);
    my $dept = ('RD', 'Sales', 'Finance', 'HR', 'Ops', 'Support')[$i % 6];
    my $city = ('Beijing', 'Shanghai', 'Shenzhen', 'Hangzhou', 'Chengdu')[$i % 5];
    my $job = ('Engineer', 'Manager', 'Analyst', 'Architect', 'Specialist')[$i % 5];

    return (
        $i,
        10000000 + $i,
        _q(sprintf('employee_%08d', $i)),
        $salary,
        _q(sprintf('%s road %d building %d room %d', $city, $i % 999, $i % 80, $i % 3000)),
        _q(sprintf('138%08d', $i % 100000000)),
        _q(sprintf('ID%016d', $i)),
        _q($dept),
        _q($job),
        2000 + ($i % 25),
        22 + ($i % 40),
        1 + ($i % 9),
        $bonus,
        _q($city),
        _q(sprintf('employee_%08d@example.test', $i)),
        $i % 4,
        10000000 + int($i / 10),
        $score,
        _q(sprintf('remark group %02d employee %08d', $i % 100, $i)),
    );
}

sub _query_fixture_rows {
    return _project() eq 'perf_ext' ? 10000 : 1000;
}

sub _insert_seed_table {
    my ($node, $table, $rows, $batch_size) = @_;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');
    my $tmp = "/tmp/tecc_perf_ext_seed_${table}_$$.sql";
    my @cols = qw(row_id employee_id name salary address phone idcard_no department
                  job_title hire_year age grade bonus city email status manager_id
                  performance_score remark);

    open(my $fh, '>', $tmp) or die "Cannot write $tmp: $!";
    for (my $start = 1; $start <= $rows; $start += $batch_size) {
        my $end = $start + $batch_size - 1;
        $end = $rows if $end > $rows;
        my @values;

        for my $i ($start .. $end) {
            push @values, '(' . join(',', _row_values($i)) . ')';
        }
        print $fh 'INSERT INTO ', $table, ' (', join(',', @cols), ') VALUES ',
            join(',', @values), ";\n";
    }
    close $fh;

    perf_ext_run_checked([$psql, '-k', 'on', '-v', 'ON_ERROR_STOP=1',
                          '-X', '-At', '-d', $connstr, '-f', $tmp],
                         "perf_ext seed $table");
    unlink $tmp;
}

sub _insert_query_fixture_table {
    my ($node, $table, $first_row_id, $fixture_rows, $batch_size) = @_;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');
    my $tmp = "/tmp/tecc_perf_ext_fixture_${table}_$$.sql";
    my @cols = qw(row_id employee_id name salary address phone idcard_no department
                  job_title hire_year age grade bonus city email status manager_id
                  performance_score remark);

    open(my $fh, '>', $tmp) or die "Cannot write $tmp: $!";
    for (my $start = 1; $start <= $fixture_rows; $start += $batch_size) {
        my $end = $start + $batch_size - 1;
        $end = $fixture_rows if $end > $fixture_rows;
        my @values;

        for my $i ($start .. $end) {
            my $row_id = $first_row_id + $i - 1;
            my @row = _row_values($row_id);

            $row[2] = _q("perf_eq_$i");
            $row[3] = sprintf('%.2f', 300000 + $i / 100);
            $row[4] = _q(sprintf('PERF-LIKE-MATCH-%04d', $i))
                if $i <= 1000;
            push @values, '(' . join(',', @row) . ')';
        }
        print $fh 'INSERT INTO ', $table, ' (', join(',', @cols), ') VALUES ',
            join(',', @values), ";\n";
    }
    close $fh;

    perf_ext_run_checked([$psql, '-k', 'on', '-v', 'ON_ERROR_STOP=1',
                          '-X', '-At', '-d', $connstr, '-f', $tmp],
                         "perf_ext query fixtures $table");
    unlink $tmp;
}

sub _multiply_to_rows {
    my ($node, $module_dir, $target_rows, $seed_rows) = @_;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');
    my $tpl = perf_ext_slurp("$module_dir/" . _project() . "/sql/perf_ext_multiply.sql");
    my $tmp = "/tmp/tecc_perf_ext_multiply_$$.sql";
    my $iters = int($target_rows / $seed_rows);
    my $rem = $target_rows % $seed_rows;

    open(my $fh, '>', $tmp) or die "Cannot write $tmp: $!";
    for my $iter (0 .. $iters - 1) {
        my $sql = $tpl;
        my $offset = $iter * $seed_rows;
        $sql =~ s/%OFFSET%/$offset/g;
        print $fh $sql, "\n";
    }
    if ($rem > 0) {
        my $offset = $iters * $seed_rows;
        print $fh "INSERT INTO employee_plain SELECT row_id + $offset, employee_id, name, salary, address, phone, idcard_no, department, job_title, hire_year, age, grade, bonus, city, email, status, manager_id, performance_score, remark FROM stage_employee_plain LIMIT $rem;\n";
        print $fh "INSERT INTO employee_full_enc SELECT row_id + $offset, employee_id, name, salary, address, phone, idcard_no, department, job_title, hire_year, age, grade, bonus, city, email, status, manager_id, performance_score, remark FROM stage_employee_full_enc LIMIT $rem;\n";
        print $fh "INSERT INTO employee_half_enc SELECT row_id + $offset, employee_id, name, salary, address, phone, idcard_no, department, job_title, hire_year, age, grade, bonus, city, email, status, manager_id, performance_score, remark FROM stage_employee_half_enc LIMIT $rem;\n";
    }
    close $fh;

    perf_ext_run_checked([$psql, '-k', 'off', '-v', 'ON_ERROR_STOP=1',
                          '-X', '-At', '-d', $connstr, '-f', $tmp],
                         'perf_ext multiply');
    unlink $tmp;
}

sub perf_ext_init_data {
    my ($node, $module_dir, $scenarios) = @_;
    my $tap = _tap();
    my $target_rows = $ENV{PERF_EXT_ROWS};
    my $seed_rows = $ENV{PERF_EXT_SEED_ROWS} // 100000;
    my $batch_size = $ENV{PERF_EXT_SEED_BATCH} // 500;
    my $fixture_rows = _query_fixture_rows();
    my $base_rows;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');

    if (!defined $target_rows) {
        $target_rows = 0;
        for my $scenario (@$scenarios) {
            my $rows = perf_ext_rows_from_label($scenario->{rows});
            $target_rows = $rows if $rows > $target_rows;
        }
        $target_rows = 10000000 if $target_rows <= 0;
    }

    die "PERF_EXT_ROWS must be greater than $fixture_rows"
        if $target_rows <= $fixture_rows;
    $base_rows = $target_rows - $fixture_rows;

    print $tap "# perf_ext data: seed_rows=$seed_rows target_rows=$target_rows " .
        "query_fixture_rows=$fixture_rows batch=$batch_size\n";
    perf_ext_run_checked([$psql, '-k', 'off', '-v', 'ON_ERROR_STOP=1',
                          '-X', '-At', '-d', $connstr, '-c',
                          'TRUNCATE employee_plain, employee_full_enc, employee_half_enc, stage_employee_plain, stage_employee_full_enc, stage_employee_half_enc'],
                         'perf_ext truncate');

    _insert_seed_table($node, 'stage_employee_plain', $seed_rows, $batch_size);
    _insert_seed_table($node, 'stage_employee_half_enc', $seed_rows, $batch_size);
    _insert_seed_table($node, 'stage_employee_full_enc', $seed_rows, $batch_size);
    _multiply_to_rows($node, $module_dir, $base_rows, $seed_rows);
    _insert_query_fixture_table($node, 'employee_plain', $base_rows + 1,
                                $fixture_rows, $batch_size);
    _insert_query_fixture_table($node, 'employee_half_enc', $base_rows + 1,
                                $fixture_rows, $batch_size);
    _insert_query_fixture_table($node, 'employee_full_enc', $base_rows + 1,
                                $fixture_rows, $batch_size);

    perf_ext_run_checked([$psql, '-k', 'on', '-v', 'ON_ERROR_STOP=1',
                          '-X', '-At', '-d', $connstr, '-c',
                          "SELECT 'plain='||count(*)||' half='||(SELECT count(*) FROM employee_half_enc)||' full='||(SELECT count(*) FROM employee_full_enc) FROM employee_plain"],
                         'perf_ext row count check');
}

sub _table_name {
    my ($table) = @_;
    return 'employee_plain' if $table eq 'plain';
    return 'employee_half_enc' if $table eq 'half';
    return 'employee_full_enc' if $table eq 'full';
    die "unknown perf_ext table kind: $table";
}

sub _all_columns {
    return 'row_id, employee_id, name, salary, address, phone, idcard_no, department, job_title, hire_year, age, grade, bonus, city, email, status, manager_id, performance_score, remark';
}

sub _query_columns {
    return 'row_id, employee_id, name, salary, department, job_title, hire_year, age, status';
}

sub _insert_sql_jmeter {
    my ($table) = @_;

    return 'INSERT INTO ' . $table . ' (' . _all_columns() . ') VALUES (' .
        '${seq_id},' .
        '${seq_id},' .
        q|'employee_${seq_id}',| .
        '${__Random(45000,245000)}.00,' .
        q|'${__RandomString(8,ABCDEFGHIJKLMNOPQRSTUVWXYZ)} road ${__Random(1,999)} building ${__Random(1,80)} room ${__Random(1,3000)}',| .
        q|'138${__RandomString(8,0123456789)}',| .
        q|'ID${__RandomString(16,0123456789)}',| .
        "'RD'," .
        "'Engineer'," .
        '${__Random(2000,2025)},' .
        '${__Random(22,62)},' .
        '${__Random(1,9)},' .
        '${__Random(0,50000)}.00,' .
        "'Beijing'," .
        q|'employee_${seq_id}@example.test',| .
        '${__Random(0,3)},' .
        '${__Random(10000000,19999999)},' .
        '${__Random(60,99)}.0,' .
        q|'jmeter insert ${seq_id}')|;
}

sub perf_ext_build_sql {
    my ($scenario, $for_jmeter) = @_;
    my $table = _table_name($scenario->{table});
    my $query_type = $scenario->{query_type} // 'scan';
    my $rows = perf_ext_rows_from_label($scenario->{rows});
    my $return_rows = $scenario->{return_rows} // 100;
    my $max_start = $rows - $return_rows + 1;
    my $start_id;
    my $end_id;

    $max_start = 1 if $max_start < 1;
    if ($for_jmeter) {
        $start_id = '${__Random(1,' . $max_start . ',perf_start_id)}';
        $end_id = '${perf_start_id} + ' . ($return_rows - 1);
    }
    else {
        $start_id = '1';
        $end_id = $return_rows;
    }

    if ($scenario->{kind} eq 'insert') {
        return _insert_sql_jmeter($table) if $for_jmeter;
        my @v = _row_values(900000000);
        return 'INSERT INTO ' . $table . ' (' . _all_columns() . ') VALUES (' .
            join(',', @v) . ')';
    }

    if ($scenario->{kind} eq 'update') {
        my $phone = $for_jmeter
            ? q|'139${__RandomString(8,0123456789)}'|
            : q|'13900000000'|;
        my $row_id = $for_jmeter
            ? '${__Random(1,' . $rows . ')}'
            : '1';

        return 'UPDATE ' . $table . ' SET phone = ' . $phone .
            ' WHERE row_id = ' . $row_id;
    }

    if ($query_type eq 'eq') {
        my $fixture_rows = _query_fixture_rows();
        my $name = $for_jmeter
            ? q|'perf_eq_${__Random(1,| . $fixture_rows . q|)}'|
            : q|'perf_eq_1'|;

        return 'SELECT row_id, name FROM ' . $table .
            ' WHERE name = ' . $name;
    }

    if ($query_type eq 'between') {
        my $upper = sprintf('%.2f', 300000 + $return_rows / 100);

        return 'SELECT row_id, salary FROM ' . $table .
            ' WHERE salary BETWEEN 300000.01 AND ' . $upper;
    }

    if ($query_type eq 'like') {
        return 'SELECT row_id, address FROM ' . $table .
            q| WHERE address LIKE 'PERF-LIKE-MATCH-%'|;
    }

    die "unknown perf_ext query type: $query_type"
        unless $query_type eq 'scan';

    return 'SELECT ' . _query_columns() . ' FROM ' . $table .
        ' WHERE row_id BETWEEN ' . $start_id . ' AND ' . $end_id;
}

sub _parse_psql_timing_line {
    my ($line) = @_;

    return undef unless defined $line;
    return int($1 + 0.5) if $line =~ /Time:\s*([\d.]+)\s*ms/i;
    return undef;
}

sub perf_ext_psql_timing {
    my ($node, $sql) = @_;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');
    my ($out, $err);
    my $ok = run([$psql, '-k', 'on', '-v', 'ON_ERROR_STOP=1', '-X', '-At',
                  '-d', $connstr, '-c', '\timing on', '-c', $sql],
                 '>', \$out, '2>', \$err);
    my @times;

    for my $line (split /\n/, (($out // '') . ($err // ''))) {
        my $ms = _parse_psql_timing_line($line);
        push @times, $ms if defined $ms;
    }
    return {
        ok => $ok ? 1 : 0,
        timing_ms => @times ? $times[-1] : undef,
        stdout => $out,
        stderr => $err,
    };
}

sub perf_ext_explain_time {
    my ($node, $sql) = @_;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');
    my ($out, $err);

    if ($sql =~ /^\s*(?:INSERT|UPDATE)\b/i) {
        return {
            ok => 1,
            execution_ms => undef,
            note => 'server EXPLAIN skipped for data-changing statements',
        };
    }

    if ($sql =~ /^\s*INSERT\b/i){
        return{
            ok => 1,
            execution_ms => undef,
            note => 'server EXPLAIN skipped for INSERT statements',
        };
    }

    my $ok = run([$psql, '-k', 'on', '-v', 'ON_ERROR_STOP=1', '-X', '-At',
                  '-d', $connstr, '-c', "EXPLAIN (ANALYZE, TIMING OFF, SUMMARY ON) $sql"],
                 '>', \$out, '2>', \$err);

    return { ok => 0, execution_ms => undef, error => ($err || $out || 'unknown') }
        unless $ok;
    if (($out // '') =~ /Execution Time:\s*([\d.]+)\s*ms/) {
        return { ok => 1, execution_ms => 0 + $1, plan => $out };
    }
    return { ok => 1, execution_ms => undef, plan => $out };
}

sub _xml {
    my ($s) = @_;
    $s =~ s/&/&amp;/g;
    $s =~ s/</&lt;/g;
    $s =~ s/>/&gt;/g;
    $s =~ s/"/&quot;/g;
    return $s;
}

sub perf_ext_write_jmx {
    my ($scenario, $file, $conn) = @_;
    my $threads = $scenario->{concurrency} // 1;
    my $total_samples = $scenario->{total_samples} // 0;
    my $duration = $ENV{PERF_EXT_JMETER_DURATION} // 60;
    my $ramp = $ENV{PERF_EXT_JMETER_RAMP} // ($total_samples ? 1 : 10);
    my $pool = $threads > 8 ? $threads : 8;
    my $query_type = $scenario->{kind} eq 'query'
        ? 'Select Statement' : 'Update Statement';
    my $return_rows = $scenario->{return_rows} // 0;
    my $large_threshold = _project() eq 'perf_ext_he' ? 10000 : 1000;
    my $large_result_query = $scenario->{kind} eq 'query' && $return_rows >= $large_threshold;
    my $result_set_handler = $ENV{PERF_EXT_JMETER_RESULT_SET_HANDLER}
        // ($large_result_query ? 'Count Records' : 'Store as String');
    my $autocommit = $ENV{PERF_EXT_JMETER_AUTOCOMMIT}
        // ($large_result_query ? 'false' : 'true');
    my $fetch_size = $ENV{PERF_EXT_JMETER_FETCH_SIZE}
        // ($large_result_query ? 1000 : '');
    my $preinit = $ENV{PERF_EXT_JMETER_PREINIT} // 'false';
    my $connection_age = $ENV{PERF_EXT_JMETER_CONNECTION_AGE} // 0;
    my $sql = perf_ext_build_sql($scenario, 1);
    my $loops = '-1';
    my $scheduler = 'true';
    my $duration_value = $duration;

    if ($total_samples) {
        $loops = int(($total_samples + $threads - 1) / $threads);
        $loops = 1 if $loops < 1;
        $scheduler = 'false';
        $duration_value = '';
    }

    my $xml = <<"XML";
<?xml version="1.0" encoding="UTF-8"?>
<jmeterTestPlan version="1.2" properties="5.0" jmeter="5.6.3">
  <hashTree>
    <TestPlan guiclass="TestPlanGui" testclass="TestPlan" testname="perf_ext" enabled="true">
      <boolProp name="TestPlan.functional_mode">false</boolProp>
      <boolProp name="TestPlan.tearDown_on_shutdown">true</boolProp>
      <boolProp name="TestPlan.serialize_threadgroups">false</boolProp>
      <elementProp name="TestPlan.user_defined_variables" elementType="Arguments" testclass="Arguments" testname="vars">
        <collectionProp name="Arguments.arguments"/>
      </elementProp>
    </TestPlan>
    <hashTree>
      <JDBCDataSource guiclass="TestBeanGUI" testclass="JDBCDataSource" testname="teccPool" enabled="true">
        <stringProp name="dataSource">teccPool</stringProp>
        <stringProp name="poolMax">$pool</stringProp>
        <stringProp name="timeout">30000</stringProp>
        <stringProp name="trimInterval">60000</stringProp>
        <stringProp name="autocommit">$autocommit</stringProp>
        <stringProp name="autocommit">$autocommit</stringProp>
        <stringProp name="transactionIsolation">DEFAULT</stringProp>
        <stringProp name="preinit">$preinit</stringProp>
        <stringProp name="keepAlive">true</stringProp>
        <stringProp name="connectionAge">$connection_age</stringProp>
        <stringProp name="checkQuery">select 1</stringProp>
        <stringProp name="dbUrl">@{[ _xml($conn->{url}) ]}</stringProp>
        <stringProp name="driver">org.postgresql.Driver</stringProp>
        <stringProp name="username">@{[ _xml($conn->{user}) ]}</stringProp>
        <stringProp name="password">@{[ _xml($conn->{password}) ]}</stringProp>
      </JDBCDataSource>
      <hashTree/>
      <ThreadGroup guiclass="ThreadGroupGui" testclass="ThreadGroup" testname="@{[ _xml($scenario->{name}) ]}" enabled="true">
        <stringProp name="ThreadGroup.on_sample_error">continue</stringProp>
        <elementProp name="ThreadGroup.main_controller" elementType="LoopController" testclass="LoopController" testname="loop">
          <boolProp name="LoopController.continue_forever">false</boolProp>
          <stringProp name="LoopController.loops">$loops</stringProp>
        </elementProp>
        <stringProp name="ThreadGroup.num_threads">$threads</stringProp>
        <stringProp name="ThreadGroup.ramp_time">$ramp</stringProp>
        <boolProp name="ThreadGroup.scheduler">$scheduler</boolProp>
        <stringProp name="ThreadGroup.duration">$duration_value</stringProp>
        <boolProp name="ThreadGroup.same_user_on_next_iteration">true</boolProp>
      </ThreadGroup>
      <hashTree>
        <CounterConfig guiclass="CounterConfigGui" testclass="CounterConfig" testname="seq" enabled="true">
          <stringProp name="CounterConfig.start">900000000</stringProp>
          <stringProp name="CounterConfig.end">1999999999</stringProp>
          <stringProp name="CounterConfig.incr">1</stringProp>
          <stringProp name="CounterConfig.name">seq_id</stringProp>
          <boolProp name="CounterConfig.per_user">false</boolProp>
          <boolProp name="CounterConfig.reset_on_tg_iteration">false</boolProp>
        </CounterConfig>
        <hashTree/>
        <JDBCSampler guiclass="TestBeanGUI" testclass="JDBCSampler" testname="@{[ _xml($scenario->{name}) ]}" enabled="true">
          <stringProp name="dataSource">teccPool</stringProp>
          <stringProp name="queryType">$query_type</stringProp>
          <stringProp name="query">@{[ _xml($sql) ]}</stringProp>
          <stringProp name="queryArguments"></stringProp>
          <stringProp name="queryArgumentsTypes"></stringProp>
          <stringProp name="variableNames"></stringProp>
          <stringProp name="resultVariable"></stringProp>
          <stringProp name="queryTimeout">300000</stringProp>
          <stringProp name="fetchSize">$fetch_size</stringProp>
          <stringProp name="resultSetHandler">@{[ _xml($result_set_handler)]}</stringProp>
        </JDBCSampler>
        <hashTree/>
      </hashTree>
    </hashTree>
  </hashTree>
</jmeterTestPlan>
XML

    _write_file($file, $xml);
}

sub _find_gnu_time {
    for my $cmd ('/usr/bin/time', '/bin/time') {
        my ($out, $err);
        next unless -x $cmd;
        return $cmd if run([$cmd, '-v', '-o', '/dev/null', 'true'], '>', \$out, '2>', \$err);
    }
    return undef;
}

sub _parse_resource {
    my ($file) = @_;
    return { available => 0, reason => 'GNU time output missing' } unless -f $file;
    my $text = perf_ext_slurp($file);
    my %r = (available => 1);

    unlink $file;
    for my $line (split /\n/, $text) {
        $r{user_cpu_sec} = 0 + $1 if $line =~ /User time \(seconds\):\s*([\d.]+)/;
        $r{sys_cpu_sec} = 0 + $1 if $line =~ /System time \(seconds\):\s*([\d.]+)/;
        $r{cpu_percent} = 0 + $1 if $line =~ /Percent of CPU this job got:\s*([\d.]+)%/;
        $r{max_rss_kb} = 0 + $1 if $line =~ /Maximum resident set size \(kbytes\):\s*(\d+)/;
    }
    $r{cpu_total_sec} = ($r{user_cpu_sec} // 0) + ($r{sys_cpu_sec} // 0);
    return \%r;
}

sub _parse_jtl_csv {
    my ($file) = @_;
    open(my $fh, '<', $file) or return { samples => 0, errors => 1, avg_ms => 0, throughput => 0 };
    my $header = <$fh>;
    my @h = split /,/, ($header // '');
    my %idx;

    chomp @h;
    for my $i (0 .. $#h) {
        $idx{$h[$i]} = $i;
    }

    my ($count, $errors, $sum, $min, $max) = (0, 0, 0, undef, 0);
    my $first_error;
    my ($first_ts, $last_ts);
    while (my $line = <$fh>) {
        chomp $line;
        next unless length $line;
        my @f = split /,/, $line, -1;
        my $elapsed = 0 + ($f[$idx{elapsed}] // 0);
        my $success = $f[$idx{success}] // 'false';
        my $ts = 0 + ($f[$idx{timeStamp}] // 0);

        $count++;
        if ($success ne 'true') {
            $errors++;
            if (!defined $first_error) {
                my $code = $f[$idx{responseCode}] // '';
                my $message = $f[$idx{responseMessage}] // '';
                $first_error = "$code $message";
            }
        }
        $sum += $elapsed;
        $min = $elapsed if !defined $min || $elapsed < $min;
        $max = $elapsed if $elapsed > $max;
        $first_ts = $ts if !defined $first_ts || $ts < $first_ts;
        $last_ts = $ts + $elapsed if !defined $last_ts || $ts + $elapsed > $last_ts;
    }
    close $fh;

    my $duration_sec = ($first_ts && $last_ts && $last_ts > $first_ts)
        ? (($last_ts - $first_ts) / 1000.0) : 0;
    return {
        samples => $count,
        errors => $errors,
        avg_ms => $count ? int($sum / $count + 0.5) : 0,
        min_ms => defined $min ? $min : 0,
        max_ms => $max,
        throughput => $duration_sec > 0 ? sprintf('%.2f', $count / $duration_sec) : 0,
        first_error => $first_error,
    };
}

sub perf_ext_run_jmeter {
    my ($jmeter, $jmx, $jtl, $log, $resource_file) = @_;
    my @cmd = ($jmeter, '-n', '-t', $jmx, '-l', $jtl, '-j', $log,
               '-Jjmeter.save.saveservice.output_format=csv',
               '-Jjmeter.save.saveservice.print_field_names=true',
               '-Jjmeter.save.saveservice.timestamp_format=ms');
    my $jar = $ENV{PERF_EXT_JDBC_JAR};
    my $time = _find_gnu_time();

    push @cmd, "-Juser.classpath=$jar" if $jar;
    if ($time) {
        unshift @cmd, ($time, '-v', '-o', $resource_file);
    }

    my ($stdout, $stderr);
    my $t0 = [gettimeofday()];
    my $ok = run(\@cmd, '>', \$stdout, '2>', \$stderr);
    my $wall_ms = int(tv_interval($t0) * 1000 + 0.5);
    my $summary = _parse_jtl_csv($jtl);
    my $resource = $time ? _parse_resource($resource_file)
                         : { available => 0, reason => 'GNU time not found' };

    $summary->{ok} = $ok ? 1 : 0;
    $summary->{wall_ms} = $wall_ms;
    $summary->{stdout} = $stdout;
    $summary->{stderr} = $stderr;
    $summary->{resource} = $resource;
    return $summary;
}

sub perf_ext_save_reports {
    my ($module_dir, $results) = @_;
    my $dir = "$module_dir/" . _project() . "/results";
    my $json_file = "$dir/perf_ext_results.json";
    my $csv_file = "$dir/perf_ext_results.csv";

    mkdir $dir unless -d $dir;
    _write_file($json_file, encode_json($results));

    open(my $csv, '>', $csv_file) or die "Cannot write $csv_file: $!";
    print $csv "name,kind,query_type,table,rows,concurrency,return_rows,total_samples,psql_timing_ms,jmeter_avg_ms,throughput,errors,wall_ms,server_explain_ms,server_explain_note,cpu_percent,user_cpu_sec,sys_cpu_sec,max_rss_kb\n";
    for my $r (@$results) {
        my $res = $r->{resource} // {};
        print $csv join(',',
            $r->{name},
            $r->{kind},
            $r->{query_type} // '',
            $r->{table},
            $r->{rows},
            $r->{concurrency},
            $r->{return_rows} // '',
            $r->{total_samples} // '',
            $r->{psql_timing_ms} // '',
            $r->{client_avg_ms} // 0,
            $r->{throughput} // 0,
            $r->{errors} // 0,
            $r->{wall_ms} // '',
            $r->{server_explain_ms} // '',
            $r->{server_explain_note} // '',
            $res->{cpu_percent} // '',
            $res->{user_cpu_sec} // '',
            $res->{sys_cpu_sec} // '',
            $res->{max_rss_kb} // '',
        ), "\n";
    }
    close $csv;
    return ($json_file, $csv_file);
}

1;
