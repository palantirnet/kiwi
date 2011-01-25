<?php

// From the emu test script.

require_once 'emu/imu.php';
require_once IMu::$lib . '/module.php';
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/trace.php';

// Because PHP yells if you don't. --LG
date_default_timezone_set('America/Chicago');

// We want to be able to twiddle with this value for optimum performance without
// worrying about the server killing a process. --LG
$config['num_children'] = 3;

$now = time();
$then = $now + 5;

// In this setup, we fork the children off first.  We could potentially only
// fork the children off after the initial parent process's request is completed,
// but that means the queue will populate fully before any processing happens. --LG

for ($i = 0; $i < $config['num_children']; ++$i) {
  $pid = pcntl_fork();
  if ($pid == -1) {
    die('could not fork');
  } else if ($pid) {
    // we are the parent
    //pcntl_wait($status); //Protect against Zombie children

  } else {
    // we are the child
    print "Creating child #{$i}" . PHP_EOL;
    emu_worker($i, $then);
    exit();
  }
}

// Do parent stuff here. -_LG
emu_big_query();
print "Parent ending..." . PHP_EOL;


/**
 * This is the parent process.
 * 
 * Normally we'd do more processing with the data we get back and then stick
 * it into a queue, but this is just an example.  It's also possibly a rather
 * inefficient way of working with Emu.  Feedback welcome.
 * 
 * This is ripped mostly from the emu example script.
 */
function emu_big_query() {

  /* We start with a new session
  */
  $session = new IMuSession;
  $session->host = "66.158.71.71";
  $session->port = "40000";
  $session->connect();

  $response = $session->getModules();
  $tables = array();
  if ($response['status'] == "ok") {
    $tables = $response['tables'];
  }
  $response = $session->getTableSchema('ecatalogue');
  if ($response['status'] == "ok") {
    $schema = $response['schema'];
  }

  $emutable = 'ecatalogue';
  $module = new IMuModule($emutable, $session);
  $terms = array();
  $columnname = "CatDepartment";
  $queryvalue = "Zoology";
  $terms[] = array($columnname, $queryvalue);
  $columnname = "PrePrepType_tab";
  $queryvalue = "alcohol";
  $terms[] = array($columnname, $queryvalue);
  $module->findTerms(array('and', $terms));

  // The above is the "query" part.  Below is the "retrieve" part. Whether the
  // license limit applies to just the stuff above or to both of these parts
  // is critically important. --LG

  // I've modified the following to fetch not 2 records but a ton of records,
  // since in practice we would be spinning through potentially hundreds of
  // thousands of records here. --LG

  $columns = array();
  $columns[] = 'SummaryData';
  $columns[] = 'IdeTaxonRef_tab->(SummaryData, ClaGenus)';
  $columns[] = '
          darwincorefields =
          [
                  InstitutionCode = DarInstitutionCode,
                  Collection = DarCollectionCode,
                  Name = DarScientificName
          ]';

  $position = 'current';
  $count = 1;
  $limit = 200;
  for ($i = 0; $i < $limit; ++$i) {
    $result = $module->fetch($position, $i, $count, $columns);

    // Do some processing to $result. --LG

    // Add $result to the beanstalkd queue. --LG
  }
}

/**
 * This simulates what we want the worker processes to be able to do.
 *
 * Naturally this would all be tied into a beanstalkd queue in practice but you
 * get the idea.
 *
 * This is ripped from the emu example script.
 */
function emu_worker($id, $then) {

  print "Child #{$id} launching." . PHP_EOL;
  time_sleep_until($then);

  print "Child #{$id} waking up." . PHP_EOL;

  /* We start with a new session
  */
  $session = new IMuSession;
  $session->host = "66.158.71.71";
  $session->port = "40000";
  $session->connect();

  $response = $session->getModules();
  $tables = array();
  if ($response['status'] == "ok") {
    $tables = $response['tables'];
  }
  $response = $session->getTableSchema('ecatalogue');
  if ($response['status'] == "ok") {
    $schema = $response['schema'];
  }

  // Each worker would, in practice, be waiting for a queue item and for each one
  // issue one or more queries to do further processing.  To simulate that, I've
  // just wrapped the emu test code in a loop to churn for a while. --LG

  for ($i = 0; $i < 30; ++$i) {

    $emutable = 'ecatalogue';
    $module = new IMuModule($emutable, $session);
    $terms = array();
    $columnname = "CatDepartment";
    $queryvalue = "Zoology";
    $terms[] = array($columnname, $queryvalue);
    $columnname = "PrePrepType_tab";
    $queryvalue = "alcohol";
    $terms[] = array($columnname, $queryvalue);
    $module->findTerms(array('and', $terms));

    $columns = array();
    $columns[] = 'SummaryData';
    $columns[] = 'IdeTaxonRef_tab->(SummaryData, ClaGenus)';
    $columns[] = '
            darwincorefields =
            [
                    InstitutionCode = DarInstitutionCode,
                    Collection = DarCollectionCode,
                    Name = DarScientificName
            ]';

    $position = "start"; /* or "current" or "end" */
    $offset = 0;
    $count = 2;
    $result = $module->fetch($position, $offset, $count, $columns);

    // Do stuff with $result, including run more queries. --LG

    // Build a Solr document object. --LG

    // Send the Solr document to Solr for indexing. --LG

  }
}
