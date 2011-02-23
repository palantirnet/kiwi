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

/**
 * Main driver for the program.
 *
 * This is the main entry point for the whole script.
 */
function main() {
  includeEmu();
  includeKiwi();
  includeQueryPath();
  includeSolr();

  IMuTrace::setFile('trace.txt');
  IMuTrace::setLevel(1);

  date_default_timezone_set('America/Chicago');

  $config = new KiwiConfiguration('config.xml');

  // Build the main query on the Emu server.
  $module_id = main_generator($config);

  debug($module_id, 'Module ID');

  // Spawn off children to do work.
  for ($child_id = 1, $num_processors = $config->numChildProcesses(); $child_id <= $num_processors; ++$child_id) {
    $pid = pcntl_fork();
    if ($pid == -1) {
      die('could not fork');
    }
    else if ($pid) {
      // This is the parent.
      pcntl_wait($status); //Protect against Zombie children.
      main_cleanup($config);
    }
    else {
      // This is the child.
      main_processor($config, $child_id, $module_id);
      // Kill the child process when done.
      exit(0);
    }
  }

  exit();
}

/**
 * Generates a new query result on the Emu server and returns the module ID.
 *
 * @param KiwiConfiguration $config
 *   The configuration object for this run.
 * @return string
 *   The Module ID of the result set object.
 */
function main_generator(KiwiConfiguration $config) {
  $server_info = $config->getEmuInfo();
  $session = new KiwiImuSession($config, $server_info['host'], $server_info['port']);

  $generator = new KiwiQueryGenerator($config, $session);

  return $generator->run();

  // The generator object goes out of scope here, clearing all outstanding
  // resources.
}

/**
 * Runs the cleanup routines for after all child processes are done.
 *
 * @param KiwiConfiguration $config
 *   The configuration object for this run.
 */
function main_cleanup(KiwiConfiguration $config) {

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

  //$solr->deleteByQuery('*:*');

}

/**
 *
 * @param KiwiConfiguration $config
 *   The configuration object for this run.
 * @param int $child_id
 *   The ID of this processor. It is a simple integer, unique within this
 *   script run only.
 * @param string $module_id
 *   The Module ID of the result set object on whic to work.
 */
function main_processor(KiwiConfiguration $config, $child_id, $module_id) {
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
}

main();
