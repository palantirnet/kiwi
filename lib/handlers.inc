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

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    //KiwiOutput::get()->setThreshold(LOG_DEBUG);
    if ($value) {
      if (!is_array($value)) {
        $value = array($value);
      }
      $values = array();
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
      $base = $prefix . '_' . ($field_prefix ? ($field_prefix . '_') : '') . $field_info->itemName();
      $solr_field_name = $base . '_year';
      $document->$solr_field_name = $year_values;
      $solr_field_name = $base . '_month';
      $document->$solr_field_name = $month_values;
      $solr_field_name = $base . '_day';
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
 * Handler for nested-table fields.
 *
 * This is a generic handler.  Many nesttab fields will want their own custom
 * handlers.
 */
class NesttabHandler extends TextHandler {

  public function map(KiwiEmuField $field_info, $value, Apache_Solr_Document $document, $field_prefix = '') {
    if ($value) {

      // Because we do not know in advance what the structure of the field is,
      // nor can we get that data from Emu, we will simply collapse it all
      // together into a multi-value text field.  That's useful for searching
      // but not really for display.  If we want a display-useful field, we
      // will have to write our own handler and assign it to that one field.
      foreach ($value as $v) {
        $data[] = recursive_implode(';', $v);
      }

      parent::map($field_info, $data, $document, $field_prefix);
    }
  }
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
       KiwiOutput::debug("\tMapping " . $field_info->columnName() . " using " . get_class($this));

      // Because this is a reference, the value is a nested associative array of
      // the fields in the referenced table.  Those need to be recursively processed.

      $ref_table = $this->session->getTable($field_info->refTable());
      KiwiOutput::debug("\tThis field references: " . $ref_table->getName());
      foreach ($value as $key => $v) {
        $ref_field = $ref_table->getField($key);
        $handler = $ref_field->getHandler();
        KiwiOutput::debug("\t\tSub-mapping " . $ref_field->columnName() . " using " . get_class($handler));
        $handler->map($ref_field, $v, $document, $field_prefix . $field_info->itemName());
      }
    }
  }
}
