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

  public function getConnection() {
    $this->session->connect();

    // Leave the session open for later use when the child processors come by.
    $this->session->suspend = TRUE;

    return $this->session;
  }

  public function run() {
    
    // Connect to Emu server.
    $session = $this->getConnection();

    $module = $session->newModulesHandler(FALSE);

    // Run initial query as defined by the config object.
    // This may require a query builder.

    // This is just a placeholder.  Apparently you cannot findTerms() without
    // specifing terms; that is, you must have a WHERE clause. Bah.
    // @TODO do something else here.
    $terms = array();
    //$terms[] = array('CatDepartment', 'Zoology');
    //$terms[] = array('ColEarliestDateCollected', '18/09/1939');

    $filters = $this->config->getFilters();
    foreach ($filters as $filter) {
      $term = array($filter->attr('name'), $filter->attr('value'));
      if ($operator = $filter->attr('operator')) {
        $term[] = $operator;
      }
      $terms[] = $term;
    }

    debug($terms);

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
