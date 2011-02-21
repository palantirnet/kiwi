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

function main() {
  includeEmu();
  includeKiwi();
  includeQueryPath();

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
      $processor = new KiwiQueryProcessor($child_id, $module_id, $config, $session);

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

  exit();
}

main();
