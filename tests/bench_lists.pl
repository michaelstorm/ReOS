#!/usr/bin/perl
`valgrind --tool=callgrind --callgrind-out-file=$ARGV[2] --dump-line=yes ./bin/bench_lists $ARGV[0] $ARGV[1]`;
