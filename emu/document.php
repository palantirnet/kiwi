<?php
/*
Title: document

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';

/*
Class: IMuDocument
	Extends a DOMDcoument.
*/
class IMuDocument extends DOMDocument
{
	public function
	__construct($encoding = false)
	{
		parent::__construct('1.0');
		if ($encoding !== false)
			$this->encoding = $encoding;

		$this->xpath = null;

		$this->formatOutput = true;
		$this->stack = array($this);
		$this->options = array();
	}

	public function
	endDocument()
	{
		while (count($this->stack) > 1)
			$this->endElement();
	}

	public function
	endElement()
	{
		array_shift($this->stack);
	}

	public function
	getTagOption($tag, $name, $default = false)
	{
		if (! array_key_exists($tag, $this->options))
			return $default;
		if (! array_key_exists($name, $this->options[$tag]))
			return $default;
		return $this->options[$tag][$name];
	}

	public function
	hasTagOption($tag, $name)
	{
		if (! array_key_exists($tag, $this->options))
			return false;
		return array_key_exists($name, $this->options[$tag]);
	}

	public function
	setTagOption($tag, $name, $value)
	{
		if (! array_key_exists($tag, $this->options))
			$this->options[$tag] = array();
		$this->options[$tag][$name] = $value;
	}

	public function
	startElement($name)
	{
		$child = $this->createElement($name);
		$parent = $this->stack[0];
		$parent->appendChild($child);
		array_unshift($this->stack, $child);
		return $child;
	}

	public function
	writeElement($name, $value)
	{
		if (is_array($value))
		{
			if (array_keys($value) === range(0, count($value) - 1))
				$this->writeList($name, $value);
			else
				$this->writeHash($name, $value);
		}
		else if (is_object($value))
			$this->writeObject($name, $value);
		else
			$this->writeText($name, $value);
	}

	private $stack;
	private $options;

	private function
	writeList($tag, $list)
	{
		/* This is an ugly hack */
		if ($this->hasTagOption($tag, 'child'))
			$child = $this->getTagOption($tag, 'child');
		else if (preg_match('/(.*)s$/', $tag, $match))
			$child = $match[1];
		else if (preg_match('/(.*)_tab$/', $tag, $match))
			$child = $match[1];
		else if (preg_match('/(.*)0$/', $tag, $match))
			$child = $match[1];
		else if (preg_match('/(.*)_nesttab$/', $tag, $match))
			$child = $match[1] . '_tab';
		else
			$child = 'item';

		$this->startElement($tag);
		foreach ($list as $item)
			$this->writeElement($child, $item);
		$this->endElement();
	}

	private function
	writeHash($tag, $hash)
	{
		$this->startElement($tag);
		foreach ($hash as $name => $value)
			$this->writeElement($name, $value);
		$this->endElement();
	}

	private function
	writeObject($tag, $object)
	{
		$this->startElement($tag);
		foreach (get_object_vars($object) as $name => $value)
			$this->writeElement($name, $value);
		$this->endElement();
	}

	private function
	writeText($tag, $text)
	{
		$parent = $this->startElement($tag);
		if ($text !== '')
		{
			$type = gettype($text);
			if ($type == 'boolean')
				$text = $text ? 'true' : 'false';
			else
				$text = utf8_encode($text);

			/* If the tag is on the raw list generate raw text */
			if ($this->getTagOption($tag, 'raw', false))
			{
				$child = $this->createDocumentFragment();
				@$child->appendXML($text);
			}
			else
				$child = $this->createTextNode($text);

			@$parent->appendChild($child);
		}
		$this->endElement();
	}
}
?>
