<?php

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
   * The name of the mapping class for this field.
   *
   * @var string
   */
  protected $handlerClass;

  /**
   *
   * @var type
   */
  protected $dataType;

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

    // Precache this information, as it will never change.  Then we can always
    // use it just as a property on this object.
    $this->dataType = $this->dataType();
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
    return (substr($this->schema['ColumnName'], -7) == 'Ref_tab' && $this->isMultiValue());
  }

  /**
   * Returns True if htis field is a Nested table field, False otherwise.
   *
   * @return boolean
   */
  public function isNesttabField() {
    return ($this->schema['DataKind'] == 'dkNested');
  }

  /**
   * Returns True if this field is a reference to another module/table.
   *
   * There are multiple kinds of reference field, as far as we're concerned, but
   * this method will return true for all of them.
   *
   * @return boolean
   */
  public function isReferenceField() {
    return $this->isMergeField() || $this->isAttachmentField();
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
    // Reference fields are stored as integers, so we need to override the type
    // on those.
    if ($this->isMergeField()) {
      $data_type = 'Merge';
    }
    else if ($this->isAttachmentField()) {
      $data_type = 'Attachment';
    }
    else if ($this->isNesttabField()) {
      $data_type = 'Nesttab';
    }
    else {
      $data_type = $this->schema['DataType'];
    }

    return $data_type;
  }

  /**
   * Returns a mapping handler object for the this field.
   *
   * @return FieldHandlerInterface
   */
  public function getHandler() {
    if (empty($this->handlerClass)) {
      $field_options = $this->config->getFieldOptions($this->table, $this->columnName(), $this->dataType);
      $this->handlerClass = $field_options['handler'];
    }

    //debug($this->itemName() . ' maps to ' . $class);
    return $this->session->getHandler($this->handlerClass);
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
