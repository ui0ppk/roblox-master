#!/usr/bin/perl
use MIME::Base64;

# str2c - simple string to c style data array - input string should be in quotes if it has spaces in it
# martin robaszewski 2013.07.17 [july 17, 2013]

$stringIn = $ARGV[0];

if ($#ARGV ne 0) {
	printf(STDERR "Usage: $0 string\n");
    exit(1);
}

my $feature_bitflip = 1; #bit flipping
my $feature_base64 = 1;

print "[STARTING]\n";
print "[INPUT]>" . $stringIn . "<\n";

if ($feature_base64 > 0)
{
	my $encoded = encode_base64($stringIn, "");
	print "[FEATURE BASE64]>" . $encoded . "<\n";
	$stringIn = $encoded;
}

if ($feature_bitflip > 0)
{
	print "[FEATURE BITFLIP]\n";
}

my $stringOut = "{ ";

$size = length($stringIn);
print "[size] = " . $size . "\n";

for (my $key = 0; $key < $size; $key++) 
{
	#print "[step " . $key . "]";
	my $c = substr($stringIn, $key, 1)."\n";
	my $ordinal = ord($c);
	if ($feature_bitflip > 0)
	{
		my $preord = $ordinal;
		$ordinal = (~$ordinal) & 255;
		#print "PRE: " . $preord . " POST: " . $ordinal . "\n";
	}
	$stringOut .= sprintf("0x%02x,", $ordinal);
}

# append terminating zero
$stringOut .= "0x00 }; //\"" . $ARGV[0] . "\"";

if ($feature_bitflip > 0)
{
	$stringOut .= " BITFLIPPED"
}

if ($feature_base64 > 0)
{
	$stringOut .= " BASE64"
}

$size++;

print "[OUTPUT]>unsigned char buffer[" . $size . "] = " . $stringOut . "\n";
print "[DONE]\n"

##EOF##