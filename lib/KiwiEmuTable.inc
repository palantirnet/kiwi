<?php

/**
 * Wrapper object representing a table in Emu.
 */
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
      if (isset($this->schema['columns'][$field_name])) {
        $this->fields[$field_name] = new KiwiEmuField($this->config, $this->session, $this->getName(), $this->schema['columns'][$field_name]);
      }
      else {
        KiwiOutput::get()->writeMessage("Field '{$field_name}' not found in schema for table '{$this->schema['table']}'", LOG_WARNING);
      }
    }
  }

  /**
   * Returns the name of the table.
   *
   * @return string
   */
  public function getName() {
    return $this->schema['table'];
  }

  /**
   * Returns a Field object for the specified field.
   *
   * @param string $field
   *   The name of the field to return.
   * @return KiwiEmuField
   */
  public function getField($field) {
    if (!isset($this->fields[$field])) {
      throw new KiwiEmuFieldNotFoundException("Field {$field} not found in schema for table " . $this->getName());
    }
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
