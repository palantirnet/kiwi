<?php

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
