#!/usr/bin/perl
use strict;
my @data;
my $line;
my @token;
my $portName;

system "ovs-vsctl show > out";

open(INPUT, "out") || die "cannot open the file";
@data = <INPUT>;
close INPUT;

foreach $line(@data){
	chomp $line;

	@token = split(' ', $line);
	if(@token[0] eq "Port"){
		if(index(@token[1], "_l") != -1){
			$portName = substr @token[1],1,(scalar($line) - 1);
			print "".$portName."\n";
			system "ovs-vsctl del-port br0 $portName";
		}
	}
}

system "docker ps > out";

open(INPUT, "out") || die "cannot open the file";
@data = <INPUT>;
close INPUT;

foreach $line(@data){
	chomp $line;

	@token = split(' ', $line);
	if(@token[0] ne "CONTAINER"){
		print "".@token[0]."\n";
		system "docker stop ".@token[0];
	}
}

