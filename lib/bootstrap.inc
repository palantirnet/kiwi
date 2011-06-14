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

/**
 * Include the Kiwi libraries.
 */
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

/**
 * Include QueryPath.
 */
function includeQueryPath() {
  require_once('QueryPath/QueryPath.php');
}

/**
 * Include the ApacheSolr client library.
 */
function includeSolr() {
  require_once('SolrPhpClient/Apache/Solr/Service.php');
  require_once('SolrPhpClient/Apache/Solr/Document.php');
  require_once('SolrPhpClient/Apache/Solr/Response.php');
}

/**
 * Initialize the system.
 */
function kiwi_init() {
  includeEmu();
  includeSolr();
  includeKiwi();
  includeQueryPath();
}

/**
 * Main driver for the program.
 *
 * This is the main entry point for the whole script.
 */
function main(KiwiInput $input = NULL, KiwiConfiguration $config = NULL) {

  IMuTrace::setFile('trace.txt');
  IMuTrace::setLevel(1);

  date_default_timezone_set('America/Chicago');
  set_error_handler('exceptions_error_handler');

  try {

    $timer_run = new KiwiTimer();
    if (empty($input)) {
      $input = new KiwiInput();
      $input->parse();
    }

    if (empty($config)) {
      $config = new KiwiConfiguration($input);
    }

    $imu_factory = new KiwiImuFactory($config);

    $config_info = $config->getConfigInfo();

    KiwiOutput::get()->setThreshold($config->defaultVerbosity());

    // Build the main query on the Emu server.
    $timer_generator = new KiwiTimer();
    $reconnect = main_generator($config, $imu_factory);
    KiwiOutput::info('Generator time: ' . number_format($timer_generator->stop(), 2) . ' seconds');

    // If specified, clear the existing Solr core before adding new content.
    if ($config_info['full-rebuild']) {
      $timer_solr_clear = new KiwiTimer();
      $server_info = $config->getSolrInfo();
      KiwiOutput::info("Purging old solr index...");
      $solr = new KiwiSolrService($server_info['host'], $server_info['port'], $server_info['path']);
      $solr->deleteByQuery('*:*');
      unset($solr); // Close the connection.
      KiwiOutput::info('Solr purge time: ' . number_format($timer_solr_clear->stop(), 2) . ' seconds');
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
        main_processor($config, $child_id, $imu_factory, $reconnect);
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
    $message = sprintf("Unknown error: %s\n\tin %s on line %s", $e->getMessage(), $e->getFile(), $e->getLine());
    KiwiOutput::get()->writeMessage($message, LOG_ERR);
  }

  KiwiOutput::info('Total run time: ' . number_format($timer_run->stop(), 2) . ' seconds');
}

/**
 * Replacement global error handler.
 *
 * This handler converts all PHP errors into exceptions so that they can be
 * centrally handled.  This does make all errors "harder" in that they are not
 * recoverable, but that's OK.  We want the system to die fast and hard so we
 * can fix bugs.
 *
 * @link http://www.php.net/set_error_handler
 */
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
 * @return array
 *   A three element array containing information about how to reconnect to the
 *   session and module used in the generator.  The keys are:
 *     - session_context
 *     - session_port
 *     - module_id
 */
function main_generator(KiwiConfiguration $config, KiwiImuFactory $factory) {
  KiwiOutput::info("Generating initial Emu query...");

  $session = $factory->getNewEmuSession(TRUE);

  $generator = new KiwiQueryGenerator($config, $session);

  $module_id = $generator->run();

  return array(
    'session_context' => $session->context,
    'session_port' => $session->port,
    'module_id' => $module_id,
  );

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
  $solr = new KiwiSolrService($server_info['host'], $server_info['port'], $server_info['path']);
  $timer_commit = new KiwiTimer();
  KiwiOutput::info("Committing Solr data...");
  $solr->commit();
  KiwiOutput::info('Solr commit time: ' . number_format($timer_commit->stop(), 2) . ' seconds');

  $timer_optimize = new KiwiTimer();
  KiwiOutput::info("Optimizing Solr index...");
  $solr->optimize();
  KiwiOutput::info('Solr optimize time: ' . number_format($timer_optimize->stop(), 2) . ' seconds');
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
function main_processor(KiwiConfiguration $config, $child_id, KiwiImuFactory $factory, $reconnect) {
  KiwiOutput::info("Starting processor {$child_id}...");

  $session = $factory->resumeEmuSession($reconnect['session_context'], $reconnect['session_port']);

  $server_info = $config->getSolrInfo();
  $solr = new KiwiSolrService($server_info['host'], $server_info['port'], $server_info['path']);

  $processor = new KiwiQueryProcessor($child_id, $reconnect['module_id'], $config, $session, $solr);

  try {
    //KiwiOutput::get()->setThreshold(LOG_INFO);
    $processor->run();
  }
  catch(Exception $e) {
    //debug($e->getTrace());
    debug('Error message is: ' . (string)$e);
    debug('Error code is: ' . $e->getCode());
    debug('Error line is: ' . $e->getFile() . ', line '. $e->getLine());
    debug(get_stack($e));
  }

  KiwiOutput::debug("Processor {$child_id}: Maximum memory used (bytes): " . number_format(memory_get_peak_usage(TRUE)));
}
