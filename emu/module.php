<?php
/*
Title: module

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/handler.php';

class IMuModule extends IMuHandler
{
	public function
	__construct($table, $session = false)
	{
		parent::__construct($session);

		$this->name = 'Module';
		$this->create = $table;

		$this->table = $table;
	}

	public function
	addFetchSet($name, $columns)
	{
		$args = array();
		$args['name'] = $name;
		$args['columns'] = $columns;
		return $this->call('addFetchSet', $args);
	}

	public function
	addFetchSets($sets)
	{
		return $this->call('addFetchSets', $sets);
	}

	public function
	addSearchAlias($name, $columns)
	{
		$args = array();
		$args['name'] = $name;
		$args['columns'] = $columns;
		return $this->call('addSearchName', $args);
	}

	public function
	addSearchAliases($names)
	{
		return $this->call('addSearchAliases', $names);
	}

	public function
	addSortSet($name, $columns)
	{
		$args = array();
		$args['name'] = $name;
		$args['columns'] = $columns;
		return $this->call('addSortSet', $args);
	}

	public function
	addSortSets($sets)
	{
		return $this->call('addSortSets', $sets);
	}

	public function
	fetch($flag, $offset, $count, $columns = false)
	{
		$args = array();
		$args['flag'] = $flag;
		$args['offset'] = $offset;
		$args['count'] = $count;
		if ($columns !== false)
			$args['columns'] = $columns;
		$data = $this->call('fetch', $args);

		$result = new IMuModuleFetchResult;
		$result->hits = $data['hits'];
		$result->rows = $data['rows'];
		return $result;
	}

	public function
	findKey($key)
	{
		$data = $this->call('findKey', $key);
		return $data + 0;
	}

	public function
	findKeys($keys)
	{
		$data = $this->call('findKeys', $keys);
		return $data + 0;
	}

	public function
	findTerms($terms)
	{
		$data = $this->call('findTerms', $terms);
		return $data + 0;
	}

	public function
	insert($values, $columns = false)
	{
		$args = array();
		$args['values'] = $values;
		if ($columns !== false)
			$args['columns'] = $columns;
		$data = $this->call('insert', $args);
		return $data;
	}

	public function
	remove($flag, $offset, $count = false)
	{
		$args = array();
		$args['flag'] = $flag;
		$args['offset'] = $offset;
		if ($count !== false)
			$args['count'] = $count;
		$data = $this->call('remove', $args);
		return $data + 0;
	}

	public function
	restoreFromFile($file)
	{
		$args = array();
		$args['file'] = $file;
		$data = $this->call('restoreFromFile', $args);
		return $data + 0;
	}

	public function
	restoreFromTemp($file)
	{
		$args = array();
		$args['file'] = $file;
		$data = $this->call('restoreFromTemp', $args);
		return $data + 0;
	}

	public function
	sort($columns, $flags = false, $langid = false)
	{
		$args = array();
		$args['columns'] = $columns;
		if ($flags !== false)
			$args['flags'] = $flags;
		if ($langid !== false)
			$args['langid'] = $langid;
		$data = $this->call('sort', $args);
		return $data;
	}

	public function
	update($flag, $offset, $count, $values, $columns = false)
	{
		$args = array();
		$args['flag'] = $flag;
		$args['offset'] = $offset;
		$args['count'] = $count;
		$args['values'] = $values;
		if ($columns !== false)
			$args['columns'] = $columns;
		$data = $this->call('update', $args);
		return $data;
	}

	protected $table;
}

class IMuModuleFetchResult
{
	public $hits;
	public $rows;
}
?>
