<?php
/*
Title: module/luts

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once(dirname(__FILE__) . '/../imu.php');
require_once(IMu::$lib . '/module.php');

class IMuLuts extends IMuModule
{
	public function
	__construct($session = false)
	{
		parent::__construct('eluts', $session);

		$this->name = 'Module::Luts';
		unset($this->create);
	}

	public function
	lookup($name, $langid, $level, $keys = false)
	{
		$params = array();
		$params['name'] = $name;
		$params['langid'] = $langid;
		$params['level'] = $level;
		if ($keys !== false)
			$params['keys'] = $keys;
		return $this->call('lookup', $params);
	}
}
?>
