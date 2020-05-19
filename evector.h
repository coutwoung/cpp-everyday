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

#include <iostream>
#include <initializer_list>
#include <algorithm>
#include <type_traits>
#include <memory>

namespace wsy {

	template <typename T, class Alloc = std::allocator<T>>
	class vector {
	public:
		vector()
			: _begin{ nullptr }, _end{ nullptr }, _cend{ nullptr } {}

		vector(std::size_t size)
			: _begin{ alloc.allocate(size) },
			_end{ std::uninitialized_fill_n(_begin, size, T{}) },
			_cend{ _begin + size }
		{

		}

		vector(const int& val, std::size_t size)
			: _begin{ alloc.allocate(size) },
			_end{ std::uninitialized_fill_n(_begin, size, val) },
			_cend{ _begin + size }
		{

		}

		vector(std::initializer_list<T> lst)
			: _begin{ alloc.allocate(lst.size()) },
			_end{ std::uninitialized_copy(lst.begin(), lst.end(), _begin) },
			_cend{ _begin + lst.size() }
		{

		}

		vector(const vector& v) // remains _cend and _csize
			: _begin{ alloc.allocate(v.Size()) },
			_end{ std::uninitialized_copy(v.Begin(), v.End(), _begin) },
			_cend{ _begin + v.Size() }
		{

		}

		vector(vector&& v) //move constructor remains _cend and _csize
			: _begin{ v._begin }, _end{ v._end }, _cend{ v._cend } {
			v._begin = nullptr;
			v._end = nullptr; // means empty
			v._cend = nullptr;
		}

		void Dealloc() {
			auto p = _begin;
			while (p != _end) {
				alloc.destroy(p++);
			}
			alloc.deallocate(_begin, Capacity());
			_begin = nullptr;
			_end = _begin;
			_cend = _end;
		}

		vector& operator=(const vector& v) {
			auto tmp_begin = alloc.allocate(v.Size());
			auto tmp_end = std::uninitialized_copy(v.Begin(), v.End(), tmp_begin);
			auto tmp_cend = tmp_begin + v.Size();
			Dealloc();
			_begin = tmp_begin;
			_end = tmp_end;
			_cend = tmp_cend;

			return *this;
		}

		vector& operator=(vector&& v) { //remain _cend and _csize
			Dealloc();
			_begin = v._begin;
			_end = v._end;
			_cend = v._cend;

			v.Dealloc();
			return *this;
		}

		vector& operator+=(const vector& v) { // do not remain _cend and _csize
			auto tmp_begin = alloc.allocate(Size() + v.Size());
			auto tmp_end = std::uninitialized_copy(Begin(), End(), tmp_begin);
			tmp_end = std::uninitialized_copy(v.Begin(), v.End(), tmp_end);
			auto tmp_cend = tmp_begin + Size() + v.Size();
			Dealloc();
			_begin = tmp_begin;
			_end = tmp_end;
			_cend = tmp_cend;

			return *this;
		}

		const T& operator[](std::size_t idx) const {
			return *(_begin + idx);
		}

		T& operator[](std::size_t idx) {
			return const_cast<T&>(static_cast<const vector&>(*this)[idx]);
		}

		const T& At(std::size_t idx) const {
			if (idx < 0 || idx >= Size()) throw std::runtime_error("index out of bound");
			return *(_begin + idx);
		}

		T& At(std::size_t idx) {
			return const_cast<T&>(static_cast<const vector&>(*this).At(idx));
		}

		void Push_back(const T& val) {
			if (Empty()) {
				_begin = alloc.allocate(1);
				alloc.construct(_begin, val);
				_end = _begin + 1;
				_cend = _end;
			}
			else if (_end == _cend) {
				//expand _cend to be double
				//expand _end by 1

				auto tmp_begin = alloc.allocate(2 * Size());
				auto tmp_end = std::uninitialized_copy(Begin(), End(), tmp_begin);
				alloc.construct(tmp_end++, val);
				auto tmp_cend = tmp_begin + 2 * Size();

				Dealloc();
				_begin = tmp_begin;
				_end = tmp_end;
				_cend = tmp_cend;
			}
			else {
				alloc.construct(_end++, val);
			}
		}

