<?php
/* First we'll import the base API library
*/
require_once '../emu/imu.php';
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
/* We're using a different port. This is a 're-connect'
** port.
*/
$session->port = "45000";
$session->connect();

/* We'll use a Module object to do most of the work
*/
$emutable = 'ecatalogue';
$module = new IMuModule($emutable, $session);
/* We are going to specify the 'id' of this module to 
** reconnect to the module previously used.
*/
//$module->id = 'some_id_of_some_sort';
$module->id = 'b76d';

/* Now we'll fork off a couple of children who will each
** wait until the same moment to get 5 records each
*/
$now = time();
$then = $now + 5;
for ($numchild = 1; $numchild < 2; $numchild++)
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
		$then = $then + ($numchild * 0.5);
		print "$numchild sleeping...\n";
		flush();
		time_sleep_until($then);

		print "$numchild requesting\n";
		flush();
		/* We'll setup the columns we want returned.
		*/
		$columns = array();
		/* A basic column
		*/
		$columns[] = 'SummaryData';

		/* We'll now fetch those columns
		*/
		$position = "current"; /* or "current" or "end" */
		$offset = 0;
		$count = 1;
		$fp = fopen("$numchild.out", "a");
		for ($i = 0; $i < 200; $i++)
		{
			try
			{
			$result = $module->fetch($position, $offset, $count, $columns);
			}
			catch (Exception $e)
			{
			}
			$rows = $result->rows;
			foreach($rows as $row)
			{
				$rownumber = $row['rownum'];
				fwrite($fp, "$rownumber\n");
			}
			usleep(5000);
		}
		fclose($fp);
		exit(0);
	}
}
// we are the parent
pcntl_wait($status); //Protect against Zombie children
exit(0);
