<?php
/*
Title: rss

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/document.php';

/*
Class: IMuRSS
*/
class IMuRSS
{
	public $category;
	public $copyright;
	public $description;
	public $encoding;
	public $language;
	public $link;
	public $title;

	public function
	__construct()
	{
		$this->category = '';
		$this->copyright = '';
		$this->description = '';
		$this->encoding = 'UTF-8';
		$this->language = '';
		$this->link = '';
		$this->title = '';

		$this->items = array();
	}

	public function
	addItem()
	{
		$item = new IMuRSSItem;
		$this->items[] = $item;
		return $item;
	}

	public function
	createXML()
	{
		$date = date('r');

		$xml = new IMuDocument($this->encoding);
		$root = $xml->startElement('rss');
		$root->setAttribute('version', '2.0');
		$xml->startElement('channel');
		$xml->writeElement('category', $this->category);
		$xml->writeElement('copyright', $this->copyright);
		$xml->writeElement('description', $this->description);
		$xml->writeElement('language', $this->language);
		$xml->writeElement('lastBuildDate', $date);
		$xml->writeElement('link', $this->link);
		$xml->writeElement('pubDate', $date);
		$xml->writeElement('title', $this->title);
		foreach ($this->items as $item)
			$item->createXML($xml);
		$xml->endDocument();

		return $xml->saveXML();
	}

	private $items;
}

class IMuRSSItem
{
	public $author;
	public $category;
	public $description;
	public $length;
	public $link;
	public $mimeType;
	public $pubDate;
	public $title;
	public $url;

	public function
	__construct()
	{
		$this->author = '';
		$this->category = '';
		$this->description = '';
		$this->length = '';
		$this->link = '';
		$this->mimeType = '';
		$this->pubDate = '';
		$this->title = '';
		$this->url = '';
	}

	public function
	createXML($xml)
	{
		$xml->startElement('item');

		$xml->writeElement('author', $this->author);
		$xml->writeElement('category', $this->category);
		$xml->writeElement('description', $this->description);

		$enclosure = $xml->startElement('enclosure');
		$enclosure->setAttribute('url', $this->url);
		$enclosure->setAttribute('length', $this->length);
		$enclosure->setAttribute('type', $this->mimeType);
		$xml->endElement();

		$xml->writeElement('guid', $this->link);
		$xml->writeElement('link', $this->link);
		$xml->writeElement('pubDate', $this->pubDate);
		$xml->writeElement('title', $this->title);

		$xml->endElement();
	}
}
?>
