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
   * @var KiwiSolrService
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

    $batch_size = $this->config->processorInfo('batch-size');
    $max_records_to_process = $this->config->processorInfo('max-size');

    // Build a list of all of the fields we are going to retrieve at once from
    // attachment tables.  We need to retrieve a field if we are using it for
    // any purpose whatsoever.
    $this->module->addFetchSet('attachFields', $this->getFetchSet());

    // Spin through all records in the result until we run out or hit the max
    // processing limit, whichever comes first.  Emu doesn't actually tell you
    // when you've reached the end of a result set, but just returns the last
    // record over and over again.  That's what all the $last_irn stuff is about.
    $last_irn = 0;
    $processed = 0;
    $counter = 1;
    while ($processed < $max_records_to_process) {
      try {
        $result = $this->module->fetch('current', 0, $batch_size, 'attachFields');

        // On the first run, print some helpful user information.  We can't do
        // this before the loop starts because we need the total number of records
        // in the result set.
        if (!$processed) {
          $total_string = number_format($result->hits);
          $max_string = number_format($max_records_to_process);
          KiwiOutput::info("Processor {$this->processorId}: Approximately {$total_string} records to process.  Will process {$max_string} at most.");
        }

        $documents = array();
        foreach ($result->rows as $row) {
          //debug($row);
          KiwiOutput::debug("Processor {$this->processorId}: Processing IRN #" . ($processed + 1) . ": " . $row['irn']);
          if ($last_irn == $row['irn']) {
            break 2;
          }
          else {
            $last_irn = $row['irn'];
          }
          $doc = $this->createSolrDocument($row);
          $processed++;
          //debug($this->solr->renderDocument($doc));
          $documents[] = $doc;
        }

        $this->solr->addDocuments($documents);
        KiwiOutput::info("Processor {$this->processorId}: Indexed $processed records.");
      }
      catch (ImuException $e) {
        KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN {$row['irn']}: Error connecting to Emu.", LOG_ERR);
        // ImuException is buggy and returns an empty string for $e->getMessage().
        // Casting to a string, though, works.  Weird.
        KiwiOutput::get()->writeMessage(sprintf('%s in %s on line %s', (string)$e, $e->getFile(), $e->getLine()), LOG_ERR);
        break;
      }
      catch (KiwiSolrException $e) {
        KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN {$row['irn']}: Error processing Solr document.", LOG_ERR);
        KiwiOutput::get()->writeMessage('Solr error: ' . $e->getMessage(), LOG_ERR);
        KiwiOutput::debug($e->getSolrMessage(), 'Failing Solr message');
      }
      catch (ErrorException $e) {
        KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN {$row['irn']}: Generic PHP error.", LOG_ERR);
        KiwiOutput::get()->writeMessage(sprintf('%s in %s on line %s', $e->getMessage(), $e->getFile(), $e->getLine()), LOG_ERR);
        break;
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
   * Returns an array of the tables/fields for the query.
   *
   * @todo Handle reference fields properly.
   *
   * The array returned by this method is safe to pass to
   * IMuModules::addFetchSet().
   *
   * @return array
   */
  protected function getFetchSet() {
    $columns = array();
    $base_table = $this->config->baseTable();

    foreach ($this->session->getTable($base_table['name']) as $field) {
      if ($field->isReferenceField()) {
        if ($nested = $this->getNestedFetchSet($field, $base_table['name'])) {
          $columns[] = $nested;
        }
      }
      else {
        $columns[] = $field->columnName();
      }
    }

    //debug($columns);
    return $columns;
  }

  /**
   * Returns a nested set of fields for the fetch set from referenced tables.
   *
   * @param KiwiEmuField $field
   *   The reference field we want to traverse to get more fields from a dependent
   *   table.
   * @param string $parent_table
   *   The name of the table from which we are referencing.  This is used to
   *   detect and avoid infinite loops.
   * @return array
   */
  protected function getNestedFetchSet(KiwiEmuField $field, $parent_table) {
    $columns = array();
    $table = $field->refTable();

    $ref_table = $this->session->getTable($table);
    if (!$ref_table) {
      $field_name = $field->columnName();
      KiwiOutput::warn("The field {$field_name} in table '{$parent_table}' references the table '{$table}', which is not defined in the configuration file.");
      return;
    }
    foreach ($ref_table as $ref_field) {
      // A table can reference itself, which if we kept on going would result
      // in an infinite loop.  Instead, we check for a reference back to the
      // original table and only traverse one level.  That is, we can go from
      // ecatalogue->ecatalogue, but ecatalogue->ecatalogue->ecatalogue will
      // be skipped.
      if ($ref_field->isReferenceField()) {
        if ($ref_field->refTable() != $parent_table) {
          $columns[] = $this->getNestedFetchSet($ref_field, $ref_table->getName());
        }
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

  /**
   * Separator for field concatenation.
   *
   * In many cases we fold fields together to make them easily searchable but
   * not really displayble.  This delimiter is used when doing so.
   *
   * @var string
   */
  protected $separator = ' | ';

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session) {
    $this->config = $config;
    $this->session = $session;
  }

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    // Some field types will die if there's no value specified, so skip empty
    // values entirely.
    if ($value) {
      KiwiOutput::debug("\tMapping " . $field_info->columnName() . " using " . get_class($this));

      $value = $this->normalizeCharset($value);

      $prefix = $this->getTypePrefix($field_info, $force_multi);
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName();
      $document->$solr_field_name = $value;

      // If appropriate, throw this field into the content solr field as well.  We
      // use that for all fulltext index searching.
      if ($field_info->indexFulltext()) {
        if (empty($document->content)) {
          $document->content = '';
        }
        $document->content .= $this->separator . (is_array($value) ? recursive_implode(', ', $value) : $value);
      }
    }
  }

  /**
   * Normalize a value to UTF-8 character encoding.
   *
   * This method is a horrid hack, but the best we can do at the moment.  The
   * problem is that the incoming data may be of unknown character encoding,
   * and PHP has no reliable way to detect the character encoding.  We therefore
   * make a good faith attempt to convert extended characters (accents, etc.)
   * to UTF-8.
   *
   * @todo Verify that this still works with non-Latin input strings.
   * @param mixed $value
   *   The value to be normalized.  If it is an array, this method will be called
   *   recursively on each element.
   * @return mixed
   *   The passed value, translated to UTF-8.
   */
  protected function normalizeCharset($value) {
    // If we're processing an array, process each value of the array individually.
    if (is_array($value)) {
      return array_map(array($this, 'normalizeCharset'), $value);
    }

    // Convert any characters we possibly can to their HTML encoded entities.
    // If we don't specify a character encoding then this should do at least
    // a passingly decent job of detecting it, or at least doesn't care as much
    // as other APIs do.  This won't catch every character, but should catch all
    // normal accented characters at least.
    $value = htmlentities($value, ENT_NOQUOTES);

    // Now convert those HTML entities back into real characters, but this time
    // insist on UTF-8.  If all goes well, this will at worst convert UTF-8
    // characters back to UTF-8 and at best convert ISO-8859-1 characters to
    // HTML entities and from HTML entities to UTF-8 characters.
    $value = html_entity_decode($value, ENT_NOQUOTES, 'UTF-8');

    return $value;
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
  protected function getTypePrefix(KiwiEmuField $field_info, $force_multi = FALSE) {
    return ($force_multi || $field_info->isMultiValue()) ? $this->prefix['multi'] : $this->prefix['single'];
  }
}
