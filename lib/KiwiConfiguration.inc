<?php

/**
 * Configuration object for a Kiwi execution run.
 */
class KiwiConfiguration {

  /**
   * Constructor.
   *
   * @param string $file
   *   The name of the file with the configuration to load.
   */
  function __construct($file = '') {

    if ($file && file_exists($file)) {

      

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
    return array(
      'name' => 'ecatalogue',
    );
  }

}
