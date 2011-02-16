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
/* We don't really need these, but they can be useful
*/
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/trace.php';

date_default_timezone_set('America/Chicago');

IMuTrace::setFile('trace.txt');
IMuTrace::setLevel(1);

function main() {
  $config = new KiwiConfiguration('');

  $generator = new KiwiQueryGenerator($config);

  $generator->run();

}


/**
 * Configuration object for a Kiwi execution run.
 */
class KiwiConfiguration {

  /**
   * Constructor.
   *
   * @param string $file
   *   The name of the file with the configuration to load.
   */
  function __construct($file = '') {

  }

}


class KiwiQueryGenerator {

  /**
   * The configuration object for this generator.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  public function __construct(KiwiConfiguration $config) {
    $this->config = $config;
  }

  public function run() {
    // Connect to Emu server.

    // Run initial query as defined by the config object.
    // This may require a query builder.

    // Return the module ID
  }

}

class KiwiWorker {

  /**
   * The configuration object for this generator.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The module ID we should load.
   *
   * This is a unique identifier for the result set object in Emu.
   *
   * @var string
   */
  protected $moduleId = '';

  public function __construct(KiwiConfiguration $config, $module_id) {
    $this->config = $config;
    $this->moduleId = $module_id;

    // Create $this->module object.

    // Create $this->solrConnection object.
  }

  public function run() {
    // Create $solr_document.

    // Create field list for all possible fields to fetch. This includes from
    // the base table and 1:1 mappings (Append).

    // Extract those fields.

    // Add those fields to $solr_document.

    // Process table fields and add those to $solr_document.

    // Process Merge tables.

    // Process Collapse tables.

  }

}
