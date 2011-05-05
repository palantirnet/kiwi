<?php
/* First we'll import the base API library
*/
require_once '../../emu/imu.php';
/* Next we'll import the module library, as that will
** provide useful tools for querying and returning 
** results
*/
require_once IMu::$lib . '/module.php';
/* We don't really need these, but they can be useful
*/
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/trace.php';

IMuTrace::setFile('trace.txt');
IMuTrace::setLevel(1);

/* We start with a new session
*/
$session = new IMuSession;
$session->host = "66.158.71.71";
$session->port = "40000";
$session->connect();
$session->login('emu', 'emucorneliafmnh');

/* Now we'll fork off a couple of children who will each
** wait until the same moment to make a getModules request
*/
$now = time();
$then = $now + 5;
for ($numchild = 1; $numchild < 6; $numchild++)
{
	$pid = pcntl_fork();
	if ($pid == -1)
	{
	     die('could not fork');
	}
	else if ($pid)
	{
	}
	else
	{
		$then = $then + ($numchild * 0);
		print "$numchild sleeping...\n";
		flush();
		time_sleep_until($then);

		print "$numchild requesting\n";
		flush();
		$response = $session->getModules();

		print "$numchild : ";
		print_r($response['status']);
		print "\n";
		flush();
		exit(0);
	}
}
// we are the parent
pcntl_wait($status); //Protect against Zombie children
exit(0);

/* We'll grab a list of all the tables using the new
** feature of the API I created
*/
$response = $session->getModules();
$tables = array();
if ($response['status'] == "ok")
{
	$tables = $response['tables'];
}

/* We'll grab the schema for a particular table
*/
$response = $session->getTableSchema('ecatalogue');
if ($response['status'] == "ok")
{
	$schema = $response['schema'];
}

/* We'll use a Module object to do most of the work
*/
$emutable = 'ecatalogue';
$module = new IMuModule($emutable, $session);

/* We'll setup an array of query terms.
/* Each column:value pair is a two row array.
/* Each boolean operator is also a two row array. 
*/
$terms = array();
$columnname = "CatDepartment";
$queryvalue = "Zoology";
$terms[] = array($columnname, $queryvalue);
$columnname = "PrePrepType_tab";
$queryvalue = "alcohol";
$terms[] = array($columnname, $queryvalue);

/* The query is run by this command and results are stored server side
*/
$module->findTerms(array('and', $terms));

/* We'll setup the columns we want returned.
*/
$columns = array();
/* A basic column
*/
$columns[] = 'SummaryData';
/* A Ref_tab with fields from the referenced records
*/
$columns[] = 'IdeTaxonRef_tab->(SummaryData, ClaGenus)';
/* A named group of re-named columns
*/
$columns[] = '
	darwincorefields =
	[
		InstitutionCode = DarInstitutionCode,
		Collection = DarCollectionCode,
		Name = DarScientificName
	]';

/* We'll now fetch those columns
*/
$position = "start"; /* or "current" or "end" */
$offset = 0;
$count = 2;
/* This will get the first two records
*/
$result = $module->fetch($position, $offset, $count, $columns);

print "First Two:\n";
print_r($result);

/* This will get the next two records
*/
$position = "current";
$result = $module->fetch($position, $offset, $count, $columns);

print "Next Two:\n";
print_r($result);

/* Now we'll cycle through the results and do stuff with the data
*/
$count = count($result->rows);
$hits = $result->hits;
print "$count records of $hits.\n";
for ($i = 0; $i < $count; $i++)
{
	$record = $result->rows[$i];
	print_r($record);
}
?>
