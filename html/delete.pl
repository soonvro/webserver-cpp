#!/usr/bin/perl 
use CGI;
use Cwd;
use HTML::Entities;

$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;

$filename = $query->param("filename");
$filename =~ s/.*[\/\\](.*)/$1/;
$filename = decode_entities($filename);

if ( $filename && -e "$upload_dir/$filename" )
{
unlink "$upload_dir/$filename";

print $query->header ( );
print <<END_HTML;

<HTML>
<HEAD>
<TITLE>Deleted File : $filename</TITLE>
<meta charset="UTF-8">
<style>
a { margin-top: 15px;text-decoration: none;
color: white;border: 1px solid #276150;
background-color: #397a86;padding: 10px 15px; border-radius: 5px;}
</style>
</HEAD>

<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<h1>Deleted File : <span style="color: cornflowerblue;">$filename</span></h1>
<P>I deleted the file!</P>
</div>

<a href="/index.pl">Back to Home</a>
</BODY>
</HTML>

END_HTML
}
else
{
if ( !$filename )
{
  $filename = "(NULL)";
}
print $query->header ( );
print <<END_HTML;

<HTML>
<HEAD>
<TITLE>File Not Found!</TITLE>
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
<h1><span style="color: cornflowerblue;">does not exist \"$filename\"</span></h1>
<P>$filname does not exist..</P>
</div>
</BODY>
</HTML>

END_HTML
}