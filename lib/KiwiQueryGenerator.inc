<?php

/**
 * Generator class for a processing run.
 *
 * This object will only create a result set on the Emu server and return its
 * ID.  It will not do any processing of that result set.
 *
 * @see KiwiQueryProcessor
 */
class KiwiQueryGenerator {

  /**
   * The configuration object for this generator.
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

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session) {
    $this->config = $config;
    $this->session = $session;
  }

  public function run() {

    $module = $this->session->newModuleHandler($base_table['name'], FALSE);

    // Run initial query as defined by the config object.
    $terms = array();
    foreach ($this->config->getFilters() as $filter) {
      $term = array($filter->attr('name'), $filter->attr('value'));
      if ($operator = $filter->attr('operator')) {
        $term[] = $operator;
      }
      $terms[] = $term;
    }

    KiwiOutput::debug($terms, 'Query terms');

    $module->findTerms(array('and', $terms));

    // Return the module ID
    return $module->id;
  }

}

class EmuQuery {

  /**
   * The configuration object for this generator.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The session against which to run a query.
   *
   * @var KiwiImuSession
   */
  protected $session;

  public function __construct(KiwiImuSession $session, KiwiConfiguration $config) {
    $this->session = $session;
    $this->config = $config;

    $base_table = $this->config->baseTable();
    $this->module = new IMuModule($base_table['name'], $session);
  }

  public function addTerm($column, $term, $operator = '') {
    if ($operator) {
      $this->terms[] = array($column, $term, $operator);
    }
    else {
      $this->terms[] = array($column, $term);
    }
  }

  public function execute() {
    $this->module->findTerms($this->terms);
  }
}
