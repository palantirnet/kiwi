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



class KiwiQueryGenerator {

  public function __construct($config) {
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

  public function __construct($config, $module_id) {
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
