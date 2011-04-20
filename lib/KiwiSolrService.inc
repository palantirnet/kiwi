<?php

/**
 * Solr connection object.
 *
 * We are extending the ApacheSolr service class so that we can add back in
 * debugging capability that it doesn't have, which is annoying.
 */
class KiwiSolrService extends Apache_Solr_Service {

  /**
   * Render a single Document fragment.
   * 
   * This is just a public wrapper around the foolishly protected 
   * _documentToXmlFragment() method.  It also pretty prints if possible.
   *
   * @param Apache_Solr_Document $document
   * @return string
   *   The XML string representing a single document.
   */
  public function renderDocument(Apache_Solr_Document $document) {
    $output = $this->_documentToXmlFragment($document);

    // If possible, pretty up the output.
    if (extension_loaded('tidy')) {
      $output = (string)tidy_parse_string($output, array('indent' => TRUE, 'input-xml' => TRUE, 'wrap' => 300));
    }

    return $output;
  }

  /**
   * Render the output that would be sent to the Solr server by addDocuments().
   *
   * This code is mostly just a copy paste from addDocuments(), which stupidly
   * renders and sends in a single method, making debugging nigh on impossible.
   *
   * @param array $documents Should be an array of Apache_Solr_Document instances
   * @param boolean $allowDups
   * @param boolean $overwritePending
   * @param boolean $overwriteCommitted
   * @return string
   *   The XML that would be sent to the server if we called addDocuments()
   *   with the same inputs.
   */
  public function renderRequest($documents, $allowDups = false, $overwritePending = true, $overwriteCommitted = true) {
    $dupValue = $allowDups ? 'true' : 'false';
    $pendingValue = $overwritePending ? 'true' : 'false';
    $committedValue = $overwriteCommitted ? 'true' : 'false';

    $rawPost = '<add allowDups="' . $dupValue . '" overwritePending="' . $pendingValue . '" overwriteCommitted="' . $committedValue . '">';

    foreach ($documents as $document) {
      if ($document instanceof Apache_Solr_Document) {
        $rawPost .= $this->_documentToXmlFragment($document);
      }
    }

    $rawPost .= '</add>';

    // If possible, pretty up the output.
    if (extension_loaded('tidy')) {
      $rawPost = (string)tidy_parse_string($rawPost, array('indent' => TRUE, 'input-xml' => TRUE, 'wrap' => 300));
    }

    return $rawPost;
  }
}
