<?php

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
