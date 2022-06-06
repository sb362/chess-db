#pragma once

#include <deque>
#include <memory>

#include "chess/util.h"

namespace pgn
{

template <class Impl>
class Node
{
private:
	Impl *_parent;
	std::deque<std::unique_ptr<Impl>> _children;

	Impl *self() { return static_cast<Impl *>(this); }
	const Impl *self() const { return static_cast<const Impl *>(this); }

public:
	Node(Impl *parent = nullptr)
		: _parent(parent), _children()
	{
	}

	std::size_t size() const
	{
		return _children.size();
	}

	bool empty() const
	{
		return _children.empty();
	}

	void clear()
	{
		_children.clear();
	}

	Impl *parent()
	{
		return _parent;
	}

	const Impl *parent() const
	{
		return _parent;
	}

	bool is_root() const
	{
		return parent() == nullptr;
	}

	bool is_dangling() const
	{
		return parent() == nullptr;
	}

	bool is_leaf() const
	{
		return empty();
	}

	Impl *root()
	{
		Impl *node = self();

		while (!node->is_root())
			node = node->parent();

		return node;
	}

	const Impl *root() const
	{
		const Impl *node = self();

		while (!node->is_root())
			node = node->parent();

		return node;
	}

	Impl *front()
	{
		return _children.front().get();
	}

	const Impl *front() const
	{
		return _children.front().get();
	}

	Impl *back()
	{
		return _children.back().get();
	}

	const Impl *back() const
	{
		return _children.back().get();
	}

	void pop_front()
	{
		ASSERT(!empty());

		_children.pop_front();
	}

	void pop_back()
	{
		ASSERT(!empty());

		_children.pop_back();
	}

	Impl *next(std::size_t i = 0)
	{
		ASSERT(i < size());

		return _children[i].get();
	}

	const Impl *next(std::size_t i = 0) const
	{
		ASSERT(i < size());

		return _children[i].get();
	}

	template <typename ...Args>
	void emplace(std::size_t i, Args &&...args)
	{
		insert(i, std::make_unique<Impl>(nullptr, std::forward<Args>(args)...));
	}

	template <typename ...Args>
	void emplace_back(Args &&...args)
	{
		push_back(std::make_unique<Impl>(nullptr, std::forward<Args>(args)...));
	}

	template <typename ...Args>
	void emplace_front(Args &&...args)
	{
		push_front(std::make_unique<Impl>(nullptr, std::forward<Args>(args)...));
	}

	void insert(std::size_t i, std::unique_ptr<Impl> node)
	{
		ASSERT(node->is_dangling());

		node->_parent = self();
		_children.insert(_children.begin() + i, std::move(node));
	}

	void push_back(std::unique_ptr<Impl> node)
	{
		ASSERT(node->is_dangling());

		node->_parent = self();
		_children.push_back(std::move(node));
	}

	void push_front(std::unique_ptr<Impl> node)
	{
		ASSERT(node->is_dangling());

		node->_parent = self();
		_children.push_front(std::move(node));
	}

	void remove(std::size_t i)
	{
		ASSERT(i < size());

		_children.erase(_children.begin() + i);
	}

	void swap(std::size_t i, std::size_t j)
	{
		ASSERT(i < size());
		ASSERT(j < size());

		_children[i].swap(_children[j]);
	}
};

} // pgn
