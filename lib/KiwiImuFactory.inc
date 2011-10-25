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
 * Factory generator for all Emu connection objects.
 *
 * There are a myriad of ways to connect to Emu, so it's eaiser to handle them
 * in a factory rather than via constructors.
 */
class KiwiImuFactory {

  /**
   * The configuration object for this session.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * Constructor.
   *
   * @param KiwiConfiguration $config
   */
  public function __construct(KiwiConfiguration $config) {

    $this->config = $config;
  }

  /**
   * Returns a new Emu session object.
   *
   * @param boolean $suspend
   *   Whether or not to suspend the session object when we're done with it.
   *   A suspended session may be reconnected to later in another process.
   * @return KiwiImuSession
   */
  public function getNewEmuSession($suspend = FALSE) {
    $server_info = $this->config->getEmuInfo();
    $session = new KiwiImuSession($this->config);

    // Depending on the Emu configuration, we may need to authenticate against
    // the user account.  If not, though, calling login() when we don't need to
    // can cause the Emu process to hang.  We therefore only try to authenticate
    // if a username and password were provided in the configuration.
    if ($server_info['user'] && $server_info['password']) {
      $session->login($server_info['user'], $server_info['password']);
    }

    $session->connect();

    $session->suspend = $suspend;

    return $session;
  }

  /**
   * Resumes and returns an Emu session.
   *
   * @param string $context
   *   The context ID of the session to which we are reconnecting.
   * @param int $port
   *   The TCP port on which we need to reconnect.  This is different for every
   *   connection and the original session object will tell us what port to use
   *   to reconnect to it.
   * @return KiwiImuSession
   */
  public function resumeEmuSession($context, $port) {
    $server_info = $this->config->getEmuInfo();
    $session = new KiwiImuSession($this->config, FALSE, $port);

    $session->context = $context;

    return $session;
  }
}
