<?php
/*
Title: service

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/handler.php';
require_once IMu::$lib . '/session.php';
require_once IMu::$lib . '/trace.php';

class IMuService
{
	public $dir;
	public $config;
	public $params;

	public function
	__construct($dir)
	{
		$this->dir = $dir;

		$this->params = array();
		global $_GET;
		foreach ($_GET as $name => $value)
			$this->params[$name] = $value;
		global $_POST;
		foreach ($_POST as $name => $value)
			$this->params[$name] = $value;

		/* Configure */
		$config = array();

		/* ... defaults */
		$config['host'] = IMuSession::$defaultHost;
		$config['port'] = IMuSession::$defaultPort;

		/* ... service-specific */
		$this->loadConfig($config);

		$this->config = $config;
	}

	public function
	extractParam($name, $default = false)
	{
		if (! array_key_exists($name, $this->params))
			return $default;
		$value = $this->params[$name];
		unset($this->params[$name]);
		return $value;
	}

	public function
	getParam($name, $default = false)
	{
		if (! array_key_exists($name, $this->params))
			return $default;
		return $this->params[$name];
	}

	public function
	hasParam($name)
	{
		return array_key_exists($name, $this->params);
	}

	public function
	setParam($name, $value)
	{
		$this->params[$name] = $value;
	}

	public function
	process()
	{
		/* Do nothing by default */
	}

	/* Protected */
	protected $session;

	protected function
	connect()
	{
		$this->session = new IMuSession;
		$this->session->host = $this->config['host'];
		$this->session->port = $this->config['port'];
		$this->session->connect();
	}

	protected function
	disconnect()
	{
		$this->session->disconnect();
	}

	protected function
	loadConfig(&$config)
	{
		@include $this->dir . '/config.php';
	}
}
?>