		void Pop() {
			if (Empty()) return;
			else if (2 * Size() <= Capacity()) {
				//shrink _cend to be half
				//shrink _end by 1

				auto tmp_begin = alloc.allocate(Size());
				auto tmp_end = std::uninitialized_copy(Begin(), End() - 1, tmp_begin);
				auto tmp_cend = tmp_begin + Size();

				Dealloc();
				_begin = tmp_begin;
				_end = tmp_end;
				_cend = tmp_cend;
			}
			else {
				alloc.destroy(_end-- - 1);
			}
		}

		T* Begin() { return _begin; }
		const T* Begin() const { return _begin; }
		T* End() { return _end; }
		const T* End() const { return _end; }

		std::size_t Capacity() const { return size_t(_cend - _begin); }
		std::size_t Size() const { return size_t(_end - _begin); }

		void Print() const {
			for (auto it = _begin; it != _end; ++it) {
				std::cout << *it << " ";
			}
			std::cout << std::endl;;
		}

		bool Empty() const { return _begin == _end; }

		~vector() {
			Dealloc();
		}

	private:
		T* _begin;
		T* _end;
		T* _cend;

		Alloc alloc;
	};

	template <typename T>
	const vector<T> operator+(const vector<T> & v1, const vector<T> & v2)
	{
		vector<T> res(v1);
		res += v2;
		return res;
	}

	template <typename T>
	std::ostream& operator<<(std::ostream& os, const vector<T>& v) {
		for (std::size_t i = 0; i < v.Size(); ++i) {
			os << v.At(i) << " ";
		}
		return os;
	}

	template <typename T>
	class rvector {
	public:
		rvector()
			///: _arr{ new T[1]{} }, _size{ 1 }, _csize{ 1 }, _begin{ _arr }, _end{ _arr + 1 }, _cend{ _arr + 1 } {}
			: _arr{ nullptr }, _size{ 0 }, _csize{ 0 }, _begin{ nullptr }, _end{ nullptr }, _cend{ nullptr } {}

		rvector(std::size_t size)
			: _arr{ new T[size]{} }, _size{ size }, _csize{ size }, _begin{ _arr }, _end{ _arr + size }, _cend{ _arr } {}

		rvector(const int& val, std::size_t size)
			: _arr{ new T[size]{} }, _size{ size }, _csize{ size }, _begin{ _arr }, _end{ _arr + size }, _cend{ _arr + _size } {
			for (std::size_t i = 0; i < _size; ++i) {
				_arr[i] = val;
			}
		}

		rvector(std::initializer_list<T> lst)
			: _arr{ new T[lst.size()]{} }, _size{ lst.size() }, _csize{ lst.size() }, _begin{ _arr }, _end{ _arr + _size }, _cend{ _arr + _size } {
			std::copy(lst.begin(), lst.end(), _begin);
		}

		rvector(const rvector& v) // remains _cend and _csize
			: _arr{ new T[v._csize]{} }, _size{ v._size }, _csize{ v._csize }, _begin{ _arr }, _end{ _arr + _size }, _cend{ _arr + _csize } {
			std::copy(v._begin, v._cend, _begin);

		}

		rvector(rvector&& v) //move constructor remains _cend and _csize
			: _arr{ v._arr }, _size{ v._size }, _csize{ v._csize }, _begin{ v._begin }, _end{ v._end }, _cend{ v._cend } {
			v._arr = nullptr;
			v._size = 0;
			v._csize = 0;
			v._begin = v._arr;
			v._end = v._begin; // means empty
			v._cend = v._end;
		}

		rvector& operator=(const rvector& v) {
			rvector _v(v);
			std::swap(*this, _v);
			return *this;
		}

		rvector& operator=(rvector&& v) { //remain _cend and _csize
			T* tmp = _arr;
			_arr = v._arr;
			_size = v._size;
			_csize = v._csize;
			_begin = v._begin;
			_end = v._end;
			_cend = v._cend;

			v._arr = nullptr;
			v._size = 0;
			v._csize = 0;
			v._begin = v._arr;
			v._end = v._begin; // means empty
			v._cend = v._end;

			delete tmp;
			return *this;
		}

