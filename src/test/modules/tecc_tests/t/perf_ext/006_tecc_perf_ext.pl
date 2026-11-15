#!/usr/bin/perl

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

use FindBin;
use lib "$FindBin::Bin/..";
use lib "$FindBin::Bin/../..";
use tecc_perf_ext_utils;
use tecc_jdbc_jar;

my $module_dir = "$FindBin::Bin/../..";
my $tap = Test::More->builder->output;

sub _progress {
    print STDERR $_[0];
    print $tap $_[0];
}

{
    package TeccPerfExtExternalNode;

    sub connstr {
        my ($self, $dbname) = @_;
        return $self->{connstr};
    }

    sub installed_command {
        my ($self, $command) = @_;
        return $self->{psql} if $command eq 'psql';
        return $self->{ktool} if $command eq 'tecc_ktool';
        return $command;
    }

    sub port {
        my ($self) = @_;
        return $self->{port};
    }
}

my $jmeter = perf_ext_find_jmeter();
if (!$jmeter) {
    plan skip_all => 'JMeter not found; set PERF_EXT_JMETER_BIN to run perf_ext';
}

my $all_scenarios = perf_ext_load_scenarios($module_dir);
my @scenarios = perf_ext_filter_scenarios($all_scenarios);
if (!@scenarios) {
    plan skip_all => 'No perf_ext scenario matched TECC_TEST';
}

plan tests => scalar @scenarios;

my $node;
my $external_connstr = $ENV{TECC_PERF_EXT_EXTERNAL_CONNSTR};
my $skip_init = $ENV{PERF_EXT_SKIP_INIT} // 0;
if ($skip_init && !$external_connstr) {
    die "PERF_EXT_SKIP_INIT requires TECC_PERF_EXT_EXTERNAL_CONNSTR to reuse an existing database\n";
}
if ($external_connstr) {
    $node = {
        connstr => $external_connstr,
        psql    => $ENV{TECC_PERF_EXT_EXTERNAL_PSQL} // 'psql',
        ktool   => $ENV{TECC_PERF_EXT_EXTERNAL_KTOOL} // 'tecc_ktool',
        port    => $ENV{TECC_PERF_EXT_EXTERNAL_PORT} // ($ENV{PERF_EXT_DB_PORT} // 5432),
    };
    bless $node, 'TeccPerfExtExternalNode';
} else {
    $node = get_new_node('tecc_perf_ext');
    $node->init;
    $node->append_conf('postgresql.conf', "listen_addresses = '127.0.0.1'\n");
    $node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32 trust\n");
    $node->start;
    $node->psql('postgres', 'CREATE ROLE postgres LOGIN SUPERUSER;');
}

my $key_dir = $ENV{TECC_KEK_PATH} // "$TestLib::tmp_check/perf_ext_keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;

my $ktool = $node->installed_command('tecc_ktool');
my $sym_key = "$key_dir/key_perf_ext.key";

if (!-f $sym_key) {
    die "PERF_EXT_SKIP_INIT requires existing $sym_key\n" if $skip_init;
    perf_ext_run_checked([$ktool, 'generate', '--key-name', 'kek_perf_ext',
                          '--key-algo', 'sm4', '--key-path', $key_dir],
                         'tecc_ktool generate perf_ext symmetric key');
}

my $results_dir = "$module_dir/perf_ext/results";
mkdir $results_dir unless -d $results_dir;
if (!$ENV{PERF_EXT_KEEP_RESULTS} && !$skip_init) {
    unlink grep { /\.json$/ || /\.csv$/ || /\.jmx$/ || /\.jtl$/ || /\.log$/ || /\.txt$/ } glob "$results_dir/*";
}

my $host = $ENV{PERF_EXT_DB_HOST} // '127.0.0.1';
my $port = $ENV{PERF_EXT_DB_PORT} // $node->port;
my $dbname = $ENV{PERF_EXT_DB_NAME} // 'postgres';
my $user = $ENV{PERF_EXT_DB_USER} // 'postgres';
my $password = $ENV{PERF_EXT_DB_PASSWORD} // '';
my $jdbc_url = $ENV{PERF_EXT_JDBC_URL}
    // "jdbc:postgresql://$host:$port/$dbname?sslmode=disable&connectTimeout=60&socketTimeout=300&ApplicationName=tecc_perf_ext&enable_tecc=1";

# ReqID:TECC4.0 - 驱动 JAR 必须按版本选（build/libs 会累积历次构建产物），
# 选取规则见 t/tecc_jdbc_jar.pm —— 不能再用 glob 的字典序结果。
if (!$ENV{PERF_EXT_JDBC_JAR} && $ENV{TECC_JDBC_HOME}) {
    my @jars = jdbc_jar_candidates($ENV{TECC_JDBC_HOME});
    if (@jars) {
        $ENV{PERF_EXT_JDBC_JAR} = $jars[0];
        _progress("# perf_ext: jdbc_jar=" . jdbc_jar_selection_note(\@jars) . "\n");
    }
}
if (!$ENV{PERF_EXT_JDBC_JAR} || !-f $ENV{PERF_EXT_JDBC_JAR}) {
    die "PERF_EXT_JDBC_JAR must point to a readable postgresql JDBC jar for JMeter\n";
}

