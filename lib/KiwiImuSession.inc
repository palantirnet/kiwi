<?php

/**
 * EMu session connection object.
 *
 * We are extending the core IMu version so that we can add some utility
 * methods.  Otherwise there's nothing really interesting about it.
 */
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
   * @param string $host
   *   The host name or IP of the Emu server.
   * @param int $port
   *   The port on the server to which to connect.
   */
  public function __construct(KiwiConfiguration $config, $host = FALSE, $port = FALSE) {
    $this->config = $config;

    parent::__construct($host, $port);
  }

  /**
   * Returns a new IMuModules object for a new handler.
   *
   * @see KiwiImuSession::resumeModulesHandler();
   * @param boolean $destroy
   *   Whether or not to destroy the server-side result object when the PHP
   *   object goes out of scope.  The default is to let it be released. This
   *   must be set to TRUE to allow the result set to be reconnected to later.
   *
   * @return IMuModules
   */
  public function newModulesHandler($destroy = TRUE) {
    $module = new IMuModules($this);

    // Leave the result set open on the server.  That allows us to reconnect
    // to it from the child processes later.
    $module->destroy = $destroy;

    $tables = $this->config->getTables('attach');

    $module->setModules($tables);

    return $module;
  }

  /**
   * Returns a new IMuModules object for an existing handler on the server.
   *
   * @param string $module_id
   *   The ID of the existing module handler on the Emu server to which to
   *   reconnect.
   * @return IMuModules
   */
  public function resumeModulesHandler($module_id) {
    $module = new IMuModules($this);

    $module->id = $module_id;

    $tables = $this->config->getTables('attach');

    $module->setModules($tables);

    return $module;
  }

}
