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
   * A collection of field handler objects.
   *
   * These are stateless and reusable, so we can cache them and only have to
   * instantiate one of each.
   *
   * @var array
   */
  protected $handlers = array();

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
    $max_records = $this->config->maxRecordsPerProcessor();

    // Build a list of all of the fields we are going to retrieve at once from
    // attachment tables.  We need to retrieve a field if we are using it for
    // any purpose whatsoever.
    $this->module->addFetchSet('attachFields', $this->getAttachFetchSet());

    //$schema = $this->session->getTableSchema('ecatalogue');
    //debug(array_keys($schema['schema']['columns']));

    // Currently, the way this is running it appears that each child process
    // runs through the entire result set, which is seriously not what we want
    // to do.  This will need to be investigated further. @TODO

    // Spin through all records in the result until we run out or hit the max
    // processing limit, whichever comes first.  Emu doesn't actually tell you
    // when you've reached the end of a result set, but just returns the last
    // record over and over again.  That's what all the $last_irn stuff is about.
    $last_irn = 0;
    $processed = 0;
    $counter = 1;
    while ($processed < $max_records) {
      $result = $this->module->fetch($position, $offset, $count, 'attachFields');
      $processed += $count;
      foreach ($result->modules as $module => $records) {
        $documents = array();
        foreach ($records->rows as $row) {
          KiwiOutput::debug("Child {$this->processorId}: Processing IRN #" . $counter++ . ": " . $row['irn']);
          $documents[] = $this->createSolrDocument($row);
          KiwiOutput::info("Child {$this->processorId}: Indexing {$count} documents to Solr. Indexed {$processed} documents so far.");
          $this->solr->addDocuments($documents);
          if ($last_irn == $row['irn']) {
            break 3;
          }
          else {
            $last_irn = $row['irn'];
          }
        }
      }


      // When fetching multiple records at once, we need to then manually advance
      // the "current" pointer to catch up with it.  This makes no sense, especially
      // why an offset of 1 seems to work regardless of the batch size.
      $this->module->fetch($position, 1, 0, 'attachFields');
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

    $field_info_wrapper = new KiwiFieldInfoWrapper();

    // Simplistic, naive approach to adding fields to the document. Works for the
    // moment...
    $tables = $this->getAttachFetchSet();
    foreach ($tables as $table => $fields) {
      foreach ($fields as $field) {
        $handler = $this->getHandler($table, $field);

        $handler->map($field, $field_info_wrapper->setFieldInfo($this->session->getFieldInfo($table, $field)), $row[$field], $document);
      }
    }

    return $document;
  }

  /**
   * Returns a handler object for the specified field.
   *
   * @param string $table
   *   The table to which the field belongs.
   * @param string $field
   *   The field we want to be able to map.
   * @return FieldHandlerInterface
   */
  protected function getHandler($table, $field) {
    $field_info = $this->session->getFieldInfo($table, $field);
    $class = $this->config->getFieldHandler($table, $field, $field_info['DataType']);
    //debug($class, 'Handler class');
    if (empty($this->handlers[$class])) {
      $this->handlers[$class] = new $class($this->config);
    }
    return $this->handlers[$class];
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

/**
 * Reusable utility wrapper for Emu field information.
 *
 * This class has no constructor, and maintains no cache or state. It is
 * controlled entirely by the field_info array specified in setFieldInfo().
 * That allows it to be reused with new data as if it were a new object but
 * without the overhead of creating hundreds or thousands of new objects.
 */
class KiwiFieldInfoWrapper {

  /**
   * A definition array for the current field, as specified by Emu.
   *
   * @var array
   */
  protected $fieldInfo;

  /**
   * Sets the internal state of the object to a new Emu field definition.
   *
   * @param array $field_info
   * @return KiwiFieldInfoWrapper
   */
  public function setFieldInfo($field_info) {
    $this->fieldInfo = $field_info;

    return $this;
  }

  /**
   * Returns true if this field is multi-value, false otherwise.
   *
   * Note that this depends on the Emu field schema, not on the number of values
   * the field may actually have at any given time.
   *
   * @return boolean
   */
  public function isMultiValue() {
    return (substr($this->fieldInfo['ColumnName'], -4) == '_tab');
  }
}

/**
 * Common interface for all Field mapping handlers.
 */
interface FieldHandlerInterface {

  /**
   * Maps the specified field information onto a Solr document.
   *
   * @todo What happens with multi-value fields?  That's up next, I guess.
   *
   * Normally multi-value fields can just be assigned as an array, so no special
   * handling is needed for them.  For some fields, though, like date, it may
   * be necessary to add special handling.  That is up to each field to figure out.
   *
   * @param string $field_name
   *   The name of the field from Emu that we are adding.
   * @param KiwiFieldInfoWrapper $field_info
   *   The Emu schema information for this field.
   * @param string $value
   *   The value of the field in Emu.
   * @param Apache_Solr_Document $document
   *   The Solr document to which to add the field.
   */
  public function map($field_name, KiwiFieldInfoWrapper $field_info, $value, Apache_Solr_Document $document);

}


class FieldHandlerBase implements FieldHandlerInterface {

  /**
   * The configuration object for this processor.
   *
   * @var KiwiConfiguration
   */
  protected $config;

  /**
   * The prefix to add to a given field name to align it with the right Solr field.
   *
   * @var string
   */
  protected $prefix;

  public function __construct(KiwiConfiguration $config) {
    $this->config = $config;
  }

  public function map($field_name, KiwiFieldInfoWrapper $field_info, $value, Apache_Solr_Document $document) {
    // Some field types will die if there's no value specified, so skip empty
    // values entirely.
    if ($value) {
      KiwiOutput::debug("\tMapping $field_name using " . get_class($this));

      $prefix = ($field_info->isMultiValue()) ? $this->prefix['multi'] : $this->prefix['single'];
      $solr_field_name = $prefix . '_' . $field_name;
      $document->$solr_field_name = $value;
    }
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
class DateHandler extends FieldHandlerBase {
  protected $prefix = array(
    'single' => 'ds',
    'multi' => 'dm'
  );

  public function map($field_name, KiwiFieldInfoWrapper $field_info, $value, Apache_Solr_Document $document) {
    //KiwiOutput::get()->setThreshold(LOG_DEBUG);
    if ($value) {
      if (!is_array($value)) {
        $value = array($value);
      }
      $values = array();
      foreach ($value as $date_value) {
        // Recode the date value from D/M/Y to ISO.
        //KiwiOutput::debug($value, 'Date value');
        list($day, $month, $year) = explode('/', $date_value);
        // Some dates have no month or day listed at all. In that case, default
        // to the 1st so that we have a valid date.
        if (!$day) {
          $day = 1;
        }
        if (!$month) {
          $month = 1;
        }
        $date = new DateTime(NULL, new DateTimeZone('UTC'));
        $date->setDate($year, $month, $day);
        $values[] = $date->format('c') . 'Z';
      }

      parent::map($field_name, $field_info, $values, $document);
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
}

/**
 * Default handler for Emu Float fields.
 */
class FloatHandler extends FieldHandlerBase {
  protected $prefix = array(
    'single' => 'tfs',
    'multi' => 'tfm'
  );
}

/**
 * Placeholder handler, just to make sure we can override fields if needed.
 *
 * @todo Remove this once the point has been made.
 */
class AlternateHandler extends TextHandler {

  public function map($field_name, KiwiFieldInfoWrapper $field_info, $value, Apache_Solr_Document $document) {
    if ($value) {
      debug($field_name, 'Field name');
      debug($field_info, 'Field info');
      debug($value, 'Field value');
      $prefix = ($field_info->isMultiValue()) ? $this->prefix['multi'] : $this->prefix['single'];
      $solr_field_name = $prefix . '_' . $field_name;
      $document->$solr_field_name = $value;
    }
  }
}

/**
 * No-map field handler that does nothing.
 *
 * Use this handler for defaults, or for fields that should be supressed entirely.
 */
class NullHandler extends FieldHandlerBase {

  public function map($field_name, KiwiFieldInfoWrapper $field_info, $value, Apache_Solr_Document $document) {}

}
