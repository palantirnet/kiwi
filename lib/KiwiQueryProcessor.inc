<?php

/**
 * Worker class for processing a result set.
 *
 * This class should be run in its own process, and each process should be
 * more or less self-contained in this class as much as possible.
 */
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

  /**
   * Constructor.
   *
   * @param int $processor_id
   *   The ID of this processor. It is a simple integer, unique within this
   *   script run only.
   * @param string $module_id
   *   The ID of the module on the Emu to which we want to reconnect.
   * @param KiwiConfiguration $config
   *   The configuration object for this processor.
   * @param KiwiImuSession $session
   *   The session on which to run all queries.
   */
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

    $position = 'current';
    $offset = 0;
    $count = $this->config->numDocumentsPerBatch();

    // @todo This will get replaced with a real value in due time.
    $columns = FALSE;

    // I'm not sure how to fencepost fetch() when it runs out of records.
    // For the time being I'm just hacking around that, but we should call
    // Aaron and find out. :-)
    for ($i=0; $i < 200; ++$i) {
      $this->module->fetch($position, $offset, $count, $columns);
    }
  }
}
