<?php

function debug($message, $label = '') {
  if ($label) {
    $message = $label . ': ' . $message;
  }
  $message .= PHP_EOL;

  print $message;
}
