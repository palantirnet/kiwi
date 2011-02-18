#!/usr/bin/php
<?php


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
  require_once('lib/KiwiWorker.inc');
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
  for ($child_id = 1; $child_id <= $config->numChildProcesses(); ++$child_id) {
    $pid = pcntl_fork();
    if ($pid == -1) {
      die('could not fork');
    }
    else if ($pid) {
      // This is the parent.
    }
    else {
      debug("Child {$child_id} running...");
      // This is the child.
      $server_info = $config->getEmuInfo();
      $session = new KiwiImuSession($config, $server_info['host'], $server_info['reconnect-port']);
      $module = $session->resumeModulesHandler($module_id);
      exit(0);
    }
  }

  // We are the parent
  pcntl_wait($status); //Protect against Zombie children

  // Close the result set object.


  exit();
}

main();
