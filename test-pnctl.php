<?php

// Load config file.
$config = array(
  'first' => 'Hello',
  'second' => 'World',
);

$num_children = 1;

for ($i = 0; $i < $num_children; ++$i) {
  $pid = pcntl_fork();
  if ($pid == -1) {
    die('could not fork');
  } else if ($pid) {
    // we are the parent
    //pcntl_wait($status); //Protect against Zombie children
  } else {
    // we are the child
    print "We are in the child." . PHP_EOL;
    sleep(1);
    print "Exiting child." . PHP_EOL;
    exit();
  }
}


print "Parent ending..." . PHP_EOL;
