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

    parent::__construct($host, $port);
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
  public function newModuleHandler($destroy = TRUE) {
    $base_table = $this->config->baseTable();
    $module = new IMuModule($base_table['name'], $this);

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
      $this->tableInfo[$table] = $this->getTableSchema($table);
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
   * @return KiwiEmuTable
   */
  public function getTable($name) {
    if (empty($this->tables[$name])) {
      $this->tables[$name] = new KiwiEmuTable($this->config, $this, $this->getSchemaInfo($name));
    }
    return $this->tables[$name];
  }

  public function getTables() {
    $this->config->getTables('attach');
  }

  public function getHandler($class) {
    if (empty($this->handlers[$class])) {
      $this->handlers[$class] = new $class($this->config, $this);
    }
    return $this->handlers[$class];
  }
}

class KiwiEmuTable implements IteratorAggregate {

  protected $schema;

  protected $fields;

  protected $config;

  protected $session;

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session, $schema) {
    $this->schema = $schema;
    $this->config = $config;
    $this->session = $session;

    // Split out our fields up front, since we're almost certain to need them.
    foreach ($this->config->getFields($this->getName()) as $field) {
      $this->fields[$field->attr('name')] = new KiwiEmuField($this->config, $this->session, $this->getName(), $this->schema['schema']['columns'][$field->attr('name')]);
    }
  }

  public function getName() {
    return $this->schema['schema']['table'];
  }

  public function getField($field) {
    return $this->fields[$field];
  }

  public function getIterator() {
    return new ArrayIterator($this->fields);
  }

}

/**
 * Wrapper class representing a field in Emu.
 */
class KiwiEmuField {

  /**
   * The Emu schema definition for this field.
   *
   * @var array
   */
  protected $schema;

  /**
   * The configuration object for this run.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The session object to which this field is bound.
   *
   * @var KiwiImuSession
   */
  protected $session;

  /**
   * The name of the table this field belongs to.
   *
   * This is a string, not an object reference, to avoid another circular
   * reference.  The table object can be derived from this string with
   *
   * @code
   * $this->session->getTable($this->table);
   * @endcode
   * 
   * if necessary.
   *
   * @var string
   */
  protected $table;

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session, $table, $schema) {
    //debug($schema);
    $this->config = $config;
    $this->session = $session;
    $this->schema = $schema;
    $this->table = $table;
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

  public function itemName() {
    return $this->schema['ItemName'];
  }

  public function columnName() {
    return $this->schema['ColumnName'];
  }

  public function isAttachmentField() {
    return ($this->schema['DataKind'] == 'dkAtom' && substr($this->schema['ColumnName'], -3) == 'Ref');
  }

  public function refTable() {
    return !empty($this->schema['RefTable']) ? $this->schema['RefTable'] : '';
  }

  public function dataType() {
    return $this->schema['DataType'];
  }

  /**
   * Returns a mapping handler object for the this field.
   *
   * @return FieldHandlerInterface
   */
  public function getHandler() {

    $field_options = $this->config->getFieldOptions($this->table, $this->columnName(), $this->dataType());
    $class = $field_options['handler'];
    return $this->session->getHandler($class);
  }
}
