<?php
/*
Title: strings

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';

class IMuStrings
{
	public static $defaultLang = 'en';
	public static $defaultCode = 500;

	public static function
	value($id, $lang = '')
	{
		$string = self::string($id);
		if (is_null($string))
			return $id;

		if ($lang == '')
			$lang = IMu::$lang;

		if (array_key_exists($lang, $string))
			return $string[$lang];

		if ($lang == self::$defaultLang)
			return $id;

		if (array_key_exists(self::$defaultLang, $string))
			return $string[self::$defaultLang];

		return $id;
	}

	public static function
	code($id)
	{
		$string = self::string($id);
		if (is_null($string))
			return self::$defaultCode;

		if (array_key_exists('code', $string))
			return $string['code'];

		return self::$defaultCode;
	}

	public static function
	string($id)
	{
		if (! isset(self::$strings))
			self::load();

		if (! array_key_exists($id, self::$strings))
			return null;

		return self::$strings[$id];
	}

	private static $strings;

	private static function
	load()
	{
		self::$strings = array();

/*
		$doc = new DOMDocument;
		$file = realpath(IMu::$shared . '/strings.xml');
		if (! $doc->load($file))
			return;

		$elements = $doc->getElementsByTagName('string');
		foreach ($elements as $element)
		{
			if (! $element->hasAttribute('id'))
				continue;

			$string = array();

			$id = $element->getAttribute('id');
			$string['id'] = $id;
			if ($element->hasAttribute('code'))
				$string['code'] = $element->getAttribute('code');

			$values = $element->getElementsByTagName('value');
			foreach ($values as $value)
			{
				if (! $value->hasAttribute('lang'))
					continue;

				$lang = $value->getAttribute('lang');
				$text = $value->textContent;

				$string[$lang] = $text;
			}

			self::$strings[$id] = $string;
		}
*/
	}
}
?>
