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
  foreach (glob('lib/*.inc') as $file) {
    require_once($file);
  }

  // Include any custom handlers.
  foreach (glob('handlers/*.inc') as $file) {
    require_once($file);
  }
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
  set_error_handler('exceptions_error_handler');

  try {
    $input = new KiwiInput();
    $input->parse();

    $config = new KiwiConfiguration($input);

    $config_info = $config->getConfigInfo();

    KiwiOutput::get()->setThreshold($config_info['debug']);

    // Build the main query on the Emu server.
    $module_id = main_generator($config);

    KiwiOutput::debug($module_id, 'Module ID');

    // If specified, clear the existing Solr core before adding new content.
    if ($config_info['full-rebuild']) {
      $server_info = $config->getSolrInfo();
      KiwiOutput::info("Purging old solr index...");
      $solr = new Apache_Solr_Service($server_info['host'], $server_info['port'], $server_info['path']);
      $solr->deleteByQuery('*:*');
      unset($solr); // Close the connection.
    }

    $children = array();

    // Spawn off children to do work.
    for ($child_id = 1, $num_processors = $config->numChildProcesses(); $child_id <= $num_processors; ++$child_id) {
      $pid = pcntl_fork();
      if ($pid == -1) {
        die('could not fork');
      }
      else if ($pid) {
        // This is the parent. Do nothing here but let the loop complete.
        $children[] = $pid;
      }
      else {
        // This is the child.
        main_processor($config, $child_id, $module_id);
        // Kill the child process when done.
        exit(0);
      }
    }

    // Wait for all of the spawned children to die.
    // This may end up waiting for process 1 long after process 3 is done, but
    // that's OK. It ensures that all processes are truly done before continuing,
    // whatever order they finish in, which is what we want.
    $status = 0;
    foreach ($children as $pid) {
     pcntl_waitpid($pid, $status);
    }
    main_cleanup($config);
  }
  catch (ConfigFileNotFoundException $e) {
    KiwiOutput::get()->writeMessage($e->getMessage(), LOG_ERR);
  }
  catch (InvalidConfigOptionException $e) {
    KiwiOutput::get()->writeMessage($e->getMessage(), LOG_ERR);
    KiwiOutput::get()->writeMessage(PHP_EOL . PHP_EOL . $input->getInstructions());
  }
  catch (Exception $e) {
    KiwiOutput::get()->writeMessage('Unknown error: ' . $e->getMessage(), LOG_ERR);
  }

  exit();
}

function exceptions_error_handler($severity, $message, $filename, $lineno) {
  if (error_reporting() == 0) {
    return;
  }
  if (error_reporting() & $severity) {
    throw new ErrorException($message, 0, $severity, $filename, $lineno);
  }
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
  KiwiOutput::info("Generating initial Emu query...");

  $server_info = $config->getEmuInfo();
  $session = new KiwiImuSession($config, $server_info['host'], $server_info['port']);
  $session->login($server_info['user'], $server_info['password']);

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
  KiwiOutput::info("Cleaning up...");

  // Close the result set object.
  // @todo I'm not sure if we need to, since the modules in the children may
  // do so.  TBD.

  // Commit any pending changes to the Solr index and tell Solr to optimize itself.
  // Note that these are synchronous operations, so that we can time the entire
  // process including Solr rebuild.  It may or may not make sense for this to
  // be synchronous later.
  $server_info = $config->getSolrInfo();
  $solr = new Apache_Solr_Service($server_info['host'], $server_info['port'], $server_info['path']);
  KiwiOutput::info("Committing Solr data...");
  $solr->commit();
  KiwiOutput::info("Optimizing Solr index...");
  $solr->optimize();
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
  KiwiOutput::info("Starting processor {$child_id}...");
  $server_info = $config->getEmuInfo();
  $session = new KiwiImuSession($config, $server_info['host'], $server_info['reconnect-port']);
  $session->login($server_info['user'], $server_info['password']);

  $server_info = $config->getSolrInfo();
  $solr = new Apache_Solr_Service($server_info['host'], $server_info['port'], $server_info['path']);

  $processor = new KiwiQueryProcessor($child_id, $module_id, $config, $session, $solr);

  try {
    //KiwiOutput::get()->setThreshold(LOG_INFO);
    $processor->run();
  }
  catch(Exception $e) {
    debug($e->getTrace());
    debug('Error message is: ' . (string)$e);
    debug('Error code is: ' . $e->getCode());
  }

  KiwiOutput::debug("Processor {$child_id}: Maximum memory used (bytes): " . number_format(memory_get_peak_usage(TRUE)));
}

main();
