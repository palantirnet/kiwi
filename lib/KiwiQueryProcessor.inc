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

    $this->module = $this->session->resumeModuleHandler($module_id);
  }

  /**
   * Process records in the result set until we run out.
   */
  public function run() {

    $position = 'current';
    $offset = 0;
    $count = $this->config->numDocumentsPerBatch();
    $max_records = $this->config->maxRecordsPerProcessor();

    // Build a list of all of the fields we are going to retrieve at once from
    // attachment tables.  We need to retrieve a field if we are using it for
    // any purpose whatsoever.
    $this->module->addFetchSet('attachFields', $this->getAttachFetchSet());

    //$schema = $this->session->getTableSchema('ecatalogue');
    //debug(array_keys($schema['schema']['columns']));

    // Spin through all records in the result until we run out or hit the max
    // processing limit, whichever comes first.  Emu doesn't actually tell you
    // when you've reached the end of a result set, but just returns the last
    // record over and over again.  That's what all the $last_irn stuff is about.
    $last_irn = 0;
    $processed = 0;
    $counter = 1;
    while ($processed < $max_records) {
      $result = $this->module->fetch($position, $offset, $count, 'attachFields');
      if (!$processed) {
        $total_string = number_format($result->hits);
        $max_string = number_format($max_records);
        KiwiOutput::info("Processor {$this->processorId}: Approximately {$total_string} records to process.  Will process {$max_string} at most.");
      }

      $documents = array();
      foreach ($result->rows as $row) {
        KiwiOutput::debug("Processor {$this->processorId}: Processing IRN #" . $counter++ . ": " . $row['irn']);
        $documents[] = $this->createSolrDocument($row);
        if ($last_irn == $row['irn']) {
          break 2;
        }
        else {
          $last_irn = $row['irn'];
        }
      }

      //KiwiOutput::get()->setThreshold(LOG_DEBUG);
      try {
        $this->solr->addDocuments($documents);
        $processed += $count;
        KiwiOutput::info("Processor {$this->processorId}: Indexed $processed records.");
      }
      catch (Exception $e) {
        KiwiOutput::get()->writeMessage("Processor {$this->processorId}: Error processing Solr document.", LOG_ERR);
        KiwiOutput::debug($e->getMessage(), 'Solr error');
        $trace = $e->getTrace();
        if ($trace[0]['class'] == 'Apache_Solr_Service' && $trace[0]['function'] == '_sendRawPost') {
          // To ease debugging, print out the failing Solr document. If the
          // Tidy extension is available, also use that to pretty-print the XML.
          $solr_xml = $trace[0]['args'][1];
          if (extension_loaded('tidy')) {
            $solr_xml = (string)tidy_parse_string($solr_xml, array('indent' => TRUE, 'input-xml' => TRUE, 'wrap' => 300));
          }
          KiwiOutput::debug($solr_xml, "Solr document\n");
        }
      }
      //KiwiOutput::get()->setThreshold();
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
    //debug($row);
    $document = new Apache_Solr_Document();
    $document->id = $row['irn'];

    $base_table = $this->config->baseTable();
    $table = $this->session->getTable($base_table['name']);
    foreach ($table as $field) {
      if (array_key_exists($field->columnName(), $row)) {
        $handler = $field->getHandler();
        $handler->map($field, $row[$field->columnName()], $document);
      }
      else {
        KiwiOutput::warn("Field not included in result set: " . $field->columnName());
      }
    }

    //debug($document);
    return $document;
  }

  /**
   * Returns a handler object for the specified field.
   *
   * @param string $table
   *   The table to which the field belongs.
   * @param KiwiEmuField $field
   *   The field we want to be able to map.
   * @return FieldHandlerInterface
   */
  protected function getHandler($table, KiwiEmuField $field) {

    $field_options = $this->config->getFieldOptions($table, $field->columnName(), $field->dataType());
    $class = $field_options['handler'];
    if (empty($this->handlers[$class])) {
      $this->handlers[$class] = new $class($this->config, $this->session);
    }
    return $this->handlers[$class];
  }

  /**
   * Returns an array of the tables/fields for the query.
   *
   * @todo Handle reference fields properly.
   *
   * The array returned by this method is safe to pass to
   * IMuModules::addFetchSet().
   *
   * @return array
   */
  protected function getAttachFetchSet() {
    $columns = array();
    $base_table = $this->config->baseTable();

    foreach ($this->session->getTable($base_table['name']) as $field) {
      if ($field->isAttachmentField()) {
        $columns[] = $this->getNestedFetchSet($field);
      }
      else {
        $columns[] = $field->columnName();
      }
    }

    //debug($columns);
    return $columns;
  }

  protected function getNestedFetchSet(KiwiEmuField $field) {
    $columns = array();
    $table = $field->refTable();
    $refTable = $this->session->getTable($table);
    foreach ($refTable as $ref_field) {
      if ($ref_field->isAttachmentField()) {
        $columns[] = $this->getNestedFetchSet($ref_field);
      }
      else {
        $columns[] = $ref_field->columnName();
      }
    }

    return $field->columnName() . '.(' . implode(',', $columns) . ')';
  }
}

