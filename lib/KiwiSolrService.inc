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

  /**
   * Overrides Apache_Solr_Service::_sendRawPost()
   *
   * This method adds useful error handling wrappers to the base object.  If
   * Solr throws an Exception with no useful information, this method catches
   * it and rethrows a more useful exception with a unique type so that it
   * can be caught in a sane fashion.
   */
  protected function _sendRawPost($url, $rawPost, $timeout = FALSE, $contentType = 'text/xml; charset=UTF-8') {
    try {
      return parent::_sendRawPost($url, $rawPost, $timeout, $contentType);
    }
    catch (Exception $e) {
      // PHP 5.3 lets us nest exceptions, which is cool.  We can't test this
      // since we're on PHP 5.2, but it's nice to offer it.
      if (version_compare(PHP_VERSION, '5.3', '>=')) {
        $new_e = new KiwiSolrException($e->getMessage(), $e->getCode(), $e);
      }
      else {
        $new_e = new KiwiSolrException($e->getMessage(), $e->getCode());
      }
      $new_e->setSolrMessage($rawPost);

      throw $new_e;
    }
  }

  /**
   * Generate the XML serialization of a Solr document.
   *
   * This is overridden from the parent solely to set htmlspecialchars() to
   * not force UTF-8.  For unknown reasons, even after a string has been
   * normalized to UTF-8 by every possible mechanism it is still sometimes
   * choking here.  However, Solr seems to accept it.  I don't know if Solr is
   * to blame, or PHP, or the original author of the Apache Solr PHP library.
   * In any event, removing that force-conversion here avoids warnings that
   * cause exceptions to be thrown.
   *
   * Note: Yes, the coding style is wrong on this function because it's a trivial
   *   update and I didn't feel like spending time just reformatting whitespace.
   *
   * @param Apache_Solr_Document $document
   * @return type
   */
  protected function _documentToXmlFragment(Apache_Solr_Document $document) {
          $xml = '<doc';

          if ($document->getBoost() !== false)
          {
                  $xml .= ' boost="' . $document->getBoost() . '"';
          }

          $xml .= '>';

          foreach ($document as $key => $value)
          {
                  $key = htmlspecialchars($key, ENT_QUOTES, 'UTF-8');
                  $fieldBoost = $document->getFieldBoost($key);

                  if (is_array($value))
                  {
                          foreach ($value as $multivalue)
                          {
                                  $xml .= '<field name="' . $key . '"';

                                  if ($fieldBoost !== false)
                                  {
                                          $xml .= ' boost="' . $fieldBoost . '"';

                                          // only set the boost for the first field in the set
                                          $fieldBoost = false;
                                  }

                                  $multivalue = htmlspecialchars($multivalue, ENT_NOQUOTES);

                                  $xml .= '>' . $multivalue . '</field>';
                          }
                  }
                  else
                  {
                          $xml .= '<field name="' . $key . '"';

                          if ($fieldBoost !== false)
                          {
                                  $xml .= ' boost="' . $fieldBoost . '"';
                          }

                          $value = htmlspecialchars($value, ENT_NOQUOTES);

                          $xml .= '>' . $value . '</field>';
                  }
          }

          $xml .= '</doc>';

          // replace any control characters to avoid Solr XML parser exception
          return $this->_stripCtrlChars($xml);
  }

}

/**
 * Exception thrown on Solr errors.
 *
 * This version provides useful debugging data.
 */
class KiwiSolrException extends Exception {

  /**
   * The Solr message that triggered this error.
   *
   * @var string
   */
  protected $solrMessage;

  /**
   * Sets the Solr message that caused this exception.
   *
   * @param string $solr_message
   *   The XML Solr message that caused this exception.
   */
  public function setSolrMessage($solr_message) {
    // If possible, pretty up the output.
    if (extension_loaded('tidy')) {
      $solr_message = (string)tidy_parse_string($solr_message, array('indent' => TRUE, 'input-xml' => TRUE, 'wrap' => 300));
    }

    $this->solrMessage = $solr_message;
  }

  /**
   * Returns the Solr message that generated this error.
   *
   * @return string
   */
  public function getSolrMessage() {
    return $this->solrMessage;
  }
}