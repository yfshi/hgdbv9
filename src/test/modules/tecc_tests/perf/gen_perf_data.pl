#!/usr/bin/perl
# gen_perf_data.pl
# Generate plaintext CSV data for TECC performance tests.
# Output: perf_int.csv, perf_decimal.csv, perf_text.csv (each 1M rows)
use strict;
use warnings;

my $rows = 1000000;
my $dir = "/tmp";

# 1) perf_int.csv: integers 1..1000000 (one per row)
open(my $fh_int, '>', "$dir/perf_int.csv") or die "Cannot write perf_int.csv: $!";
for my $i (1 .. $rows) {
    print $fh_int "$i\n";
}
close $fh_int;
printf "Generated %s/perf_int.csv: %d rows\n", $dir, $rows;

# 2) perf_decimal.csv: seq * 0.000001
open(my $fh_dec, '>', "$dir/perf_decimal.csv") or die "Cannot write perf_decimal.csv: $!";
for my $i (1 .. $rows) {
    printf $fh_dec "%.12f\n", $i * 0.000001;
}
close $fh_dec;
printf "Generated %s/perf_decimal.csv: %d rows\n", $dir, $rows;

# 3) perf_text.csv: "这是第 X 条测试记录"
open(my $fh_txt, '>', "$dir/perf_text.csv") or die "Cannot write perf_text.csv: $!";
for my $i (1 .. $rows) {
    print $fh_txt "这是第 $i 条测试记录\n";
}
close $fh_txt;
printf "Generated %s/perf_text.csv: %d rows\n", $dir, $rows;

print "All CSV files generated successfully.\n";
