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

    // Run initial query as defined by the config object.
    // This may require a query builder.

    // Return the module ID
  }

}
