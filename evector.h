/*
Copyright © 2020 Siyao Wang

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#pragma once

#include <initializer_list>
#include <memory>

//#include <algorithm>

namespace wsy {

	template <typename T, class Alloc = std::allocator<T>>
	class vector {
	public:
		using value_type = T;
		using pointer = value_type * ;
		using const_pointer = const value_type*;
		using iterator = value_type * ;
		using const_iterator = const value_type*;
		using reference = value_type & ;
		using const_reference = const value_type&;
		using rvalue_reference = value_type && ;

		using size_type = std::size_t;

		vector()
			: _begin{ nullptr }, _end{ nullptr }, _cend{ nullptr } {}

		vector(size_type size)
			: _begin{ alloc.allocate(size) },
			_end{ std::uninitialized_fill_n(_begin, size, T{}) },
			_cend{ _begin + size }
		{

		}

		vector(const int& val, size_type size)
			: _begin{ alloc.allocate(size) },
			_end{ std::uninitialized_fill_n(_begin, size, val) },
			_cend{ _begin + size }
		{

		}

		vector(std::initializer_list<value_type> lst)
			: _begin{ alloc.allocate(lst.size()) },
			_end{ std::uninitialized_copy(lst.begin(), lst.end(), _begin) },
			_cend{ _begin + lst.size() }
		{

		}

		vector(const vector& v) // remains _cend and _csize
			: _begin{ alloc.allocate(v.size()) },
			_end{ std::uninitialized_copy(v.begin(), v.end(), _begin) },
			_cend{ _begin + v.size() }
		{

		}

		vector(vector&& v) //move constructor remains _cend and _csize
			: _begin{ v._begin }, _end{ v._end }, _cend{ v._cend } {
			// must not deallocate v's memory
			v._begin = nullptr;
			v._end = nullptr;
			v._cend = nullptr;
		}

		vector& operator=(const vector& v) {
			vector tmp(v);
			_M_realloc_copy(tmp.begin(), tmp.end()); //if no copy, will call v's const function
			return *this; //tmp will automatically destroied here
		}

		vector& operator=(vector&& v) { //remain _cend and _csize
			_M_alloc_refresh(v.begin(), v.end(), v._M_getStorage());
			v._M_dealloc();
			return *this;
		}

		vector& operator+=(const vector& v) { // do not remain _cend and _csize
			auto tmp_begin = alloc.allocate(size() + v.size());
			auto tmp_end = std::uninitialized_copy(begin(), end(), tmp_begin);
			tmp_end = std::uninitialized_copy(v.begin(), v.end(), tmp_end);
			auto tmp_cend = tmp_begin + size() + v.size();
			_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);

			return *this;
		}

		const_reference operator[](size_type idx) const {
			return *(_begin + idx);
		}

		reference operator[](size_type idx) {
			return const_cast<T&>(static_cast<const vector&>(*this)[idx]);
		}

		const_reference at(size_type idx) const {
			if (idx < 0 || idx >= size()) throw std::runtime_error("index out of bound");
			return *(_begin + idx);
		}

		reference at(size_type idx) {
			return const_cast<T&>(static_cast<const vector&>(*this).at(idx));
		}

		void assign(size_type size, const_reference val) {
			_M_realloc_fill(size, val);
		}

		void assign(iterator it_begin, iterator it_end) {
			_M_realloc_copy(it_begin, it_end);
		}

		void assign(std::initializer_list<value_type> lst) {
			_M_realloc_copy(lst.begin(), lst.end());
		}

		void push_back(const_reference val) {
			if (empty()) {
				_begin = alloc.allocate(1);
				alloc.construct(_begin, val);
				_end = _begin + 1;
				_cend = _end;
			}
			else if (_end == _cend) {
				//expand _cend to be double
				//expand _end by 1

				auto tmp_begin = alloc.allocate(2 * size());
				auto tmp_end = std::uninitialized_copy(begin(), end(), tmp_begin);
				alloc.construct(tmp_end++, val);
				auto tmp_cend = tmp_begin + 2 * size();

				_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
			}
			else {
				alloc.construct(_end++, val);
			}
		}

		void push_front(const_reference val) {
			if (empty()) {
				_begin = alloc.allocate(1);
				alloc.construct(_begin, val);
				_end = _begin + 1;
				_cend = _end;
			}
			else if (_end == _cend) {
				//expand _cend to be double
				//expand _end by 1

				auto tmp_begin = alloc.allocate(2 * size());
				auto tmp_end = std::uninitialized_copy(begin(), end(), tmp_begin + 1);
				alloc.construct(tmp_begin, val);
				auto tmp_cend = tmp_begin + 2 * size();

				_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
			}
			else {
				alloc.construct(_end++, value_type{});
				value_type prev = *begin();
				*_begin = val;

				for (auto it = begin() + 1; it != end(); ++it) {
					std::swap(*it, prev);
				}
			}
		}

		void pop_back() {
			if (empty()) return;
			else if (2 * size() <= capacity()) {
				//shrink _cend to be half
				//shrink _end by 1

				auto tmp_begin = alloc.allocate(size());
				auto tmp_end = std::uninitialized_copy(begin(), end() - 1, tmp_begin);
				auto tmp_cend = tmp_begin + size();

				_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
			}
			else {
				alloc.destroy(_end-- - 1);
			}
		}

		void pop_front() {
			if (empty()) return;
			if (size() > capacity() / 2) {
				auto it = begin();
				while (it + 1 != end()) {
					*it = *(it + 1);
					++it;
				}
				alloc.destroy(it);
				--_end;
			}
			else {
				//auto it = begin();
				//while (it + 1 != end()) {
				//	*it = *(it + 1);
				//	++it;
				//}
				//alloc.destroy(it);
				//_end = it;
				//alloc.deallocate(it, size_type(_M_getStorage() - end())); // <-- will not work
				//_cend = _end;

				auto tmp_begin = alloc.allocate(size() - 1);
				auto tmp_end = std::uninitialized_copy(begin() + 1, end(), tmp_begin);
				auto tmp_cend = tmp_begin + size() - 1;
				_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
			}
		}

		iterator begin() { return _begin; }
		const_iterator begin() const { return _begin; }
		iterator end() { return _end; }
		const_iterator end() const { return _end; }

		reference front() { return *begin(); }
		const_reference front() const { return *begin(); }
		reference back() { return *(end() - 1); }
		const_reference back() const { return *(end() - 1); }

		iterator erase(iterator pos) {
			auto it = pos;
			while (it + 1 != end()) {
				*it = *(it + 1);
				++it;
			}
			alloc.destroy(it);
			--_end;
			return pos;
		}
		iterator erase(iterator it_begin, iterator it_end) {
			iterator it = std::copy(it_end, _end, it_begin);
			while (it != _end) alloc.destroy(it++);
			_end = _end - (it_end - it_begin);
			return it_begin;
		}
		iterator insert(iterator pos, const_reference val) {
			if (pos > end() || pos < begin()) throw std::runtime_error("index out of bound");

			size_type offset = size_type(pos - begin()) - 1;
			if (pos == end()) {
				push_back(val);
			}
			else if (pos == begin()) {
				push_front(val);
			}
			else {
				value_type prev = *--pos;
				*pos = val;
				while (pos != _end) {
					std::swap(*pos++, prev);
				}
				push_back(prev);
			}
			return begin() + offset;
		}
		iterator insert(iterator pos, iterator it_begin, iterator it_end) {
			if (pos > end() || pos < begin()) throw std::runtime_error("index out of bound");
			if (it_begin == it_end) return pos;

			size_type offset = size_type(pos - begin()) - 1;

			/*
			+-------------------------------------+------------------------------------------------------+
			|senario 1:                           |                                                      |
			|                                     | senario 2:                                           |
			|   _end + (end - begin) < _cend      |                                                      |
			|                                     |    _end + (end - begin) >= _cend                     |
			|                                     |                                                      |
			|          |     |      |        |    |                                                      |
			|          +-----+------+--------+    |     |     |      |        |                          |
			|      _begin   pos  _end        _cend|     +-----+------+--------+                          |
			|                                     | _begin   pos  _end        _cend                      |
			|                                     |                                                      |
			|                |    |               |                                                      |
			|                +----+               |           |            |                             |
			|             begin   end             |           +------------+                             |
			|                                     |        begin           end                           |
			|          |     |    | |    |   |    |                                                      |
			|          +-----+----+-+----+---+    | new allocation: double storage                       |
			|      _begin   pos         _end _cend|                                                      |
			|                                     |     |                     |                     |    |
			|                                     |     +---------------------+---------------------+    |
			|                                     |                                                      |
			|                                     |     |     |            |                        |    |
			|                                     |     +-----+------------+------------------------+    |
			|                                     |  _begin  pos          _end                      _cend|
			|                                     |                                                      |
			|                                     |        |        |                  |                 |
			|                                     |        |        |                  |                 |
			|                                     |        |        |                  |                 |
			|                                     |   first       third              second              |
			|                                     |   copy        copy               copy                |
			|                                     |                                                      |
			|                                     |                                                      |
			+-------------------------------------+------------------------------------------------------+
			*/
			// senario 2
			if ((it_end - it_begin + end()) >= _M_getStorage()) {
				size_type new_size = 2 * (size_type(it_end - it_begin) + size());
				auto tmp_begin = alloc.allocate(new_size);

				if (pos == end()) {
					auto tmp_end = std::uninitialized_copy(begin(), end(), tmp_begin);
					tmp_end = std::uninitialized_copy(it_begin, it_end, tmp_end);
					auto tmp_cend = tmp_begin + new_size;
					_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
				}
				else {
					auto tmp_end = std::uninitialized_copy(begin(), pos, tmp_begin);
					tmp_end = std::uninitialized_copy(pos, end(), tmp_end + size_type(it_end - it_begin));
					std::uninitialized_copy(it_begin, it_end, tmp_begin + size_type(pos - begin())); //don't use pos as dst ite!
					auto tmp_cend = tmp_begin + new_size;
					_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
				}
			}
			else { // senario 1
				if (pos == end()) {
					_end = std::uninitialized_copy(it_begin, it_end, end());
				}
				else {
					vector tmp(size_type(end() - pos));
					std::copy(pos, end(), tmp.begin());

					_end = std::uninitialized_fill_n(end(), size_type(it_end - it_begin), value_type{});
					std::copy(it_begin, it_end, pos);
					std::copy(tmp.begin(), tmp.end(), pos + size_type(it_end - it_begin));
				}
			}
			return begin() + offset;
		}

		size_type capacity() const { return size_type(_cend - _begin); }
		size_type size() const { return size_type(_end - _begin); }

		void print() const {
			for (auto it = _begin; it != _end; ++it) {
				std::cout << *it << " ";
			}
			std::cout << std::endl;;
		}

		bool empty() const { return _begin == _end; }

		~vector() {
			_M_dealloc();
		}

	private:
		iterator _begin;
		iterator _end;
		iterator _cend;

		Alloc alloc;

		iterator _M_getStorage() { return _cend; }

		void _M_alloc_refresh(iterator it_begin, iterator it_end, iterator cend) {
			_M_dealloc();
			_begin = it_begin;
			_end = it_end;
			_cend = cend;
		}

		void _M_realloc_copy(const_iterator it_begin, const_iterator it_end) {
			auto tmp_begin = alloc.allocate(size_type(it_end - it_begin));
			auto tmp_end = std::uninitialized_copy(it_begin, it_end, tmp_begin);
			auto tmp_cend = tmp_begin + size_type(it_end - it_begin);
			_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
		}

		void _M_realloc_fill(size_type size, const_reference val = value_type{}) {
			auto tmp_begin = alloc.allocate(size);
			auto tmp_end = std::uninitialized_fill_n(tmp_begin, size, val);
			auto tmp_cend = tmp_begin + size;
			_M_alloc_refresh(tmp_begin, tmp_end, tmp_cend);
		}

		void _M_dealloc() {
			auto p = _begin;
			while (p != _end) {
				alloc.destroy(p++);
			}
			alloc.deallocate(_begin, capacity());
			_begin = nullptr;
			_end = _begin;
			_cend = _end;
		}
	};

	template <typename T>
	inline const vector<T> operator+(const vector<T> & v1, const vector<T> & v2)
	{
		vector<T> res(v1);
		res += v2;
		return res;
	}

	template <typename T>
	inline bool operator==(const vector<T> & v1, const vector<T> & v2)
	{
		if (v1.size() != v2.size()) return false;
		for (auto it1 = v1.begin(), it2 = v2.begin(); it1 != v1.end() && it2 != v2.end(); ++it1, ++it2) {
			if (*it1 != *it2) return false;
		}
		return true;
	}

	/*template <typename T>
	std::ostream& operator<<(std::ostream& os, const vector<T>& v) {
		for (std::size_t i = 0; i < v.size(); ++i) {
			os << v.at(i) << " ";
		}
		return os;
	}*/
}