/**
 * Common interface for all Field mapping handlers.
 */
interface FieldHandlerInterface {

  /**
   * Constructor
   *
   * @param KiwiConfiguration $config
   *   The configuration object for this run.
   * @param KiwiImuSession $session
   *   The Emu connection from which we are fetching.
   */
  public function __construct(KiwiConfiguration $config, KiwiImuSession $session);

  /**
   * Maps the specified field information onto a Solr document.
   *
   * @todo What happens with multi-value fields?  That's up next, I guess.
   *
   * Normally multi-value fields can just be assigned as an array, so no special
   * handling is needed for them.  For some fields, though, like date, it may
   * be necessary to add special handling.  That is up to each field to figure out.
   *
   * @param KiwiEmuField $field_info
   *   The Emu schema information for this field.
   * @param string $value
   *   The value of the field in Emu.
   * @param Apache_Solr_Document $document
   *   The Solr document to which to add the field.
   */
  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document);

}


class FieldHandlerBase implements FieldHandlerInterface {

  /**
   * The configuration object for this processor.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The session object this processor is running against.
   *
   * @var KiwiImuSession
   */
  protected $session;

  /**
   * The prefix to add to a given field name to align it with the right Solr field.
   *
   * @var string
   */
  protected $prefix;

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session) {
    $this->config = $config;
    $this->session = $session;
  }

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    // Some field types will die if there's no value specified, so skip empty
    // values entirely.
    if ($value) {
      KiwiOutput::debug("\tMapping " . $field_info->columnName() . " using " . get_class($this));

      $prefix = $this->getTypePrefix($field_info);
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName();
      $document->$solr_field_name = $value;

      // If appropriate, throw this field into the content solr field as well.  We
      // use that for all fulltext index searching.
      if ($field_info->indexFulltext()) {
        $document->content = $value;
      }
    }
  }

  /**
   * Returns the solr field prefix for the provided field.
   *
   * @param KiwiEmuField $field_info
   *   The field for which we want to know the prefix.
   * @return string
   *   The prefix for this field type.  Note it does not include the trailing _,
   *   so you'll have to add it yourself.
   */
  protected function getTypePrefix(KiwiEmuField $field_info) {
    return ($field_info->isMultiValue()) ? $this->prefix['multi'] : $this->prefix['single'];
  }
}

/**
 * Default handler for Emu Text fields.
 */
class TextHandler extends FieldHandlerBase {
  protected $prefix = array(
    'single' => 'ss',
    'multi' => 'sm'
  );
}

/**
 * Default handler for Emu Date fields.
 */
