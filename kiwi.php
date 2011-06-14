#!/usr/bin/php
<?php

require_once('lib/bootstrap.inc');

kiwi_init();

/**
 * Initialization driver for the Kiwi script.
 */
function kiwi() {
  $input = new KiwiInput();
  $input->parse();

  // Allow for parsing of an entire directory or just an individual file.
  if ($directory = $input->getOption('directory')) {
    foreach (glob($directory . '/*.xml') as $file) {
      KiwiOutput::info("Parsing {$file} for processing.\n");
      main($input, new KiwiConfiguration($input, $file));
    }
  }
  else {
    main($input);
  }
}

kiwi();
exit();
