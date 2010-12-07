#!/usr/bin/perl
# abcdefghijklmnopqrstuvwxyz
$out = $ARGV[1];
@args = splice(@ARGV, 2);
$cmd = "../bin/ascii @args -f '((?=(?!b)a)a){1,$ARGV[0]}(a){1,$ARGV[0]}(\\1\\2)?' a_file";
print "$cmd\n";
`valgrind --tool=callgrind --callgrind-out-file=$out --dump-line=yes $cmd`;
