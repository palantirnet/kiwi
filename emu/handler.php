<?php
/*
Title: handler

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/session.php';

class IMuHandler
{
	public function
	__construct($session = false)
	{
		if ($session === false)
			$this->session = new IMuSession;
		else
			$this->session = $session;

		unset($this->name);
		unset($this->create);
		unset($this->id);
		unset($this->destroy);
		unset($this->lang);
	}

	public $session;
	public $name;
	public $create;
	public $id;
	public $destroy;
	public $lang;

	public function
	call($method, $params = null)
	{
		$request = array();
		$request['method'] = $method;
		$request['params'] = $params;
		$response = $this->request($request);
		return $response['result'];
	}

	public function
	close()
	{
		$this->destroy = true;
		return $this->request(array());
	}

	public function
	request($request)
	{
		if (isset($this->id))
			$request['id'] = $this->id;
		else if (isset($this->name))
		{
			$request['name'] = $this->name;
			if (isset($this->create))
				$request['create'] = $this->create;
		}
		if (isset($this->destroy))
			$request['destroy'] = $this->destroy;
		if (isset($this->lang))
			$request['language'] = $this->lang;

		$response = $this->session->request($request);

		if (array_key_exists('id', $response))
			$this->id = $response['id'];

		return $response;
	}
}
?>
