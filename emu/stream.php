<?php
/*
Title: stream

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/exception.php';
require_once IMu::$lib . '/trace.php';

/*
Class: IMuStream
	Handles low-level communication between client and server. A stream has
	only two methods:

	put	- sends information to the server.
	get - receives information from the server.

	The information is serialized as a modified version of JSON (JavaScript
	Object Notation). The primary modification is to allow the direct
	transmission of binary data.

	It is normally not necessary to create a IMuStream object directly. A stream
	object is created automatically by a IMuSession.
*/
class IMuStream
{
	public static $blockSize = 8192;

	/*
	Method: __construct
	
	Parameters:
		$handle	- handle to an open socket connection
	*/
	public function
	__construct($handle)
	{
		$this->handle = $handle;
		$this->token = '';
		$this->string = null;
		$this->file = null;

		$this->count = 0;
		$this->buffer = '';
		$this->length = 0;
	}

	/*
	Method: get
		Retrieves the next serialized object from the connection.

	Returns:
		(mixed) The unserialized data.
	*/
	public function
	get()
	{
		$this->getNext();
		$this->getToken();
		return $this->getValue();
	}

	/*
	Method: put
		Sends a value to the other end of the connection. The value is
		serialized and transmitted.

	Parameters:
		$value - (mixed) The value to be sent.
	*/
	public function
	put($value)
	{
		$this->putValue($value, 0);
		$this->putLine();
		$this->putSocket();
	}

	private $handle;
	private $input;
	private $token;
	private $value;

	private function
	getValue()
	{
		if ($this->token == 'end')
			return null;
		if ($this->token == 'string')
			return $this->string;
		if ($this->token == 'number')
			return $this->string + 0;
		if ($this->token == '{')
		{
			$array = array();
			$this->getToken();
			while ($this->token != '}')
			{
				if ($this->token == 'string')
					$name = $this->string;
				else if ($this->token == 'identifier')
					// Extension - allow simple identifiers
					$name = $this->string;
				else
					raise(500, 'StreamSyntaxName');

				$this->getToken();
				if ($this->token != ':')
					raise(500, 'StreamSyntaxColon');

				$this->getToken();
				$array[$name] = $this->getValue();

				$this->getToken();
				if ($this->token == ',')
					$this->getToken();
			}
			return $array;
		}
		if ($this->token == '[')
		{
			$array = array();
			$this->getToken();
			while ($this->token != ']')
			{
				$array[] = $this->getValue();

				$this->getToken();
				if ($this->token == ',')
					$this->getToken();
			}
			return $array;
		}
		if ($this->token == 'true')
			return true;
		if ($this->token == 'false')
			return false;
		if ($this->token == 'null')
			return null;
		if ($this->token == 'binary')
			return $this->file;
		raise(500, 'StreamSyntaxToken', $this->token);
	}

	private function
	getToken()
	{
		while (ctype_space($this->next))
			$this->getNext();
		$this->string = '';
		if ($this->next == '"')
		{
			$this->token = 'string';
			$this->getNext();
			while ($this->next != '"')
			{
				if ($this->next == '\\')
				{
					$this->getNext();
					if ($this->next == 'b')
						$this->next = "\b";
					else if ($this->next == 'f')
						$this->next = "\f";
					else if ($this->next == 'n')
						$this->next = "\n";
					else if ($this->next == 'r')
						$this->next = "\r";
					else if ($this->next == 't')
						$this->next = "\t";
					else if ($this->next == 'u')
					{
						$this->getNext();
						$num = "";
						for ($i = 0; $i < 4; $i++)
						{
							if (! ctype_xdigit($this->next))
								break;
							$num .= $this->next;
							$this->getNext();
						}
						if ($num == '')
							raise(500, 'StreamSyntaxUnicode');
						$this->next = chr($num);
					}
				}
				$this->string .= $this->next;
				$this->getNext();
			}
			$this->getNext();
		}
		else if (ctype_digit($this->next) || $this->next == '-')
		{
			$this->token = 'number';
			$this->string .= $this->next;
			$this->getNext();
			while (ctype_digit($this->next))
			{
				$this->string .= $this->next;
				$this->getNext();
			}
			if ($this->next == '.')
			{
				$this->string .= $this->next;
				$this->getNext();
				while (ctype_digit($this->next))
				{
					$this->string .= $this->next;
					$this->getNext();
				}
				if ($this->next == 'e' || $this->next == 'E')
				{
					$this->string .= 'e';
					$this->getNext();
					if ($this->next == '-')
					{
						$this->string .= '-';
						$this->getNext();
					}
					while (ctype_digit($this->next))
					{
						$this->string .= $this->next;
						$this->getNext();
					}
				}
			}
		}
		else if (ctype_alpha($this->next) || $this->next == '_')
		{
			$this->token = 'identifier';
			while (ctype_alnum($this->next) || $this->next == '_')
			{
				$this->string .= $this->next;
				$this->getNext();
			}
			$lower = strtolower($this->string);
			if ($lower == 'true')
				$this->token = 'true';
			else if ($lower == 'false')
				$this->token = 'false';
			else if ($lower == 'null')
				$this->token = 'null';
		}
		else if ($this->next == '*')
		{
			// Extension - allow embedded binary data
			$this->token = 'binary';
			$this->getNext();
			while (ctype_digit($this->next))
			{
				$this->string .= $this->next;
				$this->getNext();
			}
			if ($this->string == '')
				raise(500, 'StreamSyntaxBinary');
			$size = $this->string + 0;
			while ($this->next != "\n")
				$this->getNext();

			// Read data into temporary file
			$temp = tmpfile();
			$left = $size;
			while ($left > 0)
			{
				$read = self::$blockSize;
				if ($read > $left)
					$read = $left;
				$data = fread($this->handle, $read);
				if ($data === false)
					raise(500, 'StreamInput');
				$done = strlen($data);
				if ($done == 0)
					raise(500, 'StreamEOF');
				fwrite($temp, $data);
				$left -= $done;
			}
			fseek($temp, 0, SEEK_SET);
			$this->file = $temp;

			$this->getNext();
		}
		else
		{
			$this->token = $this->next;
			$this->getNext();
		}
	}

