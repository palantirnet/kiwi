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
  function __construct($file = '') {

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
   * Returns the connection information for the Emu server.
   *
   * @return array
   */
  function getSolrInfo() {
    $emu = $this->configFile->branch()->find('solr');
    return array(
      'host' => $emu->attr('host'),
      'port' => $emu->attr('port'),
    );
  }

  /**
   * Returns the connection information for the Solr server.
   *
   * @return array
   */
  function getEmuInfo() {
    $emu = $this->configFile->branch()->find('emu');
    return array(
      'host' => $emu->attr('host'),
      'port' => $emu->attr('port'),
      'reconnect-port' => $emu->attr('reconnect-port'),
    );
  }

  function baseTable() {

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
  function getTables($type) {
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
  function numChildProcesses() {
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
  function numDocumentsPerBatch() {
    return $this->configFile->branch()->find('processors')->attr('batch-size');
  }

  function getFields($table, $use = 'all') {
    $names = array();
    $fields = $this->configFile->branch()->find("table[name='{$table}'] field");
    if ($use != 'all') {
      $fields->filter("[{$use}=1]");
    }
    return $fields;
    /*
    foreach ($fields as $field) {
      $names[] = $field->attr('name');
    }
    return $names;
    */
  }
}
