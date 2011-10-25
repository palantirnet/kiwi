#!/usr/bin/php
<?php

/*
* This file is part of the Kiwi package.
*
* (c) Palantir.net <http://www.palantir.net/>
*
* For the full copyright and license information, please view the LICENSE.txt
* file that was distributed with this source code.
*/

require_once('lib/bootstrap.inc');

kiwi_init();

/**
 * Initialization driver for the Kiwi script.
 */
function kiwi() {
  try {
    $input = new KiwiInput();
    $input->parse();

    // Allow for parsing of an entire directory or just an individual file.
    if ($input->getOption('help')) {
      KiwiOutput::get()->write(PHP_EOL . $input->getInstructions());
      KiwiOutput::get()->write(PHP_EOL . $input->getExtendedHelp());
    }
    else if ($directory = $input->getOption('directory')) {
      foreach (glob($directory . '*.xml') as $file) {
        KiwiOutput::info("Parsing {$file} for processing.\n");
        main($input, new KiwiConfiguration($input, $file));
      }
    }
    else {
      main($input);
    }
  }
  catch (InvalidConfigOptionException $e) {
    KiwiOutput::get()->writeMessage($e->getMessage(), LOG_ERR);
    KiwiOutput::get()->write(PHP_EOL . $input->getInstructions());
  }
  catch (MissingConfigOptionException $e) {
    KiwiOutput::get()->write(PHP_EOL . $input->getInstructions());
  }
}

kiwi();
exit();
