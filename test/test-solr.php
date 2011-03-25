<?php

require_once('SolrPhpClient/Apache/Solr/Service.php');



class KiwiSolrService extends Apache_Solr_Service {

}

class KiwiSolrDocument extends Apache_Solr_Document {

}

$solr = new KiwiSolrService('solr.palantir.net', 8080, '/solr/kiwi/');

if ($time = $solr->ping()) {
  print "It took $time seconds to contact the Solr server." . PHP_EOL;
}

$doc = new KiwiSolrDocument();
$doc->title = 'Hello world';
$doc->entity_id = 1;
$doc->id = 'my-document';
$doc->entity = 'node';
$doc->bundle = 'page';
$doc->bundle_name = 'Page';

$response = $solr->addDocument($doc);

print "Solr returned HTTP response: " . $response->getHttpStatus() . ": " . $response->getHttpStatusMessage() . PHP_EOL;

$results = $solr->search('hello');

$raw = $results->getRawResponse();

print_r($raw);
print PHP_EOL;

