<?php
/*
Title: modules

Copyright:
	(c) 1998-2010 KE Software Pty Ltd
*/
require_once dirname(__FILE__) . '/imu.php';
require_once IMu::$lib . '/handler.php';
require_once IMu::$lib . '/result.php';

class IMuModules extends IMuHandler
{
	public $modules;

	public function
	__construct($session = false)
	{
		parent::__construct($session);

		$this->name = 'Modules';
	}

	public function
	addFetchSet($name, $set)
	{
		$args = array();
		$args['name'] = $name;
		$args['set'] = $set;
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
		return $this->call('addSearchAlias', $args);
	}

	public function
	addSearchAliases($names)
	{
		return $this->call('addSearchAliases', $names);
	}

	public function
	addSortSet($name, $set)
	{
		$args = array();
		$args['name'] = $name;
		$args['set'] = $set;
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
		$params = array();
		$params['flag'] = $flag;
		$params['offset'] = $offset;
		$params['count'] = $count;
		if ($columns !== false)
			$params['columns'] = $columns;
		$data = $this->call('fetch', $params);

		$result = new IMuModulesFetchResult;
		$result->count = $data['count'] + 0;
		$result->modules = array();
		foreach ($data['modules'] as $item)
		{
			$module = new IMuModulesFetchModule;
			$module->name = $item['name'];
			$module->index = $item['index'] + 0;
			$module->hits = $item['hits'] + 0;
			$module->rows = $item['rows'];

			$result->modules[] = $module;
		}
		if (array_key_exists('current', $data))
		{
			$result->current = new IMuModulesFetchPosition;
			$result->current->flag = $data['current']['flag'];
			$result->current->offset = $data['current']['offset'] + 0;
		}
		if (array_key_exists('prev', $data))
		{
			$result->prev = new IMuModulesFetchPosition;
			$result->prev->flag = $data['prev']['flag'];
			$result->prev->offset = $data['prev']['offset'] + 0;
		}
		if (array_key_exists('next', $data))
		{
			$result->next = new IMuModulesFetchPosition;
			$result->next->flag = $data['next']['flag'];
			$result->next->offset = $data['next']['offset'];
		}

		return $result;
	}

	public function
	findAttachments($table, $column, $key)
	{
		$args = array();
		$args['table'] = $table;
		$args['column'] = $column;
		$args['key'] = $key;
		return $this->call('findAttachments', $args);
	}

	public function
	findKeys($keys, $include = false)
	{
		$args = array();
		$args['keys'] = $keys;
		if ($include != false)
			$args['include'] = $include;
		return $this->call('findKeys', $args);
	}

	public function
	findTerms($terms, $include = false)
	{
		$args = array();
		$args['terms'] = $terms;
		if ($include != false)
			$args['include'] = $include;
		return $this->call('findTerms', $args);
	}

	public function
	getHits($module = null)
	{
		$data = $this->call('getHits', $module);
		return $data + 0;
	}

	public function
	setModules($list)
	{
		$data = $this->call('setModules', $list);
		return $data + 0;
	}

	public function
	sort($set, $flags = false, $langid = false)
	{
		$args = array();
		$args['set'] = $set;
		if ($flags !== false)
			$args['flags'] = $flags;
		if ($langid !== false)
			$args['langid'] = $langid;
		$data = $this->call('sort', $args);
		return $data;
	}
}

class IMuModulesFetchResult
{
	public $count;
	public $modules;
	public $current;
	public $prev;
	public $next;
}

class IMuModulesFetchModule
{
	public $name;
	public $index;
	public $hits;
	public $rows;
}

class IMuModulesFetchPosition
{
	public $flag;
	public $offset;
}
?>
