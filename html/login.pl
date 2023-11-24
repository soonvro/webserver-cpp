use CGI;
use HTML::Entities;

$query = new CGI;
$hostname = $ENV { 'SERVER_NAME' };

$username = $query->param("username");
$username = decode_entities($username);

print "user-name: $username\n";
print "Location: http://$hostname/index.pl\n\n";