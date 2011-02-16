#!/usr/bin/php
<?php

/* First we'll import the base API library
*/
require_once 'emu/imu.php';
/* Next we'll import the module library, as that will
** provide useful tools for querying and returning
** results
*/
require_once IMu::$lib . '/module.php';
require_once IMu::$lib . '/modules.php';
/* We don't really need these, but they can be useful
*/
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/trace.php';

date_default_timezone_set('America/Chicago');

IMuTrace::setFile('trace.txt');
IMuTrace::setLevel(1);

// Include the Kiwi libraries.
require_once('lib/util.inc');
require_once('lib/KiwiConfiguration.inc');
require_once('lib/KiwiQueryGenerator.inc');
require_once('lib/KiwiWorker.inc');
require_once('lib/KiwiImuSession.inc');


function main() {
  $config = new KiwiConfiguration('');

  $generator = new KiwiQueryGenerator($config);

  $module_id = $generator->run();

  debug($module_id, 'Module ID');

}

main();
