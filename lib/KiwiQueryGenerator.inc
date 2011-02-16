<?php

class KiwiQueryGenerator {

  /**
   * The configuration object for this generator.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  public function __construct(KiwiConfiguration $config) {
    $this->config = $config;
  }

  public function run() {
    // Connect to Emu server.

    // Run initial query as defined by the config object.
    // This may require a query builder.

    // Return the module ID
  }

}
