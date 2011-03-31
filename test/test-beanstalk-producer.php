<?php

require_once('pheanstalk/pheanstalk_init.php');

$pheanstalk = new Pheanstalk('beanstalk.palantir.net');

// ----------------------------------------
// producer (queues jobs)

$pheanstalk
  ->useTube('testtube')
  ->put("job payload goes here\n");
