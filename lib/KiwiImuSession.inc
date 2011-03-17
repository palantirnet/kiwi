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
      $this->tableInfo[$table] = $this->getTableSchema($table);
    }
    return $this->tableInfo[$table];
  }

  public function getTable($name) {
    if (empty($this->tables[$name])) {
      $this->tables[$name] = new KiwiEmuTable($this->getSchemaInfo($name));
    }
    return $this->tables[$name];
  }
}

class KiwiEmuTable implements IteratorAggregate {

  protected $schema;

  protected $fields;

  public function __construct($schema) {
    $this->schema = $schema;

    // Split out our fields up front, since we're almost certain to need them.
    foreach ($schema['schema']['columns'] as $field => $field_info) {
      $this->fields[$field] = new KiwiEmuField($field_info);
    }
  }

  public function getField($field) {
    return $this->fields[$field];
  }

  public function getIterator() {
    return new ArrayIterator($this->fields);
  }

}

class KiwiEmuField {

  protected $schema;

  public function __construct($schema) {
    $this->schema = $schema;
  }

  /**
   * Returns true if this field is multi-value, false otherwise.
   *
   * Note that this depends on the Emu field schema, not on the number of values
   * the field may actually have at any given time.
   *
   * @return boolean
   */
  public function isMultiValue() {
     return (substr($this->schema['ColumnName'], -4) == '_tab');
  }
}
