#!/usr/bin/perl 
use CGI;

$query = new CGI;

$filename = $query->param("filename");
$upload_dir = "/Users/inskim/inskim/5circle/Webserv/upload_files";

unlink "$upload_dir/$filename";

print "Content-type:text/html\r\n\r\n";
print "<html>";
print "<head>";
print "<title>File Delete Service Using Perl</title>";
print "</head>";
print "<body>";
if (-e "$upload_dir/$filename"){
print "file still present means unlink fails '$upload_dir/$filename' $!<br/>";
}else{
print  "file Removed successfully by unlink '$upload_dir/$filename' $!<br/>";
  print "\n";
}
print "<br/><br/>";
print "<a href=\"http://127.0.0.1/index.html\">Index Page</a>.<br/>";
print "</body>";
print "</html>";
1;