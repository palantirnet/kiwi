<?php

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

  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    if ($value) {
      if (!is_array($value)) {
        $value = array($value);
      }

      $values = array();
      $day_values = array();
      $month_values = array();
      $year_values = array();

      foreach ($value as $date_value) {
        // Sometimes, we may get an array that has a NULL value in it.  This is
        // most likely caused by user error entering content into Emu, but we
        // have to deal with it anyway.
        if (!$date_value) {
          continue;
        }
        // Recode the date value from D/M/Y to a more human (American)-friendly format.
        if (strpos($date_value, '/') === FALSE) {
          list($day, $month, $year) = array(0, 0, $date_value);
        }
        else {
          list($day, $month, $year) = explode('/', $date_value);
        }
        // Some dates have no month or day or year listed at all. In that case, default
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

        if (!$year) {
          $year = 1;
          $year_values[] = 0;
        }
        else {
          $format[] = 'Y';
          // Force-cast to an int to eliminate leading 0s.
          $year_values[] = (int)$year;
        }

        $date = new DateTime(NULL, new DateTimeZone('UTC'));
        $date->setDate($year, $month, $day);

        $values[] = $date->format(implode(' ', $format));
      }

      parent::map($field, $values, $document, $field_prefix);

      // Each date part get indexed into additional denormalized fields, as ints.
      // That way we can sort and query on parts of the date, even though we don't
      // always have the complete date.
      if ($field->isSortable() && !$field->isMultiValue()) {
        $prefix = 'sint';
      }
      else {
        $prefix = ($field->isMultiValue()) ? 'im' : 'is';
      }
      $base = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field->itemName();
      $solr_field_name = $base . '_year';
      $document->$solr_field_name = $year_values;
      $solr_field_name = $base . '_month';
      $document->$solr_field_name = $month_values;
      $solr_field_name = $base . '_day';
      $document->$solr_field_name = $day_values;
    }
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

  protected function getTypePrefix(KiwiEmuField $field, $force_multi = FALSE) {
    if ($field->isSortable() && !($force_multi ||$field->isMultiValue())) {
      return 'sint';
    }
    return parent::getTypePrefix($field);
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

  protected function getTypePrefix(KiwiEmuField $field, $force_multi = FALSE) {
    if ($field->isSortable() && !($force_multi || $field->isMultiValue())) {
      return 'sfloat';
    }
    return parent::getTypePrefix($field);
  }
}

/**
 * Handler for latitude/longitude fields, which need custom processing.
 */
class LatLongHandler extends FloatHandler {

  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    if ($value) {
      if ($field->isMultiValue() || is_array($value)) {
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
      parent::map($field, $value, $document, $field_prefix);
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
  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {}
}

/**
 * Handler for nested-table fields.
 *
 * This is a generic handler.  Many nesttab fields will want their own custom
 * handlers.
 */
class NesttabHandler extends TextHandler {

  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    if ($value) {

      // Because we do not know in advance what the structure of the field is,
      // nor can we get that data from Emu, we will simply collapse it all
      // together into a multi-value text field.  That's useful for searching
      // but not really for display.  If we want a display-useful field, we
      // will have to write our own handler and assign it to that one field.

      // In the special case that a nesttab field only has one single value,
      // we'll avoid quoting it and let it behave as if it were a string value.
      $data = array();
      foreach ($value as $v) {
        $string = recursive_implode($this->separator, $v);
        if (strpos($string, $this->separator) !== FALSE) {
          $string = '"' . $string . '"';
        }
        if ($string) {
          $data[] = $string;
        }
      }

      parent::map($field, $data, $document, $field_prefix);
    }
  }
}


/**
 * Handler for attachment fields.
 *
 * These are single-value 1:1 reference fields.
 */
class AttachmentReferenceHandler extends FieldHandlerBase {
  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    //debug($field);

    if ($value) {
       KiwiOutput::debug("\tMapping " . $field->columnName() . " using " . get_class($this));

      // Because this is a reference, the value is a nested associative array of
      // the fields in the referenced table.  Those need to be recursively processed.

      $ref_table = $this->session->getTable($field->refTable());
      KiwiOutput::debug("\tThis field references: " . $ref_table->getName());
      foreach ($value as $key => $v) {
        $ref_field = $ref_table->getField($key);
        $handler = $ref_field->getHandler();
        KiwiOutput::debug("\t\tSub-mapping " . $ref_field->columnName() . " using " . get_class($handler));
        $handler->map($ref_field, $v, $document, $field_prefix . $field->itemName());
      }
    }
  }
}

/**
 * Handler for Merge reference fields.
 *
 * These are multi-value reference fields.
 */
class MergeReferenceHandler extends FieldHandlerBase {

  public function map(KiwiEmuField $field, $value, Apache_Solr_Document $document, $field_prefix = '', $force_multi = FALSE) {
    if ($value && current($value)) {
      KiwiOutput::debug("\tMapping " . $field->columnName() . " using " . get_class($this));

      $ref_table = $this->session->getTable($field->refTable());
      KiwiOutput::debug("\tThis field references: " . $ref_table->getName());

      foreach ($value as $i => $sub_record) {
        if (! $sub_record) {
          KiwiOutput::get()->writeMessage(sprintf('Missing sub-record in Merge field: %s in table %s, IRN %s', $field->columnName(), $ref_table->getName(), $document->id), LOG_WARNING);
          continue;
        }
        foreach ($sub_record as $field_name => $v) {

          $ref_field = $ref_table->getField($field_name);
          if ($ref_field->isReferenceField()) {
            // Attachment fields referenced from a Merge field are handled directly
            // rather than being folded.  Fortunately we know that Attachment
            // fields are single-value.  We need to include the _ in the prefix
            // though, because we're nesting prefixes here.
            $ref_field->getHandler()->map($ref_field, $v, $document, $field_prefix . $field->itemName() . '_');
          }
          else {
            // If the sub-field is multi-value, it needs to get folded down to
            // a single string.  Solr can't handle sub-array fields.
            if (is_array($v)) {
              $v = recursive_implode($this->separator, array_filter($v));
            }
            if ($v) {
              $fields[$field_name][] = $v;
            }
          }
        }
      }

      foreach ($fields as $field_name => $v) {
        $ref_field = $ref_table->getField($field_name);
        $handler = $ref_field->getHandler();
        $handler->map($ref_field, $v, $document, $field_prefix . $field->itemName(), TRUE);
      }
    }
  }
}
