<?php

/*
* This file is part of the Kiwi package.
*
* (c) Palantir.net <http://www.palantir.net/>
*
* For the full copyright and license information, please view the LICENSE.txt
* file that was distributed with this source code.
*/

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
   * Cache of field options from the config file.
   *
   * @var array
   */
  protected $fieldOptions;

  /**
   * Cache of table names from the config file.
   *
   * @var array
   */
  protected $tableNames;

  /**
   * Cache of field info from the config file.
   *
   * @var array
   */
  protected $fieldTableInfo;

  /**
   * The input options from the command line.
   *
   * @var KiwiInput
   */
  protected $input;

  /**
   * Information about the processors to use for this run.
   *
   * @var array
   */
  protected $processorInformation;

  /**
   * The QueryPath fragment for the filters to apply to the query.
   *
   * @var QueryPath
   */
  protected $filters;

  /**
   * Constructor.
   *
   * @param KiwiInput $input
   *   The input object from which this configuration should operate.
   * @param string $file
   *   The name of the file with the configuration to load.  If not specified
   *   it wll be derived from the input object.
   */
  public function __construct(KiwiInput $input, $file = '') {
    $this->input = $input;

    if (empty($file)) {
      $file = $this->input->getOption('config');
    }

    if ($file && file_exists($file)) {

      // Parse the configuration file using QueryPath for easier manipulation.
      $this->configFile = qp($file);

      // Resolve XInclude directives so that config files can be broken up for
      // easier reuse.
      $this->configFile->xinclude();
    }
    else {
      throw new ConfigFileNotFoundException('Configuration file not found: ' . $file);
    }
  }

  /**
   * Returns the default verbosity level of the system.
   */
  public function defaultVerbosity() {
    return $this->input->getOption('verbosity');
  }

  /**
   * Returns general information about this configuration.
   *
   * @return array
   */
  public function getConfigInfo() {
    return $this->configFile->branch()->top()->attr() + array(
      'name' => '',
      'full-rebuild' => 0,
      'debug' => LOG_INFO,
    );
  }

  /**
   * Returns the connection information for the Solr server.
   *
   * @return array
   */
  public function getSolrInfo() {
    return $this->configFile->branch()->find('> servers > solr')->attr() + array(
      'host' => '',
      'port' => '',
      'path' => '',
    );
  }

  /**
   * Returns the connection information for the Emu server.
   *
   * @return array
   */
  public function getEmuInfo() {
    return $this->configFile->branch()->find('> servers > emu')->attr() + array(
      'host' => 'localhost',
      'port' => 40000,
      'user' => '',
      'password' => '',
    );
  }

  public function baseTable() {
    $base_table = $this->configFile->branch()->find('config')->attr('base-table');

    return array(
      'name' => $base_table,
    );
  }

  /**
   * Returns the names of all tables in the configuration set.
   *
   * @return array
   */
  public function getTables() {
    if (empty($this->tableNames)) {
      $names = array();
      $tables = $this->configFile->branch()->find("> table");
      foreach ($tables as $table) {
        $names[] = $table->attr('name');
      }
      $this->tableNames = $names;
    }
    return $this->tableNames;
  }

  /**
   * Returns various information about the processors that should run.
   *
   * @param string $property
   *   The property to request.  Legal values are:
   *   - count: The number of child processors to create.
   *   - batch-size: The number of records to process into Solr documents at once.
   *   - max-size: The number of records after which to stop processing.
   * @return mixed
   *   The value of the specified property.
   */
  public function processorInfo($property) {
    if (empty($this->processorInformation[$property])) {
      $value = $this->input->getOption('processor-' . $property);
      if (!$value) {
        $value = $this->configFile->branch()->find('> processors')->attr($property);
      }
      if ($property == 'max-size' && !$value) {
        $value = PHP_INT_MAX;
      }
      $this->processorInformation[$property] = $value;
    }
    return $this->processorInformation[$property];
  }

  /**
   * Returns a list of field directives for the specified table.
   *
   * @param string $table
   *   The name of the table for which we want to get a field list.
   * @return QueryPath
   */
  public function getFields($table) {
    if (empty($this->fieldTableInfo[$table])) {
      $this->fieldTableInfo[$table] = array();
      $names = array();
      $fields = $this->configFile->branch()->find("> table[name='{$table}'] field");
      foreach ($fields as $field) {
        $this->fieldTableInfo[$table][] = $field->attr('name');
      }
    }
    return $this->fieldTableInfo[$table];
  }

  /**
   * Returns a list of filter directives for the base query.
   *
   * @return QueryPath
   */
  public function getFilters() {
    if (empty($this->filters)) {
      $this->filters = $this->configFile->branch()->find("> filters > filter");
    }
    return $this->filters;
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
    if (empty($this->fieldOptions[$table][$field])) {
      // The specific field's definition:
      $info = $this->configFile->branch()->find("> table[name='{$table}'] > field[name={$field}]")->attr();

      // Use any values defined for that field type as defaults.
      $info += $this->configFile->branch()->find("> field-types > field-type[name={$type}]")->attr();

      // The handler specifically has an extra default specification.
      if (!$info['handler']) {
        $info['handler'] = $this->configFile->branch()->find("> field-types")->attr('default-handler');
      }

      // And now hard-coded defaults:
      $info += array(
        'handler' => 'NullHandler',
        'fulltext' => 0,
        'sort' => 0,
      );

      $this->fieldOptions[$table][$field] = $info;
    }

    return $this->fieldOptions[$table][$field];
  }
}
