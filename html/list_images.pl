#!/usr/bin/perl -w
use CGI;
use Cwd;

$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;

$upload_filehandle = $query->upload("filename");

print $query->header ( );
print <<END_HTML;
<html>
<head>
<title>Uploaded Files List</title>
<style>
html { color-scheme: light dark; }
body { width: 35em; margin: 0 auto;
font-family: Tahoma, Verdana, Arial, sans-serif;
display: flex; flex-direction: column;
justify-content: center; align-items: center; }
ul { padding: 30px; border: 2px solid #8a2be242;
list-style: none; border-radius: 15px; }
li { padding: 5px 0; }
a { text-decoration: none; outline: none; color: cornflowerblue; }
</style>
</head>
<body>
<h1>Uploaded Files List</h1>
<ul>
END_HTML
while (<$upload_dir/*>)
{
my $filename = $_;
$filename =~ s/.*[\/\\](.*)/$1/;
print "<li><a href='/upload_files/$filename'>$filename</a></li>"
}
print <<END_HTML;
</ul>
</body>
</html>
END_HTML