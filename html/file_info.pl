#!/usr/bin/perl -w
use CGI;
use Cwd;
use HTML::Entities;

$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;
$filename = $query->param("filename");
$filename = decode_entities($filename);

sub is_type
{
my $filename = shift;
my ($file_extension) = $filename =~ /\.([^.]+)$/;
my %extensions = map { $_ => $_ eq "txt"? 1 : 2 } qw(txt apng jpg jpeg png gif webp);

return $file_extension && $extensions{lc($file_extension)};
}

print $query->header ( );
print <<END_HTML;

<HTML>
<HEAD>
<TITLE>$filename</TITLE>
<meta charset="UTF-8">
<style>
a { margin-top: 15px;text-decoration: none;
color: white;border: 1px solid #276150;
background-color: #397a86;padding: 10px 15px; border-radius: 5px;}
</style>
</HEAD>

<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<h1><span style="color: cornflowerblue;">$filename</span></h1>
END_HTML
my $file_type = is_type($filename);
if ($file_type == 1)
{
print "<p>[ TXT File ]</p>";
print "<div style=\"padding: 10px;border: 2px solid lightgray;border-radius: 10px;\">";
open my $fh, '<', "$upload_dir/$filename" or die "Cannot open file '$filename': $!";
my $data;
while (<$fh>) {
    $data .= $_;
}
close $fh;
print "<p style=\"width: 500px;white-space: pre-wrap;overflow: auto;\">$data</p>";
}
elsif ($file_type == 2)
{
print "<p>[ IMG File ]</p>";
print "<div style=\"padding: 10px;border: 2px solid lightblue;border-radius: 10px;\">";
print "<img style=\"width: 500px;\" src=\"/upload_files/$filename\" />";
}
else
{
print "<p>[ Unreadable File ]</p>";
print "<div style=\"padding: 10px;border: 2px solid tomato;border-radius: 10px;\">";
print "<div>Can not read file.</div>";
}
print <<END_HTML
</div>

<a href="/uploaded_list.pl">Back to list</a>
</BODY>
</HTML>

END_HTML