		rvector& operator+=(const rvector& v) { // do not remain _cend and _csize
			T* new_arr = new T[_size + v._size]{};
			std::copy(_begin, _end, new_arr);
			std::copy(v._begin, v._end, new_arr + _size);
			Reallocate(new_arr, _size + v._size);
			return *this;
		}

		const T& operator[](std::size_t idx) const {
			return _arr[idx];
		}

		T& operator[](std::size_t idx) {
			return const_cast<T&>(static_cast<const rvector&>(*this)[idx]);
		}

		const T& At(std::size_t idx) const {
			if (idx < 0 || idx >= _size) throw std::runtime_error("index out of bound");
			return _arr[idx];
		}

		T& At(std::size_t idx) {
			return const_cast<T&>(static_cast<const rvector&>(*this).At(idx));
		}

		//void Push_back(const T& val) {
		template <typename U>
		void Push_back(U&& val) {
			if (Empty()) {
				_arr = Allocate(1);
				_arr[0] = val;
				_size++;
				_csize++;
				_begin = _arr;
				_end = _arr + 1;
				_cend = _arr + 1;
			}
			else if (_cend == _end) { // expand
				T* new_arr = new T[2 * _size]{};
				std::copy(_begin, _end, new_arr); // same as _cend

				_begin = new_arr;
				_csize = 2 * _size;
				_cend = new_arr + 2 * _size;
				_end = new_arr + _size + 1;
				if (std::is_rvalue_reference<decltype(val)>::value) {
					new_arr[_size++] = std::move(val); // std::forward<U>(val);
					std::cout << "move push back" << std::endl;
				}
				else {
					new_arr[_size++] = val;
				}
				delete[] _arr;
				_arr = new_arr;
				std::cout << "real push back" << std::endl;
			}
			else {
				_arr[_size++] = val;
				_end++;
			}
		}

		void Pop() {
			if (_size <= _csize / 2) {
				T* new_arr = Allocate(_size - 1);
				for (std::size_t i = 0; i < _size - 1; ++i) {
					new_arr[i] = _arr[i];
				}
				Reallocate(new_arr, _size - 1);
				std::cout << "real pop" << std::endl;
			}
			else {
				_arr[_size - 1] = T{};
				_size--;
				_end--;
			}
		}

		T* Allocate(std::size_t size) {
			T* new_arr = new T[size]{};
			return new_arr;
		}

		void Reallocate(std::size_t size) { //reallocate will reset the _cend and _csize
			if (_arr == nullptr) {
				_arr = Allocate(size);
				_size = size;
				_csize = size;
				_begin = _arr;
				_end = _arr + _size;
				_cend = _end;
			}
			else {
				T* new_arr = Allocate(size);
				Reallocate(new_arr, size);
			}
		}

		void Reallocate(T* arr, std::size_t size) {
			Deallocate();
			_arr = arr;
			_csize = size;
			_size = size;
			_begin = arr;
			_end = arr + size;
			_cend = _end;
		}

		void Deallocate() {
			delete[] _arr;
			_size = 0;
			_csize = 0;
			_arr = nullptr;
			_begin = nullptr;
			_end = nullptr;
			_cend = nullptr;
		}


		T* Begin() { return _begin; }
		const T* Begin() const { return _begin; }
		T* End() { return _end; }
		const T* End() const { return _end; }

		std::size_t Capacity() const { return _cend - _begin; }
		std::size_t Size() const { return _size; }

		void Print() const {
			for (std::size_t i = 0; i < _size; ++i) {
				std::cout << _arr[i] << " ";
			}
			std::cout << std::endl;;
		}

		bool Empty() const { return _begin == _end; }

		~rvector() {
			Deallocate();
		}

	private:
		T* _arr;
		std::size_t _size;
		std::size_t _csize;
		T* _begin;
		T* _end;
		T* _cend;
	};
}