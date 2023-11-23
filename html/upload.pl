#!/usr/bin/perl -w
use CGI;
use Cwd;
use HTML::Entities;

$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;

$filename = $query->param("filename") // "";
$filename =~ s/.*[\/\\](.*)/$1/;
$filename = decode_entities($filename);

if ( $filename && !(-e "$upload_dir/$filename") && (-s "$upload_dir/$filename" lt 104857600) )
{
open UPLOADFILE, ">$upload_dir/$filename";

$upload_filehandle = $query->upload("filename");
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
<meta charset="UTF-8">
<style>
a { text-decoration: none;color: dodgerblue;padding: 5px;
border: 2px dotted lightgray;border-radius: 5px; }
a#bh { margin-top: 15px;color: white;border: 1px solid #276150;
background-color: #397a86;padding: 10px 15px; border-radius: 5px;}
</style>
</HEAD>

<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<h1>Saved File : <span style="color: cornflowerblue;">$filename</span></h1>
<P>Thanks for uploading your photo!</P>
<div style="padding: 10px;border: 2px solid lightblue;border-radius: 10px;">
<a href="/file_info.pl?filename=$filename">Check to $filename</a>
</div>
<a id="bh" href="/index.pl">Back to Home</a>
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
elsif ( !$filename || !(-e "$upload_dir/$filename") )
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
<meta charset="UTF-8">
<style>
a {margin-top: 15px;text-decoration: none;
color: black;padding: 10px 15px;border-radius: 5px;
align-self: flex-start;font-weight: 600;}
a:hover {color: skyblue;}
</style>
</HEAD>
<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<a href="/index.pl">< Back to Home</a>
<h1><span style="color: cornflowerblue;">Oops ! Something was wrong.</span></h1>
<P>$message</P>
</BODY>
</HTML>

END_HTML
}