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
      $this->configFile = qp($file);
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
  function getEmuInfo() {
    return array(
      'host' => '66.158.71.71',
      'port' => 40000,
    );
  }

  function baseTable() {

    $base_table = $this->configFile->find('base-table')->attr('name');

    return array(
      'name' => $base_table,
    );
  }

}
