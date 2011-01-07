<?php
/*
Title: user

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/handler.php';

class IMuUser extends IMuHandler
{
	public function
	__construct($ip, $token, $session = false)
	{
		parent::__construct($session);

		$this->name = 'User';

		$this->ip = $ip;
		$this->token = $token;
	}

	public function
	fetch()
	{
		$params = $this->params();
		return $this->call('fetch', $params);
	}

	public function
	addGroup($name = false)
	{
		$params = $this->params();
		if ($name !== false)
			$params['name'] = $name;
		return $this->call('addGroup', $params);
	}

	public function
	defaultGroup($group)
	{
		$params = $this->params();
		$params['group'] = $group;
		return $this->call('defaultGroup', $params);
	}

	public function
	removeGroup($group)
	{
		$params = $this->params();
		$params['group'] = $group;
		return $this->call('removeGroup', $params);
	}

	public function
	renameGroup($group, $name = false)
	{
		$params = $this->params();
		$params['group'] = $group;
		if ($name !== false)
			$params['name'] = $name;
		return $this->call('renameGroup', $params);
	}

	public function
	removeEntry($module, $key, $group = false)
	{
		$params = $this->params();
		$params['module'] = $module;
		$params['key'] = $key;
		if ($group !== false)
			$params['group'] = $group;
		return $this->call('removeEntry', $params);
	}

	public function
	call($method, $params = null)
	{
		$result = parent::call($method, $params);
		if (array_key_exists('irn', $result))
			$this->irn = $result['irn'];
		return $result;
	}

	protected $ip;
	protected $token;
	protected $irn;

	protected function
	params()
	{
		$params = array();
		$params['ip'] = $this->ip;
		$params['irn'] = $this->irn;
		return $params;
	}
}
?>