_progress "# perf_ext: jmeter=$jmeter\n";
_progress "# perf_ext: jdbc_url=$jdbc_url\n";
_progress "# perf_ext: scenarios=" . scalar(@scenarios) . "\n";

if ($skip_init) {
    _progress "# perf_ext: skip schema/data initialization\n";
} else {
    perf_ext_init_schema($node, $module_dir, $key_dir);
    perf_ext_init_data($node, $module_dir, \@scenarios);
}

my @results;
my $idx = 0;
for my $scenario (@scenarios) {
    $idx++;
    my $name = $scenario->{name};
    my $jmx = "$results_dir/$name.jmx";
    my $jtl = "$results_dir/$name.jtl";
    my $log = "$results_dir/$name.log";
    my $res_file = "$results_dir/$name.resource.txt";
    my $sql = perf_ext_build_sql($scenario, 0);

    _progress sprintf("# perf_ext [%d/%d] %s\n", $idx, scalar(@scenarios), $name);
    my $timing = perf_ext_psql_timing($node, $sql);
    my $explain = perf_ext_explain_time($node, $sql);
    perf_ext_write_jmx($scenario, $jmx, {
        url      => $jdbc_url,
        user     => $user,
        password => $password,
    });
    my $jm = perf_ext_run_jmeter($jmeter, $jmx, $jtl, $log, $res_file);
    my $has_samples = ($jm->{samples} // 0) > 0;
    my $ok = $jm->{ok} && $has_samples && ($jm->{errors} // 0) == 0
        && ($timing->{ok} // 0) && ($explain->{ok} // 0);

    push @results, {
        name              => $name,
        kind              => $scenario->{kind},
        query_type        => $scenario->{query_type},
        table             => $scenario->{table},
        rows              => $scenario->{rows},
        concurrency       => $scenario->{concurrency},
        return_rows       => $scenario->{return_rows},
        total_samples     => $scenario->{total_samples},
        psql_timing_ms    => $timing->{timing_ms},
        client_avg_ms     => $jm->{avg_ms},
        client_min_ms     => $jm->{min_ms},
        client_max_ms     => $jm->{max_ms},
        throughput        => $jm->{throughput},
        samples           => $jm->{samples},
        errors            => $jm->{errors},
        wall_ms           => $jm->{wall_ms},
        server_explain_ms => $explain->{execution_ms},
        server_explain_note => $explain->{note},
        resource          => $jm->{resource},
        jmx               => $jmx,
        jtl               => $jtl,
        log               => $log,
    };

    if ($ok) {
        pass(sprintf("%s psql_timing=%sms jmeter_avg=%sms throughput=%s/s server_explain=%sms",
             $name, $timing->{timing_ms} // 'N/A', $jm->{avg_ms},
             $jm->{throughput}, $explain->{execution_ms} // 'N/A'));
    } else {
        fail(sprintf("%s samples=%s errors=%s timing_ok=%s jmeter_ok=%s explain_ok=%s",
             $name, $jm->{samples} // 0, $jm->{errors} // 0, $timing->{ok} // 0,
             $jm->{ok} // 0, $explain->{ok} // 0));
        if (!$has_samples) {
            print $tap "#   jmeter: no samples were written to $jtl; check $log for connection pool, TECC init, timeout, or sampler startup errors\n";
        }
        if (!$timing->{ok}) {
            my $err = $timing->{stderr} // $timing->{stdout} // '';
            $err =~ s/\n.*//s;
            print $tap "#   timing: $err\n" if $err;
        }
        if (!$jm->{ok}) {
            my $err = $jm->{stderr} // $jm->{stdout} // '';
            $err =~ s/\n.*//s;
            print $tap "#   jmeter: $err\n" if $err;
        }
        if (($jm->{errors} // 0) > 0 && $jm->{first_error}) {
            print $tap "#   jmeter first error: $jm->{first_error}\n";
        }
        if (!$explain->{ok}) {
            my $err = $explain->{error} // '';
            $err =~ s/\n.*//s;
            print $tap "#   explain: $err\n" if $err;
        }
    }

    if ($jm->{resource} && $jm->{resource}{available}) {
        print $tap sprintf("#   client resource: CPU%%=%s user=%.2fs sys=%.2fs RSS=%sKB\n",
            $jm->{resource}{cpu_percent} // 'N/A',
            $jm->{resource}{user_cpu_sec} // 0,
            $jm->{resource}{sys_cpu_sec} // 0,
            $jm->{resource}{max_rss_kb} // 'N/A');
    }
}

my ($json_file, $csv_file) = perf_ext_save_reports($module_dir, \@results);
print $tap "# perf_ext JSON: $json_file\n";
print $tap "# perf_ext CSV: $csv_file\n";
