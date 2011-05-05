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

phase2(phase1());

function phase1() {
  /* We start with a new session
  */
  $session = new IMuSession;
  $session->host = "66.158.71.71";
  $session->port = "40000";
  $session->connect();
  $session->login('emu', 'emucorneliafmnh');
  $session->suspend = true;

  /* We'll use a Module object to do most of the work
  */
  $emutable = 'ecatalogue';
  $module = new IMuModule($emutable, $session);
  $module->destroy = false;

  /* We'll setup an array of query terms.
  /* Each column:value pair is a two row array.
  /* Each boolean operator is also a two row array.
  */
  $terms = array();
  $columnname = "CatDepartment";
  $queryvalue = "Zoology";
  $terms[] = array($columnname, $queryvalue);
  /* The query is run by this command and results are stored server side
  */
  $module->findTerms(array('and', $terms));

  // Some useful debug info.
  print "Port: {$session->port}" . PHP_EOL;
  print "ID: {$module->id}" . PHP_EOL;
  print "Context: {$session->context}" . PHP_EOL;

  return array(
    'module_id' => $module->id,
    'session_context' => $session->context,
    'session_port' => $session->port,
  );
}


function phase2($data) {
  print_r($data);
  $session = new IMuSession;
  $session->host = "66.158.71.71";
  /* We're using a different port. This is a 're-connect'
  ** port.
  */
  $session->port = $data['session_port'];
  $session->context = $data['session_context'];
  $session->connect();
  //$session->login('emu', 'emucorneliafmnh');

  $emutable = 'ecatalogue';
  $module = new IMuModule($emutable, $session);
  /* We are going to specify the 'id' of this module to
  ** reconnect to the module previously used.
  */
  //$module->id = 'some_id_of_some_sort';
  $module->id = $data['module_id'];

  /* We'll setup the columns we want returned.
  */
  $columns = array();
  /* A basic column
  */
  $columns[] = 'SummaryData';

  $module->addFetchSet('test', $columns);

  /* We'll now fetch those columns
  */
  $position = "current"; /* or "current" or "end" */
  $offset = 0;
  $count = 1;
  $fp = fopen("newtest.out", "a");

  for ($i = 0; $i < 200; $i++) {
    try {
      $result = $module->fetch($position, $offset, $count, 'test');
      //$result = $module->fetch($position, $offset, $count, $columns);
    }
    catch (Exception $e) {
    }

    $rows = $result->rows;
    if ($rows) {
      foreach($rows as $row) {
        $rownumber = $row['rownum'];
        fwrite($fp, "$rownumber\n");
      }
    }
  }
  fclose($fp);

}

