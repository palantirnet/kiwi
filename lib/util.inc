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
    $this->defaultStream = fopen('php://stdout', 'w');
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
   */
  public function writeMessage($message, $level = LOG_INFO) {
    if ($level <= $this->threshold) {
      fwrite($this->getStream($level), $message . PHP_EOL);
    }
  }

  /**
   * Sets the minimum error level threshold.
   *
   * Any messages of a lower level will be ignored in subsequent calls.
   *
   * @param int $level
   *   The minimum error level we want printed.
   */
  public function setThreshold($level = LOG_INFO) {
    $this->threshold = $level;
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
   */
  public static function info($message) {
    self::get()->writeMessage($message, LOG_INFO);
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
   */
  public static function debug($message, $label = '') {
    $message = print_r($message, 1);
    if ($label) {
      $message = $label . ': ' . $message;
    }
    self::get()->writeMessage($message, LOG_DEBUG);
  }
}

