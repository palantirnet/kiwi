#!/usr/bin/php
<?php

// Enable OCD pedantic error reporting.
error_reporting(E_ALL | E_STRICT);

function includeEmu() {
  require_once 'emu/imu.php';
  require_once IMu::$lib . '/module.php';
  require_once IMu::$lib . '/modules.php';
  require_once IMu::$lib . '/exception.php';
  require_once IMu::$lib . '/trace.php';
}

function includeKiwi() {
  // Include the Kiwi libraries.
  require_once('lib/util.inc');
  require_once('lib/KiwiConfiguration.inc');
  require_once('lib/KiwiQueryGenerator.inc');
  require_once('lib/KiwiQueryProcessor.inc');
  require_once('lib/KiwiImuSession.inc');
}

function includeQueryPath() {
  require_once('QueryPath/QueryPath.php');
}

function includeSolr() {
  require_once('SolrPhpClient/Apache/Solr/Service.php');
  require_once('SolrPhpClient/Apache/Solr/Document.php');
  require_once('SolrPhpClient/Apache/Solr/Response.php');
}

function main() {
  includeEmu();
  includeKiwi();
  includeQueryPath();
  includeSolr();

  IMuTrace::setFile('trace.txt');
  IMuTrace::setLevel(1);

  date_default_timezone_set('America/Chicago');

  $config = new KiwiConfiguration('config.xml');

  $server_info = $config->getEmuInfo();
  $session = new KiwiImuSession($config, $server_info['host'], $server_info['port']);

  $generator = new KiwiQueryGenerator($config, $session);

  $module_id = $generator->run();

  debug($module_id, 'Module ID');

  // Destroy the generator to get rid of all outstanding resource objects
  // before we fork.
  unset($generator);

  // Spawn off children to do work.
  for ($child_id = 1, $num_processors = $config->numChildProcesses(); $child_id <= $num_processors; ++$child_id) {
    $pid = pcntl_fork();
    if ($pid == -1) {
      die('could not fork');
    }
    else if ($pid) {
      // This is the parent.
    }
    else {
      // This is the child.
      debug("Child {$child_id} running...");
      $server_info = $config->getEmuInfo();
      $session = new KiwiImuSession($config, $server_info['host'], $server_info['reconnect-port']);

      $server_info = $config->getSolrInfo();
      $solr = new Apache_Solr_Service($server_info['host'], $server_info['port'], $server_info['path']);

      $processor = new KiwiQueryProcessor($child_id, $module_id, $config, $session, $solr);

      //try {
        $processor->run();
      //}
      //catch(Exception $e) {
        //debug($e->getTrace());
        //debug('Error message is: ' . $e->getMessage());
        //debug('Error code is: ' . $e->getCode());
      //}
      exit(0);
    }
  }

  // We are the parent.
  pcntl_wait($status); //Protect against Zombie children

  // Close the result set object.
  // @todo I'm not sure if we need to, since the modules in the children may
  // do so.  TBD.

  // Commit any pending changes to the Solr index and tell Solr to optimize itself.
  // This is mostly just cleanup for performance.  The commit is set to async
  // (the two FALSE values) so that it can run in the background on its own
  // without making the user wait.
  // @todo: It may make sense to make the commit synchronous so that we don't
  // return until we're for-reals done.  At least for benchmarking.
  $server_info = $config->getSolrInfo();
  $solr = new Apache_Solr_Service($server_info['host'], $server_info['port'], $server_info['path']);
  $solr->commit(TRUE, FALSE, FALSE);


  exit();
}

main();
