<?php
/*
Title: session

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/handler.php';
require_once IMu::$lib . '/stream.php';
require_once IMu::$lib . '/trace.php';

/*
Class: IMuSession
	Manages a connection to a server.
*/
class IMuSession
{
	public static $defaultHost = '127.0.0.1';
	public static $defaultPort = 40000;

	public function
	__construct($host = false, $port = false)
	{
		global $_ENV;

		if ($host !== false)
			$this->host = $host;
		else
			$this->host = self::$defaultHost;

		if ($port !== false)
			$this->port = $port;
		else
			$this->port = self::$defaultPort;

		unset($this->connection);
		$this->socket = false;
		$this->stream = false;
	}

	public $host;
	public $port;
	public $context;
	public $suspend;
	public $close;
	public $connection;

	/*
	Method: connect
		Creates a new connection to a server.

	Exceptions:
		SessionConnect - the connection could not be established.
	*/
	public function
	connect()
	{
		if ($this->socket !== false)
			return;

		trace(2, "connecting to $this->host:$this->port");
		$this->socket = @fsockopen($this->host, $this->port, $errno, $errstr);
		if ($this->socket === false)
			raise(500, 'SessionConnect', $this->host, $this->port, $errstr);
		trace(2, 'connected ok');
		$this->stream = new IMuStream($this->socket);
	}

	/*
	Method: disconnect
		Closes an existing server connection.
	*/
	public function
	disconnect()
	{
		if ($this->socket === false)
			return;

		trace(2, 'closing connection');
		try
		{
			fclose($this->socket);
		}
		catch (Exception $error)
		{
		}
		$this->socket = false;
		$this->stream = false;
	}

	public function
	login($login, $password = null, $spawn = true)
	{
		$request = array();
		$request['login'] = $login;
		$request['password'] = $password;
		if ($spawn)
			$request['spawn'] = true;

		$this->request($request);
	}

	public function
	getModules()
	{
		$request = array();
		$request['name'] = 'System';
		$request['method'] = 'getModules';

		$result = $this->request($request);
		return $result;
	}

	public function
	getTableSchema($table)
	{
		$request = array();
		$request['name'] = 'System';
		$request['method'] = 'getTableSchema';
		$request['params'] = $table;

		$result = $this->request($request);
		return $result;
	}

	/*
	Method: request
		Submits a low-level request to the server. This method is
		usually not called directly.

	Parameters:
		$request - (array) Set of parameters making up the request.

	Returns:
		(array) The response.
	*/
	public function
	request($request)
	{
		$this->connect();

		if (isset($this->context))
			$request['context'] = $this->context;
		if (isset($this->suspend))
			$request['suspend'] = $this->suspend;
		if (isset($this->close))
			$request['close'] = $this->close;

		if (isset($this->connection))
			$request['connection'] = $this->connection;

		$this->stream->put($request);
		$response = $this->stream->get();

		if (array_key_exists('reconnect', $response))
			$this->port = $response['reconnect'];
		if (array_key_exists('context', $response))
			$this->context = $response['context'];

		$disconnect = false;
		if (isset($this->close))
			$disconnect = true;
		else if (isset($this->connection))
		{
			if ($this->connection == 'close')
				$disconnect = true;
			else if ($this->connection == 'suspend')
				$disconnect = true;
		}
		if ($disconnect)
			$this->disconnect();

		if ($response['status'] == 'error')
		{
			trace(2, 'server error');
			$code = 500;
			if (array_key_exists('code', $response))
				if (! is_null($response['code']))
					$code = $response['code'];
			$exception = new IMuException($code);
			if (array_key_exists('error', $response))
				$exception->id = $response['error'];
			else
				$exception->id = $response['id'];
			$exception->args = $response['args'];
			throw $exception;
		}

		return $response;
	}

	protected $socket;
	protected $stream;
}
?>
