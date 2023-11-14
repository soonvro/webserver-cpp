#!/usr/bin/perl -w
use CGI;
use Cwd;
use HTML::Entities;

$current_dir = cwd;
$upload_dir = "$current_dir/upload_files";

$query = new CGI;

print $query->header ( );
print <<END_HTML;
<html>
<head>
<title>Uploaded Files List</title>
<meta charset="UTF-8">
<style>
html { color-scheme: light dark; }
body { width: 35em; margin: 0 auto;
font-family: Tahoma, Verdana, Arial, sans-serif;
display: flex; flex-direction: column;
justify-content: center; align-items: center; }
ul { padding: 30px; border: 2px solid #8a2be242;
list-style: none; border-radius: 15px; width: 200px; }
li { padding: 5px 0; display: flex; justify-content: center;
transition: .3s ease-in-out; font-size: 20px; }
li:hover { background-color: #ff00d026; transform: scale(1.2); }
a { text-decoration: none; outline: none; color: cornflowerblue; }
a#back {margin-top: 15px;text-decoration: none;
color: black;padding: 10px 15px;border-radius: 5px;
align-self: flex-start;font-weight: 600;}
a#back:hover {color: skyblue;}
</style>
</head>
<body>
<a id="back" href="/index.html">< Back to Home</a>
<h1>Uploaded Files List</h1>
<ul>
END_HTML
while (<$upload_dir/*>)
{
my $filename = $_;
$filename =~ s/.*[\/\\](.*)/$1/;
$filename = decode_entities($filename);
print "<li><a href='/file_info.pl?filename=$filename'>$filename</a></li>"
}
print <<END_HTML;
</ul>
</body>
</html>
END_HTML