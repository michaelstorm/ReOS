#!/usr/bin/perl
@args = splice(@ARGV, 1);
$cmd = "../bin/ascii @args -f -a '((?=(?!b)a)a){1,$ARGV[0]}(a){1,$ARGV[0]}(\\1\\2)?' a_file";
print "$cmd\n";
`valgrind --leak-check=full --show-reachable=yes --suppressions=known_leaks.supp $cmd`;
