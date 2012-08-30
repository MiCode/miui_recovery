use strict;
use File::Copy;
use File::Basename;

my $res_dir = $ARGV[0];
my %defined_resources;
my $ret = 0;

my @dir_name_4_file_res = qw(drawable layout color raw anim interpolator animatorcd mipmap xml menu);
foreach my $dir(@dir_name_4_file_res) {
    get_file_resource($res_dir."/$dir*", \%defined_resources);
}

foreach my $value_dir (<$res_dir/values*>) {
    get_defined_values($value_dir, \%defined_resources);
}

my %public_resources = %{ parse_public($res_dir."/values/public.xml") };
foreach my $defined ( keys %defined_resources ){
    unless (exists $public_resources{$defined}) {
        my ($hint, $type) = @{ $defined_resources{$defined} };
        print stderr "$hint: error: resource($type) $defined should be declared as public in public.xml\n";
        $ret = 1;
    }
}

exit($ret);

#
# functions to get resources and public definitions
#
sub get_file_resource {
    my ($dir, $attrsHashRef) = @_;
    foreach my $file (<$dir/*>) {
        my ($res, $path, $suffix) = fileparse($file, '\.xml', '\.9\.png', '\.png', '\.jpg');
        $path =~ s/\/$//;
        my $type = basename($path);
        $type =~ s/-.*$//;
        if (exists $attrsHashRef->{$res}){
            #die "$res is already exist";
        }
        else {
            $attrsHashRef->{$res} = ["$file:0", $type];
        }
    } 
}

# the resources under values are defined as: <RES_TYPE ... name="RES_NAME"..
sub get_defined_values {
    my ($dir, $defined_values) = @_;

    foreach my $xmlfile (<$dir/*.xml>){
        next if $xmlfile =~ /public\.xml$/;

        open (VALUE_XML_FILE, $xmlfile) || die "Failed to open $xmlfile $!";
        my $line = 0;
        while(<VALUE_XML_FILE>){
            $line ++;
            if (/\<\s*([a-zA-Z\-]+)\s+.*name\s*=\s*\"([^\"]+)\"/){
                my ($type, $name) = ($1, $2);
                next if $type eq "enum";
                next if $type eq "declare-styleable";
                if ($type eq "item") {
                    if (/type\s*=\s*\"(.+)\"/){
                        $type = $1;
                    }
                    else {
                        next;
                    }
                }
                $defined_values->{$name} = ["$xmlfile:$line", $type];
             }
        }
        close(VALUE_XML_FILE);
    }
}

sub parse_public {
    my ($file) = @_;
    my %public;
    open (PUBLIC, $file);
    foreach(<PUBLIC>){
        if (/^\s*\<\s*public\s+type\s*=\s*\"(.+)\"\s+name\s*=\s*\"(.+)\"\s+id\s*=\s*\"(.+)\"\s*\/\s*\>/){
            my ($type, $name, $id) = ($1, $2, $3);
            $public{$name} = [$type, $id];
        }
    }
    return \%public;
}

