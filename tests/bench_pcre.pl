#!/usr/bin/perl
$out = $ARGV[0];
@args = splice(@ARGV, 1);
$cmd = "./bin/pcredemo @args";
print "$cmd\n";
`valgrind --tool=callgrind --callgrind-out-file=$out --dump-line=yes $cmd`;
