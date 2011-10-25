<?php

/*
* This file is part of the Kiwi package.
*
* (c) Palantir.net <http://www.palantir.net/>
*
* For the full copyright and license information, please view the LICENSE.txt
* file that was distributed with this source code.
*/

// Not part of RFC 3164, but useful for disabling output.
define('LOG_SILENT', -1);

function debug($message, $label = '') {
  $message = print_r($message, 1);
  if ($label) {
    $message = $label . ': ' . $message;
  }
  $message .= PHP_EOL;

  print $message;
}

/**
 * Recursively implodes an array.
 *
 * @see http://www.php.net/manual/en/function.implode.php#96100
 * @param string $glue
 *   The value to append between each piece.
 * @param array $pieces
 *   The array of strings to implode.
 * @return string
 */
function recursive_implode($glue, $pieces) {
  $ret_val = array();
  foreach($pieces as $r_pieces) {
    if (is_array($r_pieces)) {
      $ret_val[] = recursive_implode($glue, $r_pieces);
    }
    else {
      $ret_val[] = $r_pieces;
    }
  }
  return implode($glue, $ret_val);
}


/**
 * Returns the call stack for the specified exception.
 *
 * The normal getTrace() method of an exception includes all arguments, which
 * when passing objects around results in uselessly large variables.  It also
 * mismatches line numbers due to stupidity in PHP's backtrace logic.  This
 * utility function works around both issues and returns a formatted
 * string of the call stack, without the arguments or other flotsam.
 *
 * @param Exception $e
 *   The exception whose stack we want to decode.
 * @return string
 */
function get_stack(Exception $e) {

  $trace = array();
  $return = array();

  $stack = $e->getTrace();

  for ($i = 0, $stack_count = count($stack); $i < $stack_count; ++$i) {
    $line = array(
      'file' => isset($stack[$i]['file']) ? $stack[$i]['file'] : NULL,
      'line' => isset($stack[$i]['line']) ? $stack[$i]['line'] : NULL,
      'function' => isset($stack[$i + 1]['function']) ? $stack[$i + 1]['function'] : NULL,
      'class' => isset($stack[$i + 1]['class']) ? $stack[$i + 1]['class'] : NULL,
      'type' => isset($stack[$i + 1]['type']) ? $stack[$i + 1]['type'] : NULL,
    );
    $line['stack_call'] = (!empty($line['class']) ? $line['class'] . '::' : '') . $line['function'];
    $trace[] = $line;
  }

  foreach ($trace as $call) {
    $return[] = sprintf('%-30s in %s line %s', $call['stack_call'], $call['file'], $call['line']);
  }

  return implode(PHP_EOL, $return);
}


/**
 * Exception for a missing Kiwi configuration file.
 */
class ConfigFileNotFoundException extends UnexpectedValueException {}

/**
 * Exception for an invalid and unexpected command line switch.
 */
class InvalidConfigOptionException extends UnexpectedValueException {}

/**
 * Exception for a missing required command line switch.
 */
class MissingConfigOptionException extends UnexpectedValueException {}

/**
 * Exception for connection errors from Emu.
 */
class KiwiEmuConnectionException extends Exception {}

/**
 * Exception for a missing Field in the schema.
 */
class KiwiEmuFieldNotFoundException extends Exception {}
