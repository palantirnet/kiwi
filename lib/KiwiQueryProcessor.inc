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
   * The base table object for this query.
   *
   * This never changes, so for performance we just request it once and store it.
   *
   * @var KiwiEmuTable
   */
  protected $baseTable;

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

    $this->baseTable = $this->session->getTable($this->config->baseTable());
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

    // Processed increases with the number of records successfully turned into
    // Solr documents, which if there is an error may be less than the number
    // of records we've tried to process.  That's why these are separate variables.
    $processed = 0;
    $attempt_counter = 1;

    // Spin through all records in the result until we run out or hit the max
    // processing limit, whichever comes first.  Emu doesn't actually tell you
    // when you've reached the end of a result set, but just returns the last
    // record over and over again.  That's what all the $last_irn stuff is about.
    $last_irn = 0;
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
          if ($last_irn == $row['irn']) {
            break 2;
          }
          KiwiOutput::debug("Processor {$this->processorId}: Processing IRN #" . $attempt_counter++ . ": " . $row['irn']);

          // If there is an error processing this row, keep processing other rows.
          try {
            $last_irn = $row['irn'];
            $doc = $this->createSolrDocument($row);
            $processed++;
            //debug($this->solr->renderDocument($doc));
            $documents[] = $doc;
          }
          catch (ErrorException $e) {
            KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN {$row['irn']}: Generic PHP error.  Skipping this IRN.", LOG_ERR);
            KiwiOutput::get()->writeMessage(sprintf('%s in %s on line %s', $e->getMessage(), $e->getFile(), $e->getLine()), LOG_ERR);
          }
        }

        try {
          $this->solr->addDocuments($documents);
          KiwiOutput::info("Processor {$this->processorId}: Indexed $processed records.");
        }
        catch (ErrorException $e) {
          // This is most often caused by invalid characters in one of the strings
          // that is being processed, which triggers a PHP warning. We cannot add
          // that batch to the index, but we can report it and continue processing.
          // We can do a reasonably good job of detecting if it's an invalid
          // character error by digging through the backtrace, since we know
          // where in the stack trace the error would be.
          //
          // If we are unable to reliably detect that error, then it's some other
          // unknown error that we cannot handle cleanly.  Report it and hope
          // the user can figure ito out.
          //
          $trace = $e->getTrace();
          if ($trace[1]['function'] == 'htmlspecialchars') {
            if (isset($trace[0]['args'][0])) {
              $bad_string = $trace[0]['args'][0];
            }
            if (isset($trace[1]['args'][0]->id)) {
              $bad_document = $trace[1]['args'][0];
            }

            if (isset($bad_string) && isset($bad_document)) {
              $bad_fields = array();
              foreach ($bad_document as $field => $value) {
                if ($value == $trace[0]['args'][0]) {
                  $bad_fields[] = $field;
                }
              }
              $error = "Error processing field. It likely contains invalid UTF-8 characters.\nIRN: %s\nInvalid string: %s\nPossible fields: %s\nSkipping this batch.";
              KiwiOutput::get()->writeMessage(sprintf($error, $bad_document->id, $bad_string, implode(', ', $bad_fields)), LOG_ERR);
              continue;
            }
          }
          KiwiOutput::get()->writeMessage("Processor {$this->processorId} while committing to Solr.  Last IRN in batch: {$row['irn']}. Note the actual error could be from another IRN in the same batch. Skipping this batch.", LOG_ERR);
          KiwiOutput::get()->writeMessage(sprintf('%s in %s on line %s', $e->getMessage(), $e->getFile(), $e->getLine()), LOG_ERR);
        }
      }
      catch (ImuException $e) {
        KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN #{$attempt_counter}: {$row['irn']}: Error connecting to Emu.", LOG_ERR);
        // ImuException is buggy and returns an empty string for $e->getMessage().
        // Casting to a string, though, works.  Weird.
        // Emu is also very fault-intolerant, so if there is an exception it is
        // likely that Emu itself will jam.  We therefore break to avoid an
        // infinite loop of fail.
        KiwiOutput::get()->writeMessage(sprintf('%s in %s on line %s', (string)$e, $e->getFile(), $e->getLine()), LOG_ERR);
        break;
      }
      catch (KiwiSolrException $e) {
        KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN #{$attempt_counter}: {$row['irn']}: Error processing Solr document.", LOG_ERR);
        KiwiOutput::get()->writeMessage('Solr error: ' . $e->getMessage(), LOG_ERR);
        // @TODO Switch this back to just a debug call.  We just need the
        // output with the error messages for a while to debug some weird Solr
        // behavior.
        KiwiOutput::get()->writeMessage('Failing Solr message: ' . $e->getSolrMessage(), LOG_ERR);
        KiwiOutput::debug($e->getSolrMessage(), 'Failing Solr message');
      }
      catch (ErrorException $e) {
        // This is most likely caused by a failure that will result in an infinite
        // loop, so we break out entirely.
        KiwiOutput::get()->writeMessage("Processor {$this->processorId} while processing IRN #{$attempt_counter}: {$row['irn']}: Generic PHP error.", LOG_ERR);
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

    foreach ($this->baseTable as $field) {
      $column_name = $field->columnName();
      if (array_key_exists($column_name, $row)) {
        $handler = $field->getHandler();
        $handler->setRecord($row);
        $handler->map($field, $row[$column_name], $document);
      }
      else {
        KiwiOutput::warn("Field not included in result set: " . $column_name);
      }
    }

    //debug($document);
    return $document;
  }

  /**
   * Returns an array of the tables/fields for the query.
   *
   * The array returned by this method is safe to pass to
   * IMuModules::addFetchSet().
   *
   * @return array
   */
  protected function getFetchSet() {
    $columns = array();
    $base_table = $this->config->baseTable();

    foreach ($this->baseTable as $field) {
      if ($field->isReferenceField()) {
        if ($nested = $this->getNestedFetchSet($field, $base_table)) {
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
   * @param KiwiEmuField $field
   *   The Emu schema information for this field.
   * @param string $value
   *   The value of the field in Emu.
   * @param Apache_Solr_Document $document
   *   The Solr document to which to add the field.
   */
  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document);

  /**
   * Set the record which this handler is processing.
   *
   * Note that in the case of a sub-record, the sub-record should be assigned
   * here, not the parent record.
   *
   * @param array $record
   */
  public function setRecord(array $record);

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

  /**
   * The record that this handler is processing.
   *
   * @var array
   */
  protected $record;

  public function __construct(KiwiConfiguration $config, KiwiImuSession $session) {
    $this->config = $config;
    $this->session = $session;
  }

  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    // Some field types will die if there's no value specified, so skip empty
    // values entirely.
    if ($value) {
      KiwiOutput::debug("\tMapping " . $field->columnName() . " using " . get_class($this));

      $value = $this->normalizeCharset($value);

      $prefix = $this->getTypePrefix($field, $force_multi);
      $solr_field_name = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field->itemName();
      $document->$solr_field_name = $value;

      // If appropriate, throw this field into the content solr field as well.  We
      // use that for all fulltext index searching.
      if ($field->indexFulltext()) {
        if (empty($document->content)) {
          $document->content = '';
        }
        $document->content .= $this->separator . (is_array($value) ? recursive_implode(', ', $value) : $value);
      }
    }
  }

  public function setRecord(array $record) {
    $this->record = $record;
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

    // mb_detect_encoding() in most situations supports only two character sets,
    // ASCII and UTF-8.  However, it is sadly not unusual for incoming data
    // to be in Windows-1252, which is used by no software in the universe
    // except Microsoft Word. We therefore guess that a false/not-found character
    // set is Windows-1251, and try to convert that to UTF-8.
    // Note: The odds of this breaking on some other character encoding besides
    // those three is rather high.  The real fix is to fix Emu so that it
    // doesn't output non-UTF-8.

    $encoding = mb_detect_encoding($value);
    switch ($encoding) {

      // ASCCI strings are the only things that work properly without processing.
      // It probably helps that ASCII is by definition a strict subset of UTF-8.
      case 'ASCII':
        break;

      // I have absolutely no idea why UTF-8 strings need to be converted
      // from UTF-8 to UTF-8, but if this code is removed many strings end up
      // failing with invalid multi-byte encoding.  This also seems like a
      // rather hacky way of doing it, but it seems to work. Best not to ask
      // questions when character encoding is involved lest awaken
      // the ancient sleeping deamons.
      case 'UTF-8':
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
        break;

      // A False return from mb_detect_encoding() means that it couldn't
      // figure out what the encoding is.  In a standard configuration mb_* only
      // knows about ASCCI and UTF-8, so that's not especially useful. We will
      // make an assumption that anything else is Windows-1252, aka "Microsoft's
      // proprietary character encoding used by nothing in the universe except
      // MS Word"  If correct, this will convert $value from Windows-1252 to
      // UTF-8, transliterating where necessary rather than just failing. We
      // also record a log entry about it so that we know that someone is using
      // Word when they shouldn't.
      case FALSE:
        $value = iconv('Windows-1252', 'UTF-8//TRANSLIT', $value);
        KiwiOutput::warn("Detected Windows-1252 character encoding\nIRN: {$this->record['irn']}\nString: {$value}.\nThe use of that format is not recommended. Please ensure all incoming data is in UTF-8 encoding.\n");
        break;

      // This should only trigger if mb_detect_encoding() is configured to
      // understand something other than UTF-8 and ASCII.  Simply flag that
      // the system doesn't expect something even more esoteric than Windows-1252.
      default:
        KiwiOutput::warn("Unrecognized character encoding: '{$encoding}' for string '{$value}'.  Please ensure all data in Emu is in UTF-8 format.");
    }

    return $value;
  }

  /**
   * Returns the solr field prefix for the provided field.
   *
   * @param KiwiEmuField $field
   *   The field for which we want to know the prefix.
   * @return string
   *   The prefix for this field type.  Note it does not include the trailing _,
   *   so you'll have to add it yourself.
   */
  protected function getTypePrefix(KiwiEmuField $field, $force_multi = FALSE) {
    return ($force_multi || $field->isMultiValue()) ? $this->prefix['multi'] : $this->prefix['single'];
  }
}
