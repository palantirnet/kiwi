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
}
