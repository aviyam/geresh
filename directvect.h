// Copyright (C) 2003 Mooffie <mooffie@typo.co.il>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111, USA.

#ifndef BDE_SIMPLEVEC_H
#define BDE_SIMPLEVEC_H

#include <vector>

// For some vectors we use our DirectVector instead of STL's vector.
//
// Why's that?
//
// In this application I use vector<T> mostly as an "enhanced" array.
// Unfortunately, STL's vector<T>::iterator is not necessarily a pointer to
// T. That's the case in newer STL libraries provided with GNU C++.
//
// DirectVector accepts T* arguments.

template <class T>
class DirectVector {
    std::vector<T> vec;
public:
    typedef size_t size_type;

    DirectVector(): vec() {}
    DirectVector(size_type n): vec(n) {}
    DirectVector(size_type n, const T& t): vec(n, t) {}
    DirectVector(const DirectVector& other_vec): vec(other_vec.vec) {}
    DirectVector(const T *first, const T *last): vec(first, last) {}
    DirectVector& operator=(const DirectVector& other_vec) {
	vec.operator=(other_vec.vec);
	return *this;
    }

    size_type size() const { return vec.size(); }
    size_type capacity() const { return vec.capacity(); }
    bool empty() const { return vec.empty(); }
    T* begin() { return &vec[0]; }
    T* end() { return (begin() + size()); }
    const T* begin() const { return &vec[0]; }
    const T* end() const { return (begin() + size()); }
    T& operator[] (size_type n) { return vec[n]; }
    const T& operator[] (size_type n) const { return vec[n]; }

    void reserve(size_type n) { vec.reserve(n); }
    void resize(size_type n) { vec.resize(n); }
    T& back() { return vec.back(); }
    T& front() { return vec.front(); }
    const T& front() const { return vec.front(); }
    const T& back() const { return vec.back(); }

    void push_back(const T& x) { vec.push_back(x); }
    void pop_back() { vec.pop_back(); }

    void swap(DirectVector& other_vec) { vec.swap(other_vec); }
    void clear() { vec.clear(); }
   
    // Note the "&*expression" syntax. "*" dereferences the iterator
    // and "&" gives us the pointer we need.
    T* insert(T* pos, const T& x) {
	return &*vec.insert(vec.begin() + (pos - begin()), x);
    }
    void insert(T* pos, const T* first, const T* last) {
	vec.insert(vec.begin() + (pos - begin()), first, last);
    }
    void insert(T* pos, size_type n, const T& x) {
	vec.insert(vec.begin() + (pos - begin()), n, x);
    }
    T* erase(T* pos) {
	return &*vec.erase(vec.begin() + (pos - begin()));
    }
    T* erase(T* first, T* last) {
	return &*vec.erase(vec.begin() + (first - begin()),
			   vec.begin() + (last  - begin()));
    }

    bool operator==(const DirectVector &other) const { return vec == other.vec; }
    bool operator!=(const DirectVector &other) const { return vec != other.vec; }
    bool operator<(const DirectVector &other) const { return vec < other.vec; }
};

#endif

