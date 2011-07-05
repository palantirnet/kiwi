<?php

/**
 * This object acts as a gateway to all incoming user configuration directives.
 */
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

  /**
   * The width in characters of the argument instructions.
   *
   * @var int
   */
  protected static $instructionWidth = 80;

  /**
   * Constructor.
   */
  public function __construct() {
    // If we are able to detect the width of the screen, set our output to that
    // size.  If not, just stick with the default.
    // Note: If you want to have this value be available, you must use "export COLUMNS"
    // before running this program.  Putting that in your bashrc file or similar
    // is recommended.
    if ($cols = getenv('COLUMNS')) {
      self::$instructionWidth = $cols;
    }
  }

  public function defaultOptions() {
    $options['config'] = array(
      'value' => '',
      'description' => 'The configuration file to process.  If a path is specified, it will be interpreted relative to the current working directory. Either this directive or --directory must be specified.',
    );
    $options['directory'] = array(
      'value' => '',
      'description' => 'A directory of files to process.  All .xml files in the specified directory will be processed sequentially. Either this directive or --config must be specified.  If specified, this value overrides the --config directive.',
    );
    $options['verbosity'] = array(
      'value' => LOG_INFO,
      'description' => 'The level of output verbosity. To increase it, specify -v, -vv, -vvv, etc. to increase the verbosity.',
    );
    $options['processor-count'] = array(
      'value' => '',
      'description' => 'Optional.  Specifies the number of processors to run at once.  If specified, the value in the configuration file is ignored.',
    );
    $options['processor-batch-size'] = array(
      'value' => '',
      'description' => 'Optional.  Specifies the number of records to process at once.  More is generally faster but leads to higher memory usage.  If specified, the value in the configuration file is ignored.',
    );
    $options['processor-max-size'] = array(
      'value' => '',
      'description' => 'Optional.  Specifies the number of records after which each processor should stop processing.  If specified, the value in the configuration file is ignored.  If neither this directive nor the configuration file specify a limit, the default is the maximum interger size in PHP (several billion on most systems, so effectively unlimited).',
    );
    $options['help'] = array(
      'value' => 0,
      'description' => 'Show extended help information.',
    );

    return $options;
  }

  /**
   * Parse command line switches into a usable structure.
   *
   * We're not doing this in the constructor because it may throw exceptions,
   * but we still want the object around for debugging if so.  If a constructor
   * throws an exception, the object is not created.
   */
  public function parse() {
    // Store the original, unprocessed data.
    $this->argv = $_SERVER['argv'];
    $this->argc = $_SERVER['argc'];

    // Initialize our options.
    $options = $this->defaultOptions();

    $matches = array();
    foreach ($this->argv as $argument) {
      // Match value-less switches first, as that regex will match valued switches
      // too.  We let the later regex override the former where appropriate.

      // Match value-less switches.
      if (preg_match('/--([\w-]+)/', $argument, $matches)) {
        if (in_array($matches[1], array_keys($options))) {
          $options[$matches[1]]['value'] = 1;
        }
        else {
          throw new InvalidConfigOptionException("'{$matches[1]}' is not a valid configuration directive.");
        }
      }

      // Match long-form options.
      // This regex matches --<word>=<word allowing periods and forward slashes for file names>.
      if (preg_match('/--([\w-]+)=([\/\w\.]+)/', $argument, $matches)) {
        if (in_array($matches[1], array_keys($options))) {
          $options[$matches[1]]['value'] = $matches[2];
        }
        else {
          throw new InvalidConfigOptionException("'{$matches[1]}' is not a valid configuration directive.");
        }
      }

      // Match short-form options, if any.
      // @TODO: Implement this, I guess. :-)

      // Handle boolean options and other flags.
      if (preg_match('/-(v+)/', $argument, $matches)) {
        $chars = count_chars($matches[1], 1);
        $options['verbosity']['value'] += $chars[ord('v')];
      }

    }

    // Save the options, including defaults, to the object.
    $this->options = $options;

    if (!$this->getOption('help') && !$this->getOption('config') && !$this->getOption('directory')) {
      throw new MissingConfigOptionException('Unable to continue.  At least one of --config or --directory must be specified.');
    }
  }

  /**
   * Returns the requested option.
   *
   * Note: Default handling is not necessary as the parse() method ensures
   * that we always have a value, even if it's a default.
   *
   * @param string $option
   *   The name of the option to return. This should not have the leading --
   *   on it, although it must on the command line.
   * @return mixed
   *   The value of the specified option.
   */
  public function getOption($option) {
    return $this->options[$option]['value'];
  }

  /**
   * Returns the summary table for all available options and what they do.
   *
   * The returned string is ready for user-consumtion.
   *
   * @return string
   */
  public function getInstructions() {
    $output = array();

    $output[] = <<<END
example: kiwi.php --directory=config_files

END;

    $columns['option'] = 22;
    $columns['default'] = 10;
    $columns['description'] = self::$instructionWidth - $columns['option'] - $columns['default'];

    $output[] = sprintf("%-{$columns['option']}s%-{$columns['default']}s%-{$columns['description']}s", 'Option', 'Default', 'Description');
    $output[] = str_repeat('-', self::$instructionWidth);

    $break = PHP_EOL . str_repeat(' ', $columns['option'] + $columns['default']);

    foreach ($this->defaultOptions() as $option => $info) {
      $output[] = sprintf("%-{$columns['option']}s%-{$columns['default']}s%-{$columns['description']}s", $option, $info['value'], wordwrap($info['description'], $columns['description'], $break, TRUE));
    }

    return implode(PHP_EOL, $output);
  }

}
