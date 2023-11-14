#!/usr/bin/perl -w
use CGI;
use Cwd;
use HTML::Entities;


$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;

$filename = CGI::param("filename") // "";
$filename =~ s/.*[\/\\](.*)/$1/;
$upload_filehandle = $query->upload("filename");

sub is_image_file
{
my $filename = shift;
my %image_extensions = map { $_ => 1 } qw(apng jpg jpeg png gif webp);

my ($extension) = $filename =~ /\.([^.]+)$/;
return $extension && $image_extensions{lc($extension)};
}

if ( $filename && !(-e "$upload_dir/$filename") && (-s "$upload_dir/$filename" lt 104857600) )
{
my $decode_filename = decode_entities($filename);
open UPLOADFILE, ">$upload_dir/$decode_filename";

while ( <$upload_filehandle> )
{
  print UPLOADFILE;
}

close UPLOADFILE;

print $query->header ( );
print <<END_HTML;

<HTML>
<HEAD>
<TITLE>Saved File : $filename</TITLE>
<style>
a { margin-top: 15px;text-decoration: none;
color: white;border: 1px solid #276150;
background-color: #397a86;padding: 10px 15px; border-radius: 5px;}
</style>
</HEAD>

<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<h1>Saved File : <span style="color: cornflowerblue;">$filename</span></h1>
<P>Thanks for uploading your photo!</P>
<div style="padding: 10px;border: 2px solid lightblue;border-radius: 10px;">
END_HTML
if (is_image_file($filename))
{
print "<img src=\"/upload_files/$filename\" style=\"width: 900px;\" />";
}
else
{
print "<div style=\"color: darkgray;font-weight: 600;\">[ not image file ]</div>";
}
print <<END_HTML;
</div>

<a href="/index.html">Back to Home</a>
</BODY>
</HTML>

END_HTML
}
else
{
my $message;
if ( $filename )
{
  $message = "\"$filename\" already exists.";
}
elsif ( !(-e "$upload_dir/$filename") )
{
  $message = "NULL files cannot be saved.";
}
else
{
  $message = "File too long ..";
}

print $query->header ( );
print <<END_HTML;
<HTML>
<HEAD>
<TITLE>Failed Upload</TITLE>
<style>
a {margin-top: 15px;text-decoration: none;
color: black;padding: 10px 15px;border-radius: 5px;
align-self: flex-start;font-weight: 600;}
a:hover {color: skyblue;}
</style>
</HEAD>
<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<a href="/index.html">< Back to Home</a>
<h1><span style="color: cornflowerblue;">Oops ! Something was wrong.</span></h1>
<P>$message</P>
</BODY>
</HTML>

END_HTML
}