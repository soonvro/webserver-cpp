use CGI;
use HTML::Entities;

$query = new CGI;

$username = $query->param("username");
$username = decode_entities($username);

print "Location: /index.pl\n";
print "user-name: $username\n";
print "Location: /index.pl\n\n";