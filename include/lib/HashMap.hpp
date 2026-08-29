#ifndef HMAP_H
# define HMAP_H

#include <utility>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include "StringView.hpp"

template <typename T>
struct Hash
{
	std::size_t operator()(const T& val) const { return static_cast<size_t>(val); }
};

template <typename T>
struct EqualTo
{
	bool operator()(const T& lhs, const T& rhs) const { return lhs == rhs; }
};

std::size_t djb2(const char *s, const size_t len);

template <>
struct Hash<StringView>
{
	size_t operator()(const StringView& val) const { return djb2(val.c_str(), val.length()); }
};

template <>
struct Hash<std::string>
{
	size_t operator()(const std::string& val) const { return djb2(val.c_str(), val.length()); }
};

#ifndef HMAP_DEFAULT_CAPACITY
#define HMAP_DEFAULT_CAPACITY 128
#endif

#ifndef HMAP_DEFAULT_LOAD_FACTOR
#define HMAP_DEFAULT_LOAD_FACTOR 0.8f
#endif

template <bool IsConst, class PairT>
struct IteratorTraits;

template <class PairT>
struct IteratorTraits<false, PairT>
{
	typedef PairT& reference;
	typedef PairT* pointer;
};

template <class PairT>
struct IteratorTraits<true, PairT>
{
	typedef const PairT& reference;
	typedef const PairT* pointer;
};

template <
	class KeyT,
	class ValT,
	class HashT = Hash<KeyT>,
	class KeyEqualT = EqualTo<KeyT>
	>
class HashMap
{
	struct Slot;

    Slot		*_data;
	std::size_t	_capacity;
	std::size_t	_count;
	float		_max_load_factor;
	HashT		_hash;
	KeyEqualT	_equal;

	enum SlotState
	{ SLOT_EMPTY	= 0
	, SLOT_DELETED	= 1
	, SLOT_OCCUPIED	= 2
	};

	struct Slot
	{
		std::size_t hash;
		std::pair<KeyT, ValT> content;

		Slot() : hash(0) {}
	};

	Slot *_get(const KeyT &key) const
	{
		std::size_t	hash;
	
		hash = _hash(key);
		hash |= 2;

		const std::size_t first_slot_index = hash % _capacity;
		Slot *slot = NULL;
	
		for (std::size_t i = 0; i < _capacity; ++i)
		{
			slot = &_data[(first_slot_index + i) % _capacity];
			if (slot->hash == SLOT_EMPTY)
				return NULL;
			if (slot->hash == hash && _equal(slot->content.first, key))
				return slot;
		}
		return NULL;
	}
	
	Slot *_probe_insert(const KeyT& key, std::size_t hash)
	{
		const std::size_t first_slot_index = hash % _capacity;
		Slot *slot = NULL;
		Slot *tomb = NULL;

		for (std::size_t i = 0; i < _capacity; ++i)
		{
			slot = &_data[(first_slot_index + i) % _capacity];
			if (slot->hash == hash && _equal(slot->content.first, key))
				return (slot);
			if (tomb == NULL && slot->hash == SLOT_DELETED)
				tomb = slot;
			else if (tomb != NULL && slot->hash == SLOT_EMPTY)
				return (tomb);
			else if (slot->hash == SLOT_EMPTY)
				return (slot);
		}
		return tomb;
	}

	std::pair<Slot *, bool> _insert(const KeyT& key, const ValT& val)
	{
		std::size_t	hash;
		Slot		*slot;

		if (static_cast<float>((_count + 1)) / _capacity >= _max_load_factor)
			rehash(_capacity * 2);
		hash = _hash(key);
		hash |= 2;
		slot = _probe_insert(key, hash);
		if (slot == NULL)
		{
			rehash(_capacity * 2);
			return _insert(key, val);
		}
		if (slot->hash == SLOT_EMPTY || slot->hash == SLOT_DELETED)
		{
			slot->hash = hash;
			slot->content.first = key;
			slot->content.second = val;
			_count++;
			return std::make_pair(slot, true);
		}
		return std::make_pair(slot, false);
	}

