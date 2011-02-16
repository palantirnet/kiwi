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

  $generator = new KiwiQueryGenerator($config);

  $module_id = $generator->run();

  debug($module_id, 'Module ID');

}

main();
