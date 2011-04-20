<?php
/*
Title: trace

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';

function trace($level, $mesg)
{
	$args = func_get_args();
	array_shift($args);
	array_shift($args);
	IMuTrace::write($level, $mesg, $args);
}

class IMuTrace
{
	public static function
	getFile()
	{
		return self::$File;
	}

	public static function
	getLevel()
	{
		return self::$Level;
	}

	public static function
	getPrefix()
	{
		return self::$Prefix;
	}

	public static function
	setFile($file = null)
	{
		if (is_null($file))
			$handle = false;
		else if ($file == 'STDOUT')
			$handle = STDOUT;
		else
			$handle = @fopen($file, 'a');
		if ($handle === false)
		{
			self::$File = null;
			self::$Handle = null;
		}
		else
		{
			self::$File = $file;
			self::$Handle = $handle;
		}
	}

	public static function
	setLevel($level)
	{
		self::$Level = $level;
	}

	public static function
	setPrefix($prefix)
	{
		self::$Prefix = $prefix;
	}

	public static function
	write($level, $mesg, $args)
	{
		if (is_null(self::$Handle))
			return;
		if (is_null(self::$Level))
			return;
		if ($level > self::$Level)
			return;
		if (is_null(self::$Prefix))
			self::$Prefix = '%D %T: ';

		/* Process the arguments
		*/
		$strs = array();
		foreach ($args as $arg)
			$strs[] = print_r($arg, true);

		/* Build the prefix
		*/
		$prefix = self::$Prefix;

		/* ... time
		*/
		$y = date('Y');
		$m = date('m');
		$d = date('d');
		$D = "$y-$m-$d";

		$H = date('H');
		$M = date('i');
		$S = date('s');
		$T = "$H:$M:$S";

		/* ... process id
		*/
		$p = getmypid();

		/* ... function information
		*/
		$F = '(unknown)';
		$L = '(unknown)';
		$f = '(none)';
		$g = '(none)';
		$trace = debug_backtrace();
		$count = count($trace);
		for ($i = 0; $i < $count; $i++)
		{
			$frame = $trace[$i];
			if ($frame['file'] != __FILE__)
			{
				$F = $frame['file'];
				$L = $frame['line'];
				if ($i < $count - 1)
				{
					$frame = $trace[$i + 1];
					if (array_key_exists('class', $frame))
						$f = $frame['class'] . '::' . $frame['function'];
					else
						$f = $frame['function'];
					$g = preg_replace('/^IMu/', '', $f);
				}
				break;
			}
		}

		$prefix = preg_replace('/%y/', $y, $prefix);
		$prefix = preg_replace('/%m/', $m, $prefix);
		$prefix = preg_replace('/%d/', $d, $prefix);
		$prefix = preg_replace('/%D/', $D, $prefix);

		$prefix = preg_replace('/%H/', $H, $prefix);
		$prefix = preg_replace('/%M/', $M, $prefix);
		$prefix = preg_replace('/%S/', $S, $prefix);
		$prefix = preg_replace('/%T/', $T, $prefix);

		$prefix = preg_replace('/%p/', $p, $prefix);

		$prefix = preg_replace('/%F/', $F, $prefix);
		$prefix = preg_replace('/%L/', $L, $prefix);
		$prefix = preg_replace('/%f/', $f, $prefix);
		$prefix = preg_replace('/%g/', $g, $prefix);

		/* Build the string
		*/
		$mesg = "$mesg";
		if (count($args) > 0)
			$mesg = vsprintf($mesg, $strs);
//		$text = '';
//		foreach (explode("\n", $mesg) as $line)
//		{
//			$line = preg_replace('/\r$/', '', $line);
//			$text .= $prefix . $line . "\n";
//		}
		$text = $prefix . $mesg;
		$text = preg_replace('/\r/', '', $text);
		$text = preg_replace('/\s+$/', '', $text);
		$text .= "\n";

		/* Write it out
		*/
		if (self::$File != 'STDOUT')
		{
			/* Lock */
			if (! flock(self::$Handle, LOCK_EX))
				return;

			/* Append */
			if (fseek(self::$Handle, 0, SEEK_END) != 0)
			{
				flock(self::$Handle, LOCK_UN);
				return;
			}
		}
		fwrite(self::$Handle, $text);
		fflush(self::$Handle);
		if (self::$File != 'STDOUT')
			flock(self::$Handle, LOCK_UN);
	}

	private static $File = null;
	private static $Handle = null;
	private static $Level = null;
	private static $Prefix = null;
}
?>
