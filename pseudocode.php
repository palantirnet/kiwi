<?php

//For the processing workers:

$result;

//Create $doc;

add_to_doc($doc, $result->fields);

foreach ($result->append_tables as $table) {
  // This may get rolled into the original query.  TBD.
  //fetch $record
  add_to_doc($doc, $record->fields);
}

foreach ($result->merge_tables as $table) {
  //fetch $records
  merge_flatten($table, $records);
  add_to_doc($doc, $fields);
}

foreach ($result->collapse_tables as $table) {
  //fetch $records
  collapse_flatten($table, $records);
  add_to_go($doc, $field);
}

function merge_flatten($table, $records) {
  //This is pluggable as an object, configurable per table.
  //The default implementation for now is:
  //flatten $records into a single set of multi-value fields
  return $fields;
}

function collapse_flatten($table, $records) {
  //This is pluggable as an object, configurable per table.
  //The default implementation for now is:
  //flatten $records to a single string, and make a pseudo-fields array of it
  return $fields;
}

function add_to_doc($doc, $fields) {
  foreach ($fields as $field) {
    //determine solr name (based on type, searchable, displayable, etc.)
    //add field to $doc.
  }
}
