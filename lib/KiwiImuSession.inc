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

class KiwiEmuTable implements IteratorAggregate {

  /**
   * The Emu schema definition for this table.
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
   * An array of the fields that make up this table.
   *
   * This value is filtered to only those fields that are in our configuration
   * as a field we care about.
   *
   * @var array
   */
  protected $fields = array();

  /**
   * Constructor
   *
   * @param KiwiConfiguration $config
   *   The configuration object for this run.
   * @param KiwiImuSession $session
   *   The Emu session connection object we are processing.
   * @param array $schema
   *   The schema definition for this table.
   */
  public function __construct(KiwiConfiguration $config, KiwiImuSession $session, $schema, $table_type = 'attach') {
    $this->schema = $schema;
    $this->config = $config;
    $this->session = $session;

    // Split out our fields up front, since we're almost certain to need them.
    foreach ($this->config->getFields($this->getName(), 'all', $table_type) as $field_name) {
      if (isset($this->schema['schema']['columns'][$field_name])) {
        $this->fields[$field_name] = new KiwiEmuField($this->config, $this->session, $this->getName(), $this->schema['schema']['columns'][$field_name]);
      }
      else {
        KiwiOutput::get()->writeMessage("Field '{$field_name}' not found in schema for table '{$this->schema['schema']['table']}'", LOG_WARNING);
      }
    }
  }

  /**
   * Returns the name of the table.
   *
   * @return string
   */
  public function getName() {
    return $this->schema['schema']['table'];
  }

  /**
   * Returns a Field object for the specified field.
   *
   * @param string $field
   *   The name of the field to return.
   * @return KiwiEmuField
   */
  public function getField($field) {
    return $this->fields[$field];
  }

  /**
   * Returns the iterator for this table.
   *
   * Iterating the table iterates over all of the fields it encompasses.
   *
   * @see IteratorAggregate
   * @return ArrayIterator
   */
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

  /**
   * Constructor.
   *
   * @param KiwiConfiguration $config
   *   The configuration object for this run.
   * @param KiwiImuSession $session
   *   The Emu connection object we are processing.
   * @param <type> $table
   * @param array $schema
   *   The schema definition for this field.
   */
  public function __construct(KiwiConfiguration $config, KiwiImuSession $session, $table, $schema) {
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
    return ($this->schema['DataKind'] == 'dkTable');
    //return (substr($this->schema['ColumnName'], -4) == '_tab');
  }

  /**
   * Returns the item name of the field.
   *
   * The item name is not always the same as the columnName. It may be missing
   * Hungarian suffixes like _tab.
   *
   * @return string
   *   The item name for this field.
   */
  public function itemName() {
    return $this->schema['ItemName'];
  }

  /**
   * Returns the column name of the field.
   *
   * The column name is the physical name within Emu.
   *
   * @return string
   */
  public function columnName() {
    return $this->schema['ColumnName'];
  }

  /**
   * Returns True if this field is an attachment field, False otherwise.
   *
   * Note that for our purposes, attachment fields are only single-value.
   * Multi-value reference fields we call something else.
   *
   * @return boolean
   */
  public function isAttachmentField() {
    return ($this->schema['DataKind'] == 'dkAtom' && substr($this->schema['ColumnName'], -3) == 'Ref');
  }

  /**
   * Returns True if this field is a Merge field, False otherwise.
   *
   * @return boolean
   */
  public function isMergeField() {
    return ($this->isMultiValue() && substr($this->schema['ColumnName'], -7) == 'Ref_tab');
  }

  /**
   * Returns the name of the table referenced by this field, if any.
   *
   * @return string
   *   The name of the table this field references, or empty string if it
   *   is not a reference field.
   */
  public function refTable() {
    return !empty($this->schema['RefTable']) ? $this->schema['RefTable'] : '';
  }

  /**
   * Returns the data type for this field.
   *
   * The data type is the type of field, such as Integer, Float, etc.
   *
   * @return string
   */
  public function dataType() {
    return $this->schema['DataType'];
  }

  /**
   * Returns a mapping handler object for the this field.
   *
   * @return FieldHandlerInterface
   */
  public function getHandler() {
    // If it's an attachment field, override the type since Emu files them
    // as integers.
    $data_type = $this->isAttachmentField() ? 'Attachment' : $this->dataType();

    $field_options = $this->config->getFieldOptions($this->table, $this->columnName(), $data_type);
    $class = $field_options['handler'];
    //debug($this->itemName() . ' maps to ' . $class);
    return $this->session->getHandler($class);
  }

  /**
   * Returns whether or not this field should be included in fulltext searches.
   *
   * @return boolean
   */
  public function indexFulltext() {
    return in_array($this->columnName(), $this->config->getFields($this->table, 'fulltext'));
  }

  /**
   * Returns whether or not this field should be sortable.
   *
   * @return boolean
   */
  public function isSortable() {
    return in_array($this->columnName(), $this->config->getFields($this->table, 'sort'));
  }

  public function  __toString() {
    return print_r($this->schema, 1);
  }
}
