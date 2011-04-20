<?php

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


class KiwiInput {

  /**
   * Command line arguments provided to our script.
   *
   * @var array
   */
  protected $argv;

  /**
   * The number of elements in $this->argv.
   *
   * @var int
   */
  protected $argc;

  /**
   * The name of the configuration file to use for this run.
   *
   * @var string
   */
  protected $configFileName;

  public function defaultOptions() {
    $options['config'] = array(
      'value' => 'config.xml',
      'description' => 'The configuration file to process.  If a path is specified, it will be interpreted relative to the current working directory.',
    );
    $options['debug'] = array(
      'value' => LOG_INFO,
      'description' => 'The level of output verbosity. To increase it, specify -v, -vv, -vvv, etc. to increase the verbosity.',
    );


    return $options;
  }

  public function parse() {
    $this->argv = $_SERVER['argv'];
    $this->argc = $_SERVER['argc'];

    $options = $this->defaultOptions();

    $matches = array();
    foreach ($this->argv as $argument) {
      // Match long-form options.
      // This regex matches --<word>=<word allowing periods for file names>.
      if (preg_match('/--(\w+)=([\w\.]+)/', $argument, $matches)) {
        if (in_array($matches[1], array_keys($options))) {
          $options[$matches[1]] = $matches[2];
        }
        else {
          throw new InvalidConfigOptionException("{$matches[1]} is not a valid configuration directive.");
        }
      }

      // Match short-form options, if any.
      // @TODO: Implement this, I guess. :-)

      // Handle boolean options and other flags.
      if (preg_match('/-(v+)/', $argument, $matches)) {
        $chars = count_chars($matches[1], 1);
        $options['debug'] += $chars[ord('v')];
      }

    }

    debug($options);

    // Save the options, including defaults, to the object.
    $this->options = $options;
  }

  public function getOption($option, $default = '') {
    return isset($this->options[$option]) ? $this->options[$option] : $default;
  }

  public function getInstructions() {
    $output = array();

    $total_width = 80;
    $columns['option'] = 10;
    $columns['default'] = 15;
    $columns['description'] = $total_width - $columns['option'] - $columns['default'];

    $output[] = sprintf("%-{$columns['option']}s%-{$columns['default']}s%-{$columns['description']}s", 'Option', 'Default value', 'Description');
    $output[] = str_repeat('-', $total_width);

    $break = PHP_EOL . str_repeat(' ', $columns['option'] + $columns['default']);

    foreach ($this->defaultOptions() as $option => $info) {
      $output[] = sprintf("%-{$columns['option']}s%-{$columns['default']}s%-{$columns['description']}s", $option, $info['value'], wordwrap($info['description'], $columns['description'], $break, TRUE));
    }

    return implode(PHP_EOL, $output);
  }

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
 * Output manager for Kiwi.
 *
 * This class uses the error reporting constants defined in PHP core, as
 * conformant with RFC 3164, section 4.1.1:
 *
 * LOG_EMERG
 * LOG_ALERT
 * LOG_CRIT
 * LOG_ERR
 * LOG_WARNING
 * LOG_NOTICE
 * LOG_INFO
 * LOG_DEBUG
 *
 * It also adds one additional constant, LOG_SILENT, which has a lower value
 * than all of them.  That allows all output to be disabled by setting that
 * as the threshold value.
 *
 */
class KiwiOutput {

  /**
   * A singleton instance of this class.
   *
   * @var KiwiOutput
   */
  protected static $instance;

  /**
   * Static list of human-readable names for each message level.
   *
   * @var array
   */
  protected static $levelPrefixes = array(
    LOG_EMERG => 'Emergency',
    LOG_ALERT => 'Alert',
    LOG_CRIT => 'Critical',
    LOG_ERR => 'Error',
    LOG_WARNING => 'Warning',
    LOG_NOTICE => 'Notice',
    LOG_INFO => 'Info',
    LOG_DEBUG => 'Debug'
  );

  /**
   * The default stream to use for any output.
   *
   * @var Stream
   */
  protected $defaultStream;

  /**
   * Streams to use for various message levels.
   *
   * @var array
   */
  protected $streams = array();

  /**
   * The threshold for messages.
   *
   * Messages will be printed iff they have this level or higher.
   *
   * @var int
   */
  protected $threshold = LOG_INFO;

  /**
   * Constructor.
   */
  protected function __construct() {
    $this->defaultStream = STDOUT;

    //$stderr = fopen('php://stderr', 'w');
    $this->streams[LOG_NOTICE] = STDERR;
    $this->streams[LOG_WARNING] = STDERR;
    $this->streams[LOG_ERR] = STDERR;
    $this->streams[LOG_CRIT] = STDERR;
    $this->streams[LOG_ALERT] = STDERR;
    $this->streams[LOG_EMERG] = STDERR;
  }

  /**
   * Returns the appropriate I/O stream object for the specified level.
   *
   * @param int $level
   *   One of the defined message levels.
   * @return Stream
   */
  protected function getStream($level) {
    return (!empty($this->streams[$level])) ? $this->streams[$level] : $this->defaultStream;
  }

  /**
   * Writes a message out to the appropriate output.
   *
   * If the level of the message is not at least the current threshold, it will
   * not be written.
   *
   * @param string $string
   *   The message to record.
   * @param int $level
   *   The level of the message we are writing.
   * @return KiwiOutput
   *   The called object.
   */
  public function writeMessage($message, $level = LOG_INFO) {
    if ($level <= $this->threshold) {
      fwrite($this->getStream($level), $this->formatMessage($message, $level). PHP_EOL);
    }
    return $this;
  }

  /**
   * Formats a message for display to the user.
   *
   * @param string $message
   *   The message to report to the user.
   * @param int $level
   *   The error level of the message.
   * @return string
   *   The formatted message string.
   */
  protected function formatMessage($message, $level) {
    return '[' . self::$levelPrefixes[$level] . '] ' . $message;
  }

  /**
   * Sets the minimum error level threshold.
   *
   * Any messages of a lower level will be ignored in subsequent calls.
   *
   * @param int $level
   *   The minimum error level we want printed.
   * @return KiwiOutput
   *   The called object.
   */
  public function setThreshold($level = LOG_INFO) {
    $this->threshold = $level;
    return $this;
  }

  /**
   * Returns a singleton instance of this clas.
   *
   * @return KiwiOutput
   */
  static public function get() {
    if (! self::$instance) {
      self::$instance = new self();
    }
    return self::$instance;
  }

  /**
   * Writes an INFO level message.
   *
   * @param string $string
   *   The message to record.
   * @return KiwiOutput
   *   The active output object for chaining.
   */
  public static function info($message) {
    return self::get()->writeMessage($message, LOG_INFO);
  }

  /**
   * Writes a DEBUG level message.
   *
   * This
   *
   * @param mixed $string
   *   The message to record.  Alternatively, a non-string value (array or object)
   *   that will first be serialized for display.  That makes it useful for 
   *   all sorts of debugging.
   * @param string $label
   *   An optional title for this message.
   * @return KiwiOutput
   *   The active output object for chaining.
   */
  public static function debug($message, $label = '') {
    $message = print_r($message, 1);
    if ($label) {
      $message = $label . ': ' . $message;
    }
    return self::get()->writeMessage($message, LOG_DEBUG);
  }

  public static function warn($message) {
    self::get()->writeMessage($message, LOG_WARNING);
  }
}

