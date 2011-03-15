<?php

/**
 * Configuration object for a Kiwi execution run.
 */
class KiwiConfiguration {

  /**
   * The QueryPath representation of the configuration file, for easier parsing.
   *
   * @var QueryPath
   */
  protected $configFile;

  /**
   * Constructor.
   *
   * @param string $file
   *   The name of the file with the configuration to load.
   */
  public function __construct($file = '') {

    if ($file && file_exists($file)) {

      // Parse the configuration file using QueryPath for easier manipulation.
      $this->configFile = qp($file);

      // Resolve XInclude directives so that config files can be broken up for
      // easier reuse.
      $this->configFile->get(0)->ownerDocument->xinclude();
    }
    else {
      throw new Exception('No configuration file found.');
    }
  }

  /**
   * Returns general information about this configuration.
   *
   * @return array
   */
  public function getConfigInfo() {
    return $this->configFile->branch()->find('config')->attr() + array(
      'name' => '',
      'full-rebuild' => 0,
    );
  }

  /**
   * Returns the connection information for the Emu server.
   *
   * @return array
   */
  public function getSolrInfo() {
    return $this->configFile->branch()->find('solr')->attr() + array(
      'host' => '',
      'port' => '',
      'path' => '',
    );
  }

  /**
   * Returns the connection information for the Solr server.
   *
   * @return array
   */
  public function getEmuInfo() {
    return $this->configFile->branch()->find('emu')->attr() + array(
      'host' => 'localhost',
      'port' => 40000,
      'reconnect-port' => 45000,
    );
  }

  public function baseTable() {

    $base_table = $this->configFile->branch()->find('base-table')->attr('name');

    return array(
      'name' => $base_table,
    );
  }

  /**
   * Returns the names of all tables of the specified type.
   *
   * @param string $type
   *   The type of table to retrieve. Valid values are attach, merge, collapse.
   * @return array
   */
  public function getTables($type) {
    $names = array();
    $tables = $this->configFile->branch()->find("table[type='{$type}']");
    foreach ($tables as $table) {
      $names[] = $table->attr('name');
    }
    return $names;
  }

  /**
   * Returns the number of child processes we want to spawn for this batch.
   *
   * @return int
   *   The number of child processes we want to spawn.
   */
  public function numChildProcesses() {
    return $this->configFile->branch()->find('processors')->attr('count');
  }

  /**
   * Returns the number of records to process into Solr documents at once.
   *
   * Solr supports sending multiple documents to it in a single command,
   * so this is another performance dial to twiddle.
   *
   * @return int
   */
  public function numDocumentsPerBatch() {
    return $this->configFile->branch()->find('processors')->attr('batch-size');
  }

  /**
   * Returns the maximum number of records each processor will process.
   *
   * This is mostly for debug purposes, but allows a maximum size to be enforced
   * on an index.
   *
   * @return int
   */
  public function maxRecordsPerProcessor() {
    $max = $this->configFile->branch()->find('processors')->attr('max-size');
    if (!$max) {
      $max = PHP_INT_MAX;
    }
    return $max;
  }

  /**
   * Returns a list of field directives for the specified table.
   *
   * @param string $table
   *   The name of the table for which we want to get a field list.
   * @param string $use
   *   The use of the field, e.g., sortable, displayable, etc.  If set, only
   *   those fields that have the appropriate attribute set to 1 will be
   *   returned.  Valid values are:
   *     - indexed: The field should be searchable from Solr.
   *     - stored: The field will be retrievable from Solr for display.
   *     - fulltext: The field should be included in fulltext searches.
   *     - sort: The field is sortable in Solr.
   *     - all (default): Do not filter at all; return all fields for this table.
   * @return QuerPath
   */
  public function getFields($table, $use = 'all') {
    $names = array();
    $fields = $this->configFile->branch()->find("table[name='{$table}'] field");
    if ($use != 'all') {
      $fields->filter("[{$use}=1]");
    }
    return $fields;
  }

  /**
   * Returns a list of filter directives for the base query.
   *
   * @return QueryPath
   */
  public function getFilters() {
    return $this->configFile->branch()->find("filters filter");
  }

  /**
   * Returns the name of the handler class for the specified field.
   *
   * @param string $table
   *   The table to which the field belongs.
   * @param string $field
   *   The field for which we want to know the handler class.
   * @param string $type
   *   The type of the field in question.
   *
   * @return string
   *   The name of the handler class for this field.
   */
  public function getFieldHandler($table, $field, $type) {
    $handler = $this->configFile->branch()->find("table[name='{$table}'] field[name={$field}]")->attr('handler');
    if (!$handler) {
      $handler = $this->configFile->branch()->find("field-types field-type[name={$type}]")->attr('handler');
    }
    if (!$handler) {
      $handler = $this->configFile->branch()->find("field-types")->attr('default-handler');
    }
    return $handler;
  }

  /**
   * Returns the configuration for a specific field.
   *
   * @param string $table
   *   The table to which the field belongs.
   * @param string $field
   *   The field for which we want configuration data.
   * @param string $type
   *   The type of the field in question.
   *
   * @return array
   *   An array of all properties relevant to the specified field.
   */
  public function getFieldOptions($table, $field, $type) {
    // The specific field's definition:
    $info = $this->configFile->branch()->find("table[name='{$table}'] field[name={$field}]")->attr();

    // Use any values defined for that field type as defaults.
    $info += $this->configFile->branch()->find("field-types field-type[name={$type}]")->attr();

    // The handler specifically has an extra default specification.
    if (!$info['handler']) {
      $info['handler'] = $this->configFile->branch()->find("field-types")->attr('default-handler');
    }

    // And now hard-coded defaults:
    $info += array(
      'handler' => 'NullHandler',
      'indexed' => 0,
      'stored' => 0,
      'fulltext' => 0,
      'sorted' => 0,
    );

    return $info;
  }
}