	private function
	getNext()
	{
		$this->next = fgetc($this->handle);
		if ($this->next === false)
			raise(500, 'StreamEOF');
		return $this->next;
	}

	private function
	putValue($value, $indent)
	{
		$type = gettype($value);
		if ($type == 'NULL')
			$this->putData('null');
		else if ($type == 'string')
			$this->putString($value);
		else if ($type == 'integer')
			$this->putData(sprintf('%d', $value));
		else if ($type == 'double')
			$this->putData(sprintf('%g', $value));
		else if ($type == 'object')
			$this->putObject(get_object_vars($value), $indent);
		else if ($type == 'array')
		{
			/* A bit magical.
			**
			** If the array is empty treat it as an array rather than
			** a JSON object. Also, if the keys of the array are exactly
			** from 0 to count - 1 then put a JSON array otherwise put a
			** JSON object.
			*/
			if (empty($value))
				$this->putArray($value, $indent);
			else if (array_keys($value) === range(0, count($value) - 1))
				$this->putArray($value, $indent);
			else
				$this->putObject($value, $indent);
		}
		else if ($type == 'boolean')
			$this->putData($value ? 'true' : 'false');
		else if ($type == 'resource')
		{
			if (fseek($value, 0, SEEK_END) < 0)
				raise(500, 'StreamFileSeek');
			$size = ftell($value);
			if (fseek($value, 0, SEEK_SET) < 0)
				raise(500, 'StreamFileSeek');
			$this->putData(sprintf('*%d', $size));
			$this->putLine();

			$left = $size;
			while ($left > 0)
			{
				$need = self::$blockSize;
				if ($need > $left)
					$need = $left;
				$data = fread($value, $need);
				if ($data === false)
					raise(500, 'StreamFileRead');
				$done = strlen($data);
				if ($done == 0)
					break;
				$this->putData($data);
				$left -= $done;
			}
			if ($left > 0)
			{
				/* The file did not contain enough bytes
				** so the output is padded with nulls
				*/
				while ($left > 0)
				{
					$need = self::$blockSize;
					if ($need > $left)
						$need = $left;
					$data = str_repeat(chr(0), $need);
					$this->putData($data);
					$left -= $need;
				}
			}
		}
	}

	private function
	putString($value)
	{
		$this->putData('"');
		$value = preg_replace('/\\\\/', '\\\\\\\\', $value);
		$value = preg_replace('/"/', '\\"', $value);
		$this->putData($value);
		$this->putData('"');
	}

	private function
	putObject($array, $indent)
	{
		$this->putData('{');
		$this->putLine();
		$count = count($array);
		$i = 0;
		foreach ($array as $name => $value)
		{
			$this->putIndent($indent + 1);
			$this->putString($name);
			$this->putData(' : ');
			$this->putValue($value, $indent + 1);
			if ($i < $count - 1)
				$this->putData(',');
			$this->putLine();
			$i++;
		}
		$this->putIndent($indent);
		$this->putData('}');
	}

	private function
	putArray($array, $indent)
	{
		$this->putData('[');
		$this->putLine();
		$count = count($array);
		$i = 0;
		foreach ($array as $value)
		{
			$this->putIndent($indent + 1);
			$this->putValue($value, $indent + 1);
			if ($i < $count - 1)
				$this->putData(',');
			$this->putLine();
			$i++;
		}
		$this->putIndent($indent);
		$this->putData(']');
	}

	private function
	putIndent($indent)
	{
		$string = '';
		for ($i = 0; $i < $indent; $i++)
			$string .= "\t";
		$this->putData($string);
	}

	private function
	putLine()
	{
		$this->putData("\r\n");
	}

	private function
	putData($data)
	{
		/* Uncomment to debug low-level output
		$copy = preg_replace('/\n/', '\\n', $data);
		$copy = preg_replace('/\r/', '\\r', $copy);
		$copy = preg_replace('/\t/', '\\t', $copy);
		trace(3, 'putting >>%s<<', $copy);
		*/

		$this->buffer .= $data;
		$this->length += strlen($data);
		if ($this->length >= self::$blockSize)
			$this->putSocket();
	}

	private function
	putSocket()
	{
		if ($this->length > 0)
		{
			trace(4, 'flushing buffer (%d bytes)', $this->length);
			$copy = $this->buffer;
//			$copy = preg_replace('/\n/', '\\n', $copy);
//			$copy = preg_replace('/\r/', '\\r', $copy);
			$copy = preg_replace('/\r/', '', $copy);
//			$copy = preg_replace('/\t/', '\\t', $copy);
			trace(4, "putting: %s\n", $copy);

			$this->count += $this->length;
			while ($this->length > 0)
			{
				$wrote = fwrite($this->handle, $this->buffer);
				if ($wrote === false)
					raise(500, 'StreamWriteError');
				if ($wrote == 0)
					raise(500, 'StreamWriteError');
				$this->buffer = substr($this->buffer, $wrote);
				$this->length -= $wrote;
			}
			fflush($this->handle);

			trace(4, 'total bytes written so far: %d', $this->count);
		}
		$this->buffer = '';
		$this->length = 0;
	}
}

?>
