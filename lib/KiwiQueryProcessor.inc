<?php


class KiwiQueryProcessor {

  /**
   * The configuration object for this processor.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The ImuSession object on which to run queries.
   *
   * @var KiwiImuSession
   */
  protected $session;

  /**
   * The ID of this processor.
   *
   * @var int
   */
  protected $processorId;

  /**
   * The module object for the initial query.
   *
   * We will likely spawn other queries, but this represents the "main" result set.
   *
   * @var IMuModules
   */
  protected $module;

  public function __construct($processor_id, $module_id, KiwiConfiguration $config, KiwiImuSession $session) {
    $this->processorId = $processor_id;
    $this->config = $config;
    $this->session = $session;

    $this->module = $this->session->resumeModulesHandler($module_id);

  }

  /**
   * Process records in the result set until we run out.
   */
  public function run() {

    // I'm not sure how to fencepost fetch() when it runs out of records.
    // For the time being I'm just hacking around that, but we should call
    // Aaron and find out. :-)
    $position = 'current';
    $offset = 0;
    $count = 1;
    $columns = FALSE;

    for ($i=0; $i < 200; ++$i) {
      $this->module->fetch($position, $offset, $count, $columns);
    }
  }
}