	Slot *_probe_erase(const KeyT& key, std::size_t hash)
	{
		const std::size_t first_slot_index = hash % _capacity;
		Slot* slot = NULL;
	
		for (std::size_t i = 0; i < _capacity; ++i)
		{
			slot = &_data[(first_slot_index + i) % _capacity];
			if (slot->hash == SLOT_EMPTY)
				return NULL;
			if (slot->hash == hash && _equal(slot->content.first, key))
				return slot;
		}
		return NULL;
	}

	Slot *_end() const { return _data + _capacity; }
	Slot *_begin() const
	{
		if (!_count)
			return _end();
		for (std::size_t i = 0; i  < _capacity; i++)
		{
			if (_data[i].hash >= SLOT_OCCUPIED)
				return &_data[i];
		}
		return _end();
	}

public:
	template <bool IsConst>
	class Iterator;

	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef std::pair<const KeyT, ValT> value_type;
	typedef HashT hasher;
	typedef KeyEqualT key_equal;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef Iterator<false> iterator;
	typedef Iterator<true> const_iterator;

    HashMap() : _data(new Slot[HMAP_DEFAULT_CAPACITY]),
		_capacity(HMAP_DEFAULT_CAPACITY),
		_count(0),
   		_max_load_factor(HMAP_DEFAULT_LOAD_FACTOR) {}

    HashMap(std::size_t capacity) : _count(0),
   		_max_load_factor(HMAP_DEFAULT_LOAD_FACTOR)
	{
		_capacity = (capacity > 0 ? capacity : 1);
		_data = new Slot[_capacity];
	}

    ~HashMap() { delete[] _data; }

    HashMap(const HashMap& other)
		: _data(NULL), _capacity(other._capacity),
		_count(other._count), _max_load_factor(other._max_load_factor)
	{
		_data = new Slot[_capacity];
		for (size_type i = 0; i < other._capacity; ++i)
		{
			const Slot& slot = other._data[i];
			if (slot.hash >= SLOT_OCCUPIED)
				_data[i] = slot;
		}
	}

	void swap(HashMap& other)
	{
		std::swap(_data, other._data);
		std::swap(_capacity, other._capacity);
		std::swap(_count, other._count);
		std::swap(_max_load_factor, other._max_load_factor);
		std::swap(_hash, other._hash);
		std::swap(_equal, other._equal);
	}

	HashMap& operator=(HashMap other)
	{
		swap(other);
		return *this;
	}

    ValT& operator[](const KeyT& key)
	{
		Slot *slot = _get(key);

		if (slot == NULL)
		{
			std::pair<Slot *, bool> res = _insert(key, ValT());
			if (!res.second)
				throw std::runtime_error("HashMap::operator[]: unreachable");
			return res.first->content.second;
		}
		return slot->content.second;
	}

	ValT& at(const KeyT& key)
	{
		Slot *slot = _get(key);
		if (slot == NULL)
			throw std::out_of_range("HashMap::at()");
		return slot->content.second;
	}

	const ValT& at(const KeyT& key) const
	{
		Slot *slot = _get(key);
		if (slot == NULL)
			throw std::out_of_range("HashMap::at()");
		return slot->content.second;
	}

	iterator find(const KeyT& key)
	{
		Slot *slot = _get(key);
		if (slot == NULL)
			return end();
		return iterator(slot, _end());
	}

	const_iterator find(const KeyT& key) const
	{
		Slot *slot = _get(key);
		if (slot == NULL)
			return end();
		return const_iterator(slot, _end());
	}

	std::pair<iterator, bool> insert(const value_type& value)
	{
		std::pair<Slot *, bool> slot = _insert(value.first, value.second);

		if (!slot.second)
			return std::make_pair(end(), false);
		return std::make_pair(iterator(slot.first, _end()), true);
	}

