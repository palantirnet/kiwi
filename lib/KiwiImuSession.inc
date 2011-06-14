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
   * Cache of field information from the schema.
   *
   * This is mostly to work around ugly ugly nested arrays in the return from
   * IMuSession::getTableSchema();
   *
   * @var array
   */
  protected $fieldInfo = array();

  /**
   * Cache of table schema information.
   *
   * This is mostly to work around ugly ugly nested arrays in the return from
   * IMuSession::getTableSchema();
   *
   * @var array
   */
  protected $tableInfo = array();

  /**
   * Cache of table schema objects.
   *
   * @var array
   */
  protected $tables = array();

  /**
   * A collection of field handler objects.
   *
   * These are stateless and reusable, so we can cache them and only have to
   * instantiate one of each.
   *
   * @var array
   */
  protected $handlers = array();

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

    // If no host and port were specified, extract them from the system configuration.
    $server_info = $this->config->getEmuInfo();
    if (!$host) {
      $host = $server_info['host'];
    }
    if (!$port) {
      $port = $server_info['port'];
    }

    parent::__construct($host, $port);
  }

  /**
   * Destructor.
   */
  public function  __destruct() {
    // I'm not certain if PHP will properly clean up the resource properties on
    // this object in the parent class when it goes out of scope, so just
    // to be safe we'll do it explicitly.
    $this->disconnect();
  }

  /**
   * Returns a new IMuModule object for a new handler.
   *
   * @see KiwiImuSession::resumeModuleHandler();
   * @param boolean $destroy
   *   Whether or not to destroy the server-side result object when the PHP
   *   object goes out of scope.  The default is to let it be released. This
   *   must be set to TRUE to allow the result set to be reconnected to later.
   *
   * @return IMuModule
   */
  public function newModuleHandler($base_table, $destroy = TRUE) {
    $module = new IMuModule($base_table, $this);

    // Leave the result set open on the server.  That allows us to reconnect
    // to it from the child processes later.
    $module->destroy = $destroy;

    return $module;
  }

  /**
   * Returns a new IMuModule object for an existing handler on the server.
   *
   * @param string $module_id
   *   The ID of the existing module handler on the Emu server to which to
   *   reconnect.
   * @return IMuModule
   */
  public function resumeModuleHandler($module_id) {
    $base_table = $this->config->baseTable();
    $module = new IMuModule($base_table['name'], $this);

    $module->id = $module_id;

    return $module;
  }

  /**
   * Returns schema information for the specified field.
   *
   * @param string $table
   *   The table on which the field exists.
   * @param string $field
   *   The field for which we want available schema information.
   * @return array
   *   The field schema information for the specified field.
   */
  public function getFieldInfo($table, $field) {
    if (empty($this->fieldInfo[$table][$field])) {
      $schema = $this->getSchemaInfo($table);
      $this->fieldInfo[$table][$field] = $schema['schema']['columns'][$field];
    }

    return $this->fieldInfo[$table][$field];
  }

  /**
   * Returns schema information for the specified table.
   *
   * This method is mostly just a caching wrapper around IMuSession::getTableSchema().
   *
   * @param string $table
   *   The table for which we want schema information.
   * @return array
   *   Full schema information array for the specified table.
   */
  public function getSchemaInfo($table) {
    if (empty($this->tableInfo[$table])) {
      $result = $this->getTableSchema($table);
      if ($result['status'] == 'ok') {
        $this->tableInfo[$table] = $result['result'];
      }
      else {
        throw new KiwiEmuConnectionException("Unable to retrieve table schema: {$able}", $result['status']);
      }
    }

    return $this->tableInfo[$table];
  }

  /**
   * Returns an object representing a given table in the Emu schema.
   *
   * This object will be pre-filtered to only know about fields that are in
   * our current configuration.
   *
   * @param string $name
   *   The name of the table we want information about.
   * @param string $type
   *   The type of table.  Values are attach, merge, or collapse.
   * @return KiwiEmuTable
   */
  public function getTable($name, $type = 'attach') {
    if (empty($this->tables[$name][$type])) {
      $this->tables[$name][$type] = new KiwiEmuTable($this->config, $this, $this->getSchemaInfo($name), $type);
    }
    return $this->tables[$name][$type];
  }

  public function getTables($type) {
    $tables = array();
    foreach ($this->config->getTables($type) as $name) {
      $tables[] = $this->getTable($name, $type);
    }
    return $tables;
  }

  /**
   * Returns a single instance of the specified handler class.
   *
   * This is a factory function that will reuse classes once instantiated.
   * That means all handler classes must be stateless.
   *
   * @param string $class
   *   The name of the handler class to return.
   * @return FieldHandlerInterface
   */
  public function getHandler($class) {
    if (empty($this->handlers[$class])) {
      $this->handlers[$class] = new $class($this->config, $this);
    }
    return $this->handlers[$class];
  }
}

/**
 * Exception for connection errors from Emu.
 */
class KiwiEmuConnectionException extends Exception {}

/**
 * Exception for a missing Field in the schema.
 */
class KiwiEmuFieldNotFoundException extends Exception {}

