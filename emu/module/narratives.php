<?php
/*
Title: module/narratives

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once(dirname(__FILE__) . '/../imu.php');
require_once(IMu::$lib . '/module.php');

class IMuNarratives extends IMuModule
{
	public function
	__construct($session = false)
	{
		parent::__construct('enarratives', $session);

		$this->name = 'Module::Narratives';
		unset($this->create);
	}

	public function
	findMaster()
	{
		return $this->call('findMaster');
	}
}
?>
