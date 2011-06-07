#!/usr/bin/php
<?php

require_once('bootstrap.php');

kiwi_init();

function benchmark() {
  $input = new KiwiInput();
  $input->parse();
  $config = new KiwiConfigurationBenchmark($input);

  $benchmarks[] = array(
    'processors' => 3,
    'batch_size' => 1,
    'max_size' => 10,
  );
  $benchmarks[] = array(
    'processors' => 3,
    'batch_size' => 1,
    'max_size' => 100,
  );
  $benchmarks[] = array(
    'processors' => 3,
    'batch_size' => 1,
    'max_size' => 1000,
  );

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
