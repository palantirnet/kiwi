#!/usr/bin/php
<?php

require_once('bootstrap.php');

kiwi_init();

function benchmark() {
  $input = new KiwiInput();
  $input->parse();
  $config = new KiwiConfigurationBenchmark($input);

  $benchmarks = array();
  $benchmarks[] = array(
    'processors' => 3,
    'batch_size' => 1,
    'max_size' => 10,
  );

  foreach (range(1, 3) as $processors) {
    foreach (array(1, 10, 25, 50, 100, 200) as $batch_size) {
      foreach (array(10, 100, 500, 1000, 2000) as $max_size) {
        $benchmarks[] = array(
          'processors' => $processors,
          'batch_size' => $batch_size,
          'max_size' => $max_size,
        );
      }
    }
  }

  foreach ($benchmarks as $benchmark) {
    debug("-----\nProcessors: {$benchmark['processors']}, Batch size: {$benchmark['batch_size']}, Records: {$benchmark['max_size']}");
    $config->setProcessorInfo($benchmark['processors'], $benchmark['batch_size'], $benchmark['max_size']);
    main($input, $config);
  }

}

class KiwiConfigurationBenchmark extends KiwiConfiguration {

  protected $processorOverride;
  protected $batchSizeOverride;
  protected $maxSizeOverride;

  public function setProcessorInfo($processors = NULL, $batch_size = NULL, $max_size = NULL) {
    if ($processors) {
      $this->processorOverride = $processors;
    }
    if ($batch_size) {
      $this->batchSizeOverride = $batch_size;
    }
    if ($max_size) {
      $this->maxSizeOverride = $max_size;
    }
  }

  public function numChildProcesses() {
    return $this->processorOverride ? $this->processorOverride : parent::numChildProcesses();
  }

  public function numDocumentsPerBatch() {
    return $this->batchSizeOverride ? $this->batchSizeOverride : parent::numDocumentsPerBatch();
  }

  public function maxRecordsPerProcessor() {
    return $this->maxSizeOverride ? $this->maxSizeOverride : parent::maxRecordsPerProcessor();
  }

}

benchmark();
exit();
