<?php

foreach (range(0, 10) as $i) {
  $messages[] = "Hello World #" . $i;
}

$config['messages'] = $messages;
$config['num_children'] = 2;

require_once('pheanstalk/pheanstalk_init.php');

for ($i = 0; $i < $config['num_children']; ++$i) {
  $pid = pcntl_fork();
  if ($pid == -1) {
    die('could not fork');
  } else if ($pid) {
    // we are the parent
    //pcntl_wait($status); //Protect against Zombie children

  } else {
    // we are the child
    print "Creating child #{$i}" . PHP_EOL;
    child_process($config, $i);
    exit();
  }
}

parent_process($config);
print "Parent ending..." . PHP_EOL;


function parent_process($config) {

  $pheanstalk = new Pheanstalk('beanstalk.palantir.net');

  $pheanstalk->useTube('messages');

  // Mostly just for debugging.
  print_r($pheanstalk->statsTube('messages'));

  foreach ($config['messages'] as $message) {
    $pheanstalk->put($message, 10);
    // Slow down the queuing process so that we can test the system better.
    // Obviously the real one won't have this.
    sleep(1);
  }

  for ($i = 0; $i < $config['num_children'] - 1; ++$i) {
    $pheanstalk->put('KILL', 20);
  }

}

function child_process($config, $id) {
  $pheanstalk = new Pheanstalk('beanstalk.palantir.net');

  while (true) {
    // This will block until there is a job to process.
    // If we wanted to process jobs continually, we would put this in a while loop.
    $job = $pheanstalk
      ->watch('messages')
      ->ignore('default')
      ->reserve(5);

    // If FALSE was returned, it means we timed out. In that case we assume
    // the queue is empty and will stay empty, so just give up.
    if (!$job) {
      echo "Child {$id} died of boredom." . PHP_EOL;
      break;
    }

    $data = $job->getData();


    if ($data == 'KILL') {
      echo "KILL: Child {$id} is about to commit sepuku." . PHP_EOL;
      $pheanstalk->delete($job);
      break;
    }
    else {
      echo "Child {$id} says: {$data}" . PHP_EOL;
      $pheanstalk->delete($job);
    }
  }


  //print "We are in the child." . PHP_EOL;
  //print "Child: {$config['first']} {$config['second']}." . PHP_EOL;
}