class DateHandler extends TextHandler {

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    //KiwiOutput::get()->setThreshold(LOG_DEBUG);
    if ($value) {
      if (!is_array($value)) {
        $value = array($value);
      }
      $values = array();
      foreach ($value as $date_value) {
        // Recode the date value from D/M/Y to a more human (American)-friendly format.
        //KiwiOutput::debug($value, 'Date value');
        list($day, $month, $year) = explode('/', $date_value);
        // Some dates have no month or day listed at all. In that case, default
        // to the 1st so that we have a valid date.
        $format = array();
        if (!$day) {
          $day = 1;
          $day_values[] = 0;
        }
        else {
          $format[] = 'j';
          // Force-cast to an int to eliminate leading 0s.
          $day_values[] = (int)$day;
        }
        if (!$month) {
          $month = 1;
          $month_values[] = 0;
        }
        else {
          $format[] = 'F';
          // Force-cast to an int to eliminate leading 0s.
          $month_values[] = (int)$month;
        }
        $format[] = 'Y';
        $year_values[] = $year;
        $date = new DateTime(NULL, new DateTimeZone('UTC'));
        $date->setDate($year, $month, $day);

        $values[] = $date->format(implode(' ', $format));
      }

      parent::map($field_info, $values, $document, $field_prefix);

      // Each date part get indexed into additional denormalized fields, as ints.
      // That way we can sort and query on parts of the date, even though we don't
      // always have the complete date.
      if ($field_info->isSortable() && !$field_info->isMultiValue()) {
        $prefix = 'sint';
      }
      else {
        $prefix = ($field_info->isMultiValue()) ? 'im' : 'is';
      }
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName() . '_year';
      $document->$solr_field_name = $year_values;
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName() . '_month';
      $document->$solr_field_name = $month_values;
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName() . '_day';
      $document->$solr_field_name = $day_values;
    }
    //KiwiOutput::get()->setThreshold();
  }
}

/**
 * Default handler for Emu Integer fields.
 */
class IntegerHandler extends FieldHandlerBase {
  protected $prefix = array(
    'single' => 'is',
    'multi' => 'im'
  );

  protected function getTypePrefix(KiwiEmuField $field_info) {
    if ($field_info->isSortable() && !$field_info->isMultiValue()) {
      return 'sint';
    }
    parent::getTypePrefix($field_info);
  }
}

/**
 * Default handler for Emu Float fields.
 */
class FloatHandler extends FieldHandlerBase {
  protected $prefix = array(
    'single' => 'tfs',
    'multi' => 'tfm'
  );

  protected function getTypePrefix(KiwiEmuField $field_info) {
    if ($field_info->isSortable() && !$field_info->isMultiValue()) {
      return 'sfloat';
    }
    parent::getTypePrefix($field_info);
  }
}

/**
 * Placeholder handler, just to make sure we can override fields if needed.
 *
 * @todo Remove this once the point has been made.
 */
class AlternateHandler extends FloatHandler {

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    //KiwiOutput::get()->setThreshold(LOG_DEBUG);
    if ($value) {
      KiwiOutput::debug($field_info, 'Field info');
      KiwiOutput::debug($value, 'Field value');
      $prefix = ($field_info->isMultiValue()) ? $this->prefix['multi'] : $this->prefix['single'];
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName();
      //$document->$solr_field_name = $value;
    }
    //KiwiOutput::get()->setThreshold();
  }
}

/**
 * Handler for latitude/longitude fields, which need custom processing.
 */
class LatLongHandler extends FloatHandler {

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    if ($value) {
      if ($field_info->isMultiValue() || is_array($value)) {
        $new_values = array();
        foreach ($value as $v) {
          $new_values[] = $this->convertToFloat($v);
        }
        $value = $new_values;
      }
      else {
        if (!is_float($value)) {
          $value = $this->convertToFloat($value);
        }
      }
      parent::map($field_info, $value, $document, $field_prefix);
    }
  }

  /**
   * Returns the float version of a string Lat/Long field.
   *
   * @param string $value
   *   The string version of the Lat/Long value.
   * @return float
   *   The Lat/Long value, converted to a float.
   */
  protected function convertToFloat($value) {
    list($parts['degrees'], $parts['minutes'], $parts['seconds'], $parts['direction']) = explode(' ', $value);
    $value = $parts['degrees'] + $parts['minutes'] / 60 + $parts['seconds'] / 3600;
    return $value;
  }
}

/**
 * No-map field handler that does nothing.
 *
 * Use this handler for defaults, or for fields that should be supressed entirely.
 */
class NullHandler extends FieldHandlerBase {
  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {}
}

/**
 * Handler for attachment fields.
 *
 * These are single-value 1:1 reference fields.
 */
class AttachmentReferenceHandler extends FieldHandlerBase {
  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    //debug($field_info);

    if ($value) {
      // Because this is a reference, the value is a nested associative array of
      // the fields in the referenced table.  Those need to be recursively processed.

      $ref_table = $this->session->getTable($field_info->refTable());
      foreach ($value as $key => $v) {
        $ref_field = $ref_table->getField($key);
        $handler = $ref_field->getHandler();
        $handler->map($ref_field, $v, $document, $field_prefix . $field_info->itemName());
      }
    }
  }
}
