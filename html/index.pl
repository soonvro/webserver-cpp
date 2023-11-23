#!/usr/bin/perl -w
use CGI;

$query = new CGI;

$username = $ENV { 'username' };
print $query->header ( );
print <<END_HTML;
<!DOCTYPE html>
<html>
<head>
<title>Welcome to Webserv!</title>
<style>
html { color-scheme: light dark; }
body { width: 35em; margin: 0 auto;
font-family: Tahoma, Verdana, Arial, sans-serif;
display: flex; flex-direction: column;
justify-content: center; align-items: center; }
main { padding: 10px 5px; border: 2px solid yellowgreen; border-radius: 15px; width: 380px; }
.item { padding: 10px 80px; border-radius: 15px; }
.item:hover { background-color: beige; transform: scale(1.02); color: darkcyan; }
a { text-decoration: none; color: inherit; }
#set-name { margin-left: 10px; padding: 5px 10px; border-radius: 10px; background-color: cornflowerblue; }
#set-name:hover { background-color: #556e9b; }
#message { margin-top: 15px; color: gray; }
</style>
</head>
<body>
<h1>Welcome to Webserv!</h1>
END_HTML
if ($username)
{
print "<p>hi ! $username<a id=\"set-name\" href=\"/login.html\">set name</a></p>";
}
else
{
print "<p>who are you ?<a id=\"set-name\" href=\"/login.html\">set name</a></p>";
}
print <<END_HTML;
<p>What to do ?</p>
<main>
<div class="item">
<a href="/upload_pl.html">
<small>file upload is availuable</small>
<div>File upload Using Perl</div>
</a>
</div>

<div class="item">
<a href="/delete_pl.html">
<small>file delete is availuable</small>
<div>Using Perl</div>
</a>
</div>

<div class="item">
<a href="/uploaded_list.pl">
<small>uploaded files list</small>
<div>file uploaded list</div>
</a>
</div>

<div class="item">
<a href="/cur_time.py">
<small>check current time</small>
<div>current time</div>
</a>
</div>
</main>
<div id="message">Thank you for using Webserv.</div>
</body>
</html>
END_HTML