	size_type erase(const KeyT& key)
	{
		std::size_t hash;
		Slot *slot;
	
		hash = _hash(key);
		hash |= 2;
		slot = _probe_erase(key, hash);
		if (slot == NULL)
			return 0;
		slot->hash = SLOT_DELETED;
		if (_count)
			_count--;
		return 1;
	}

	iterator erase(const_iterator pos)
	{
		const KeyT& key = pos->first;
		iterator next = static_cast<iterator>(++pos);
		std::size_t hash;
		Slot *slot;
	
		hash = _hash(key);
		hash |= 2;
		slot = _probe_erase(key, hash);
		if (slot == NULL)
			return next;
		slot->hash = SLOT_DELETED;
		if (_count)
			_count--;
		return next;
	}

	void reserve(size_type count)
	{
		if (count == 0)
			count = 1;
		rehash(static_cast<std::size_t>(std::ceil(count / _max_load_factor)));
	}

	void rehash(size_type count = 0)
	{
		std::size_t new_cap = count;
		std::size_t min_buckets = static_cast<std::size_t>(std::ceil(_count / _max_load_factor));

		if (count < min_buckets)
			new_cap = min_buckets;
		else if (!count)
			new_cap = _capacity;

		HashMap res(new_cap);

		for (std::size_t i = 0; i < _capacity; ++i)
		{
			if (_data[i].hash >= SLOT_OCCUPIED)
				res._insert(_data[i].content.first, _data[i].content.second);
		}
		swap(res);
	}

	size_type size() const { return _count; }
	bool empty() const { return _count == 0; }

	float max_load_factor() const { return _max_load_factor; }
	void max_load_factor(float ml) { _max_load_factor = ml; }
	float load_factor() const { return _count / static_cast<float>(_capacity); }

	iterator end() { return iterator(_end(), _end()); }
	iterator begin() { return iterator(_begin(), _end()); }
	const_iterator end() const { return const_iterator(_end(), _end()); }
	const_iterator begin() const { return const_iterator(_begin(), _end()); }

	template <bool IsConst>
	class Iterator
	{
		Slot *_pos;
		Slot *_end;

	public:
		typedef typename IteratorTraits<IsConst, value_type>::reference val_ref;
		typedef typename IteratorTraits<IsConst, value_type>::pointer val_ptr;

		Iterator(Slot *pos, Slot *end) : _pos(pos), _end(end) {}

		template <bool TrueOrFalse>
		Iterator(const Iterator<TrueOrFalse>& other) : _pos(other._pos), _end(other._end) {}

		template <bool TrueOrFalse>
		Iterator& operator=(const Iterator<TrueOrFalse>& other)
		{
			if (this != &other)
			{
				_pos = other._pos;
				_end = other._end;
			}
			return *this;
		}
		~Iterator() {}

		val_ref operator*() { return reinterpret_cast<val_ref>(_pos->content); }
		val_ptr operator->() { return reinterpret_cast<val_ptr>(&_pos->content); }

		Iterator& operator++()
		{
		    ++_pos;
		    while (_pos != _end && !(_pos->hash >= SLOT_OCCUPIED))
		        ++_pos;
		    return *this;
		}

		Iterator operator++(int)
		{
			Iterator old(*this);
			++(*this);
		    return old;
		}

		template <bool TrueOrFalse>
		bool operator==(const Iterator<TrueOrFalse>& rhs) const { return _pos == rhs._pos; }

		template <bool TrueOrFalse>
		bool operator!=(const Iterator<TrueOrFalse>& rhs) const { return _pos != rhs._pos; }

		template <bool TrueOrFalse>
		bool operator<=(const Iterator<TrueOrFalse>& rhs) const { return _pos <= rhs._pos; }

		template <bool TrueOrFalse>
		bool operator<(const Iterator<TrueOrFalse>& rhs) const { return _pos < rhs._pos; }
	};
};


#endif
