<?php

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
      fwrite($this->getStream($level), $this->formatMessage($message, $level) . PHP_EOL);
    }
    return $this;
  }

  /**
   * Writes a message to standard out, without any prefixing.
   *
   * This is mostly useful for help text and similar instructions.
   *
   * @param string $message
   *   The message to print.
   */
  public function write($message) {
    fwrite($this->getStream(LOG_INFO), $message);
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
    // Since this is frequently called from within a long loop, bail out quickly
    // and save several function calls.
    $instance = self::get();
    if (LOG_DEBUG > $instance->threshold) {
      return $instance;
    }
    $message = print_r($message, 1);
    if ($label) {
      $message = $label . ': ' . $message;
    }
    return $instance->writeMessage($message, LOG_DEBUG);
  }

  /**
   * Writes an INFO level message.
   *
   * @param string $message
   *   The message to record.
   * @return KiwiOutput
   *   The active output object for chaining.
   */
  public static function warn($message) {
    return self::get()->writeMessage($message, LOG_WARNING);
  }
}
