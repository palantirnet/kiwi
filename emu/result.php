<?php
/*
Title: result

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';

class IMuResult
{
	public function
	__construct($array = array())
	{
		foreach ($array as $name => $value)
			$this->$name = $this->parse($value);
	}

	private function
	parse($value)
	{
		if (! is_array($value))
			return $value;
		if (empty($value))
			return array();
		if (array_keys($value) === range(0, count($value) - 1))
		{
			$array = array();
			foreach ($value as $element)
				$array[] = $this->parse($element);
			return $array;
		}
		return new IMuResult($value);
	}
}
?>
