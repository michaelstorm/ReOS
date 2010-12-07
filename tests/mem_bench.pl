#!/usr/bin/perl
$out = $ARGV[1];
@args = splice(@ARGV, 2);
$cmd = "../bin/ascii @args -f '((?=(?!b)a)a){1,$ARGV[0]}(a){1,$ARGV[0]}(\\1\\2)?' a_file";
print "$cmd\n";
`valgrind --tool=massif --massif-out-file=$out $cmd`;
