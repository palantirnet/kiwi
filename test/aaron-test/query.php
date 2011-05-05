<?php
/* First we'll import the base API library
*/
require_once '../../emu/imu.php';
/* Next we'll import the module library, as that will
** provide useful tools for querying and returning 
** results
*/
require_once IMu::$lib . '/module.php';
/* We don't really need these, but they can be useful
*/
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/trace.php';

IMuTrace::setFile('trace.txt');
IMuTrace::setLevel(1);

/* We start with a new session
*/
$session = new IMuSession;
$session->host = "66.158.71.71";
$session->port = "40000";
$session->connect();
$session->login('emu', 'emucorneliafmnh');
$session->suspend = true;

/* We'll use a Module object to do most of the work
*/
$emutable = 'ecatalogue';
$module = new IMuModule($emutable, $session);
$module->destroy = false;

/* We'll setup an array of query terms.
/* Each column:value pair is a two row array.
/* Each boolean operator is also a two row array. 
*/
$terms = array();
$columnname = "CatDepartment";
$queryvalue = "Zoology";
$terms[] = array($columnname, $queryvalue);
/* The query is run by this command and results are stored server side
*/
$module->findTerms(array('and', $terms));
print "Port: $session->port\n";
print "ID: $module->id\n";
