#!/usr/bin/perl

# 基于 perf/results/*.json 快速重新生成客户端资源报告。

use strict;
use warnings;
use FindBin;
use JSON::PP;

use lib "$FindBin::Bin/../t";
use tecc_perf_utils;

sub slurp_file {
    my ($file) = @_;
    my $fh;
    my $content;

    open($fh, '<', $file) or die "Cannot read $file: $!";
    local $/;
    $content = <$fh>;
    close $fh;
    return $content;
}

my $module_dir = "$FindBin::Bin/..";
my $results_dir = "$module_dir/perf/results";
my @results;

opendir(my $dh, $results_dir) or die "Cannot open $results_dir: $!";
for my $file (sort grep { /\.json$/ } readdir($dh)) {
    my $path = "$results_dir/$file";
    my $data = decode_json(slurp_file($path));
    my $name = $data->{test};

    $name = $file unless defined $name && length $name;
    $name =~ s/\.json$//;

    push @results, {
        name => $name,
        avg => $data->{avg_ms} // 0,
        resource => $data->{resource},
    };
}
closedir($dh);

die "No perf JSON files found in $results_dir\n" unless @results;

my $report = save_perf_resource_report($module_dir, \@results, {
    driver => 'libpq',
});

print "客户端资源报告：$report\n" if $report;
