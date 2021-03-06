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
 * Generator class for a processing run.
 *
 * This object will only create a result set on the Emu server and return its
 * ID.  It will not do any processing of that result set.
 *
 * @see KiwiQueryProcessor
 */
class KiwiQueryGenerator {

  /**
   * The configuration object for this generator.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The ImuSession object on which to run queries.
   *
   * @var KiwiImuSession
   */
  protected $session;

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session) {
    $this->config = $config;
    $this->session = $session;
  }

  public function run() {

    $module = $this->session->newModuleHandler($this->config->baseTable(), FALSE);

    // Run initial query as defined by the config object.
    $terms = array();
    foreach ($this->config->getFilters() as $filter) {
      $term = array($filter->attr('name'), $filter->attr('value'));
      if ($operator = $filter->attr('operator')) {
        $term[] = $operator;
      }
      $terms[] = $term;
    }

    KiwiOutput::debug($terms, 'Query terms');

    $module->findTerms(array('and', $terms));

    // Return the module ID
    return $module->id;
  }
}
