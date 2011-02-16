<?php

class KiwiImuSession extends IMuSession {

  /**
   * The configuration object for this session.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * Constructor.
   *
   * This overrides the parent constructor just to let us pass in a config object.
   *
   * @param KiwiConfiguration $config
   * @param <type> $host
   * @param <type> $port
   */
  public function __construct(KiwiConfiguration $config, $host = FALSE, $port = FALSE) {
    $this->config = $config;

    parent::__construct($host, $port);
  }

  /**
   * Returns a new IMuModules object for this configuration.
   *
   * @return IMuModules
   */
  public function getModulesHandler() {
    $base_table = $this->config->baseTable();
    $module = new IMuModules($this);

    $module->setModules(array($base_table['name']));

    return $module;
  }

}
