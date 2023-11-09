#!/usr/bin/perl -w
use CGI;
use Cwd;

$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;

$filename = $query->param("filename");
$filename =~ s/.*[\/\\](.*)/$1/;
$upload_filehandle = $query->upload("filename");

open UPLOADFILE, ">$upload_dir/$filename";

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
</HEAD>

<BODY style="display: flex;flex-direction: column;justify-content: center;align-items: center;">
<h1>Saved File : <span style="color: cornflowerblue;">$filename</span></h1>
<P>Thanks for uploading your photo!</P>
<div style="padding: 10px;border: 2px solid lightblue;border-radius: 10px;">
<img src="/upload_files/$filename" style="border-radius: 10px;"/>
</div>
</BODY>
</HTML>

END_HTML