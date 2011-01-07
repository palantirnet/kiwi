<?php
/*
Title: exception

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/trace.php';

class IMuException extends Exception
{
	public function
	__construct($code = 500)
	{
		parent::__construct();
		$this->code = $code;
		$this->id = 'InternalError';
		$this->args = array();
	}

	public $id;
	public $args;

	public function
	getString($lang = '')
	{
		$string = '[' . $this->code . ']';
		$string .= ' ' . $this->id;
		if (count($this->args) > 0)
			$string .= ' (' . implode(',', $this->args) . ')';
		return $string;
	}

	public function
	__toString()
	{
		return $this->getString();
	}
}

function raise($code, $id)
{
	$exception = new IMuException($code);
	$exception->id = $id;
	$args = func_get_args();
	array_shift($args);
	array_shift($args);
	$exception->args = $args;
	trace(1, 'raising exception %s', $exception);
	throw $exception;
}
?>
