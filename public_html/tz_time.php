<?php

// send text plain header
header('Content-Type: text/plain');


// https://www.virendrachandak.com/techtalk/php-isset-vs-empty-vs-is_null/
if (isset($_REQUEST['debug']))
{
	
	ini_set('display_errors', 1);
	ini_set('display_startup_errors', 1);
	error_reporting(E_ALL);
}

	
// Provide the time

/*
	$date = new DateTime("now", new DateTimeZone('Australia/Brisbane') );
	echo $date->format('Y-m-d H:i:s');
 */

$tz_obj = null;
if ( !empty($_REQUEST['timezone']) )
{
	try 
	{
		$tz_obj = new DateTimeZone($_REQUEST['timezone']);
		
		// We won't get here if an exception occurs					
	}
	catch (Exception $e) {
		echo 'ERROR:Caught exception: ',  $e->getMessage(), "\n";

	}
}

$datetime = new DateTime("now", $tz_obj );

echo "TIME:". $datetime->format('Y-m-d H:i:s') ."\n";
echo "TIMESTAMP:". $datetime->getTimestamp() ."\n"; // this is ALWAYS UTC per: http://php.net/manual/en/datetime.gettimestamp.php
echo "TIMESTAMP_OFFSET:". $datetime->getOffset() ."\n";

