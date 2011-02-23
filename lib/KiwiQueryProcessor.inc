<?php

/**
 * Worker class for processing a result set.
 *
 * This class should be run in its own process, and each process should be
 * more or less self-contained in this class as much as possible.
 */
class KiwiQueryProcessor {

  /**
   * The configuration object for this processor.
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

  /**
   * The Solr server to which to add documents.
   *
   * @var Apache_Solr_Service
   */
  protected $solr;

  /**
   * The ID of this processor.
   *
   * @var int
   */
  protected $processorId;

  /**
   * The module object for the initial query.
   *
   * We will likely spawn other queries, but this represents the "main" result set.
   *
   * @var IMuModules
   */
  protected $module;

  /**
   * Constructor.
   *
   * @param int $processor_id
   *   The ID of this processor. It is a simple integer, unique within this
   *   script run only.
   * @param string $module_id
   *   The ID of the module on the Emu to which we want to reconnect.
   * @param KiwiConfiguration $config
   *   The configuration object for this processor.
   * @param KiwiImuSession $session
   *   The session on which to run all queries.
   */
  public function __construct($processor_id, $module_id, KiwiConfiguration $config, KiwiImuSession $session, Apache_Solr_Service $solr) {
    $this->processorId = $processor_id;
    $this->config = $config;
    $this->session = $session;
    $this->solr = $solr;

    $this->module = $this->session->resumeModulesHandler($module_id);
  }

  /**
   * Process records in the result set until we run out.
   */
  public function run() {

    $position = 'current';
    $offset = 0;
    $count = $this->config->numDocumentsPerBatch();

    // Build a list of all of the fields we are going to retrieve at once from
    // attachment tables.  We need to retrieve a field if we are using it for
    // any purpose whatsoever.
    $this->module->addFetchSet('attachFields', $this->getAttachFetchSet());

    // I'm not sure how to fencepost fetch() when it runs out of records.
    // For the time being I'm just hacking around that, but we should call
    // Aaron and find out. :-)
    for ($i = 0; $i < 3; ++$i) {
      $result = $this->module->fetch($position, $offset, $count, 'attachFields');
      foreach ($result->modules as $module => $records) {
        $documents = array();
        foreach ($records->rows as $row) {
          $documents[] = $this->createSolrDocument($row);
        }
        $this->solr->addDocuments($documents);
      }
    }
  }

  /**
   * Maps a result record to a Solr document.
   *
   * This is the meat of the processor.  This routine may call various additional
   * Emu queries in order to build the complete document.
   *
   * @param array $row
   *   A row of results from an Emu base query.
   * @return Apache_Solr_Document
   */
  protected function createSolrDocument(array $row) {
    debug($row);
    $document = new Apache_Solr_Document();
    $document->id = $row['irn'];

    // Simplistic, naive approach to adding fields to the document. Works for the
    // moment...
    $tables = $this->getAttachFetchSet();
    foreach ($tables as $table => $fields) {
      foreach ($fields as $field) {
        $document->$field = $row[$field];
      }
    }

    return $document;
  }

  /**
   * Returns an array of all tables/fields to retrieve from the main query.
   *
   * The double-nested array returned by this method is safe to pass to
   * IMuModules::addFetchSet().
   *
   * @return array
   */
  protected function getAttachFetchSet() {
    $columns = array();
    foreach ($this->config->getTables('attach') as $table) {
      foreach ($this->config->getFields($table) as $field) {
        $columns[$table][] = $field->attr('name');
      }
    }
    return $columns;
  }
}
