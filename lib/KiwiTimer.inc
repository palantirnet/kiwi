<?php

/*
* This file is part of the Kiwi package.
*
* (c) Palantir.net <http://www.palantir.net/>
*
* For the full copyright and license information, please view the LICENSE.txt
* file that was distributed with this source code.
*/

/**
 * Timer class.
 *
 * This class is a simple wrapper for doing internal timing.  The timer starts
 * when the object is instantiated.  Calling stop() will return the total runtime
 * of the timer.
 */
class KiwiTimer {

  /**
   * The starting timestamp, in ms.
   *
   * @var float
   */
  protected $start;

  /**
   * Constructor.
   */
  public function __construct() {
    $this->start = microtime(TRUE);
  }

  /**
   * Stops the timer and returns the time the timer ran.
   *
   * @return float
   *   The runtime of the timer, in seconds, accurate to milliseconds.
   */
  public function stop() {
    return microtime(TRUE) - $this->start;
  }
}
