<?php

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

  public function getConnection() {
    $server_info = $this->config->getEmuInfo();
    $session = new KiwiImuSession($this->config, $server_info['host'], $server_info['port']);
    //$session->host = "66.158.71.71";
    //$session->port = "40000";
    $session->connect();

    // We'll want to suspend later once we start processing.  For now we don't need it.
    //$session->suspend = true;

    return $session;
  }

  public function run() {
    
    // Connect to Emu server.
    $session = $this->getConnection();


    //$query = new EmuQuery($session, $this->config);

    $base_table = $this->config->baseTable();
    debug($base_table);
    $module = new IMuModule($base_table['name'], $session);


    // Run initial query as defined by the config object.
    // This may require a query builder.

    // This is just a placeholder.  Apparently you cannot findTerms() without
    // specifing terms; that is, you must have a WHERE clause. Bah.
    // @TODO do something else here.
    $terms = array();
    $columnname = "CatDepartment";
    $queryvalue = "Zoology";
    $terms[] = array($columnname, $queryvalue);
    $columnname = "PrePrepType_tab";
    $queryvalue = "alcohol";
    $terms[] = array($columnname, $queryvalue);

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
