use CGI;
use HTML::Entities;

$query = new CGI;
$hostname = $ENV { 'SERVER_NAME' };

$sessionId = $ENV { 'session_id' };

print "logout: $sessionId\n";
print "Location: http://$hostname/index.pl\n\n";