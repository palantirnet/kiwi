<?php

require_once('pheanstalk/pheanstalk_init.php');

$pheanstalk = new Pheanstalk('beanstalk.palantir.net');

// ----------------------------------------
// worker (performs jobs)

// This will block until there is a job to process.
// If we wanted to process jobs continually, we would put this in a while loop.
$job = $pheanstalk
  ->watch('testtube')
  ->ignore('default')
  ->reserve();

echo $job->getData();

$pheanstalk->delete($job);
