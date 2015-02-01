use strict;
use Google::ProtocolBuffers;
use Data::Dumper;

Google::ProtocolBuffers->parsefile("../facts.proto");

foreach my $arg (@ARGV) {
  open(my $fh, "< $arg") || die "can't open $arg";

  while ((my $bytes = ReadVarint32($fh)) != 0) {
    my $data;
    my $n = read $fh, $data, $bytes;
    my $doc = NFactex::TDocument->decode($data);
    print Dumper($doc);
  }
  close(F);
}

sub ReadVarint32 {
  my ($fh) = @_;
  my ($r, $b, $offset) = (0, 0, 0);

  while ((my $n = read $fh, $b, 1) != 0) {
    $b = ord $b;

    if (0 == $offset) {
      $r = $b & 0x7F;
    } else {
      $r = $r | ( ($b & 0x7F) << $offset );
    }

    if (!($b & 0x80)) { last; }

    $offset += 7;
  }

  return $r;
}

