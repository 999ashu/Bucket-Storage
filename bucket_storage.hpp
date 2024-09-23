#pragma once

#include <iterator>
#include <stdexcept>

// INTERFACE

namespace detail
{
	using size_type = std::size_t;

	template< typename U >
	struct Node
	{
		U m_value;
		Node* m_next{};
		Node* m_prev{};

		Node() = default;
		explicit Node(const U& value, Node* prev = nullptr) : m_value(value), m_prev(prev) {}
	};

	template< typename U >
	struct List
	{
		using NodeType = Node< U >;

		List() = default;
		List(const List& other);
		List(List&& other) noexcept;
		~List();
		List& operator=(const List& other) noexcept;
		List& operator=(List&& other) noexcept;
		void push_back(const U& value);
		void erase(NodeType* target);
		void pop_back();
		[[nodiscard]] NodeType* front() const noexcept;
		[[nodiscard]] NodeType* back() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] size_type size() const noexcept;
		void clear();
		void swap(List& other) noexcept;

	  private:
		size_type m_size{};
		NodeType* m_head{};
		NodeType* m_tail{};
	};
}	 // namespace detail

template< typename T >
class BucketStorage
{
	template< typename U >
	struct BSIterator;

	struct Block;

  public:
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;
	using iterator = BSIterator< T >;
	using const_iterator = BSIterator< const T >;

  private:
	using Node = detail::Node< value_type* >;

	template< typename U >
	struct BSIterator
	{
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = Node*;
		using difference_type = std::ptrdiff_t;
		using pointer = U*;
		using reference = U&;

	  private:
		value_type m_node{};
		Block* m_block{};

	  public:
		BSIterator(const iterator& other);
		BSIterator(value_type node, Block* block);
		BSIterator& operator=(const iterator& other);
		BSIterator operator++(int);
		BSIterator& operator++();
		BSIterator operator--(int);
		BSIterator& operator--();
		bool operator<(const BSIterator& other) const;
		bool operator>(const BSIterator& other) const;
		bool operator<=(const BSIterator& other) const;
		bool operator>=(const BSIterator& other) const;
		bool operator==(const BSIterator& other) const noexcept;
		pointer operator->();
		reference operator*();
		value_type node() const noexcept;
		Block* block() const noexcept;
	};

	struct Block
	{
		Node* m_values;
		size_type m_index;
		size_type m_size{};
		detail::List< Node* > m_stack;
		typename detail::List< Block* >::NodeType* m_node{};
		value_type* m_data;
		size_type m_block_capacity;

		explicit Block(const size_type block_capacity, const size_type index) :
			m_values(new Node[block_capacity]), m_index(index),
			m_data(static_cast< value_type* >(operator new(block_capacity * sizeof(value_type)))), m_block_capacity(block_capacity)
		{
		}

		~Block()
		{
			delete[] m_values;
			operator delete(m_data);
		}
	};

	size_type m_block_capacity;
	size_type m_size{};
	size_type m_blocks_count{};
	Node* m_end;
	Node* m_front;
	detail::List< Block* > m_list{};
	detail::List< Block* > m_stack{};

	template< typename U >
	iterator insert_impl(U&& value);

  public:
	BucketStorage();
	BucketStorage(const BucketStorage& other);
	BucketStorage(BucketStorage&& other) noexcept;
	explicit BucketStorage(size_type block_capacity);
	~BucketStorage();
	BucketStorage& operator=(const BucketStorage& other) noexcept;
	BucketStorage& operator=(BucketStorage&& other) noexcept;
	iterator insert(const value_type& value);
	iterator insert(value_type&& value);
	iterator erase(const_iterator it);
	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] size_type size() const noexcept;
	[[nodiscard]] size_type capacity() const noexcept;
	void shrink_to_fit();
	void clear() noexcept;
	void swap(BucketStorage& other) noexcept;
	[[nodiscard]] iterator begin() noexcept;
	[[nodiscard]] const_iterator begin() const noexcept;
	[[nodiscard]] const_iterator cbegin() noexcept;
	[[nodiscard]] iterator end() noexcept;
	[[nodiscard]] const_iterator end() const noexcept;
	[[nodiscard]] const_iterator cend() noexcept;
	[[nodiscard]] iterator get_to_distance(iterator it, difference_type distance);
};

// BUCKETSTORAGE IMPLEMENTATION

template< typename T >
BucketStorage< T >::BucketStorage() : m_block_capacity(64), m_end(new Node()), m_front(m_end)
{
}

template< typename T >
BucketStorage< T >::BucketStorage(const BucketStorage& other) :
	m_block_capacity(other.m_block_capacity), m_end(new Node())
{
	for (auto it = other.begin(); it != other.end(); ++it)
	{
		insert(*it);
	}
}

template< typename T >
BucketStorage< T >::BucketStorage(BucketStorage&& other) noexcept : m_block_capacity(64), m_end(new Node())
{
	swap(other);
}

template< typename T >
BucketStorage< T >::BucketStorage(const size_type block_capacity) :
	m_block_capacity(block_capacity), m_end(new Node()), m_front(m_end)
{
}

template< typename T >
BucketStorage< T >::~BucketStorage()
{
	clear();
	delete m_end;
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(const BucketStorage& other) noexcept
{
	if (this != &other)
	{
		clear();
		BucketStorage temp(other);
		swap(temp);
	}
	return *this;
}

template< typename T >
BucketStorage< T >& BucketStorage< T >::operator=(BucketStorage&& other) noexcept
{
	if (this != &other)
	{
		clear();
		swap(other);
	}
	return *this;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(const value_type& value)
{
	return insert_impl(value);
}
template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(value_type&& value)
{
	return insert_impl(std::move(value));
}

template< typename T >
template< typename U >
typename BucketStorage< T >::iterator BucketStorage< T >::insert_impl(U&& value)
{
	if (m_size == capacity())
	{
		m_list.push_back(new Block(m_block_capacity, m_blocks_count));
		m_list.back()->m_value->m_node = m_list.back();
		++m_blocks_count;
	}

	Node* curr;
	Block* block = m_list.back()->m_value;
	if (!m_stack.empty())
	{
		curr = m_stack.back()->m_value->m_stack.back()->m_value;
		size_type index = curr - block->m_values;

		curr = block->m_values + index;
		new (block->m_data + index) value_type(std::forward< U >(value));
		curr->m_value = block->m_data + index;
		m_stack.back()->m_value->m_stack.pop_back();

		if (m_stack.back()->m_value->m_stack.size() == 0)
		{
			m_stack.pop_back();
		}
	}
	else
	{
		size_type index = m_size % m_block_capacity;

		curr = block->m_values + index;
		new (block->m_data + index) value_type(std::forward< U >(value));
		curr->m_value = block->m_data + index;

		if (m_size != 0)
		{
			m_end->m_prev->m_next = curr;
			curr->m_prev = m_end->m_prev;
		}
		else
		{
			m_front = curr;
		}

		curr->m_next = m_end;
		m_end->m_prev = curr;
		++m_list.back()->m_value->m_size;
	}

	++m_size;
	return iterator(curr, m_list.back()->m_value);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::erase(const_iterator it)
{
	if (!(it.node() && it.block()))
	{
		throw std::runtime_error("Attempt to erase by uninitialized iterator.");
	}

	Node* curr_node = it.node();
	Block* curr_block = it.block();

	curr_block->m_stack.push_back(curr_node);
	if (curr_block->m_stack.size() == 1)
	{
		m_stack.push_back(curr_block);
	}

	curr_node->m_value->~value_type();
	curr_node->m_value = nullptr;

	if (curr_node->m_prev && curr_node->m_next)
	{
		curr_node->m_next->m_prev = curr_node->m_prev;
		curr_node->m_prev->m_next = curr_node->m_next;
	}
	else if (!curr_node->m_prev && curr_node->m_next)
	{
		curr_node->m_next->m_prev = nullptr;
		m_front = curr_node->m_next;
	}
	else
	{
		curr_node->m_prev->m_next = nullptr;
	}

	--curr_block->m_size;
	--m_size;

	if (curr_node->m_next && !(curr_block->m_values <= curr_node && curr_node <= curr_block->m_values + curr_block->m_block_capacity))
	{
		curr_block = curr_block->m_node->m_next->m_value;
	}

	if (curr_block->m_size == 0)
	{
		auto tmp = it.block()->m_node;
		delete it.block();
		m_list.erase(tmp);
	}

	return iterator(curr_node, curr_block);
}

template< typename T >
bool BucketStorage< T >::empty() const noexcept
{
	return m_size == 0;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::size() const noexcept
{
	return m_size;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::capacity() const noexcept
{
	return m_list.size() * m_block_capacity;
}

template< typename T >
void BucketStorage< T >::shrink_to_fit()
{
	BucketStorage temp = BucketStorage(m_block_capacity);
	for (iterator it = begin(); it != end(); ++it)
	{
		temp.insert(std::move(*it));
	}
	swap(temp);
}

template< typename T >
void BucketStorage< T >::clear() noexcept
{
	while (!empty())
	{
		erase(begin());
	}

	if (!m_list.empty())
	{
		delete m_list.back()->m_value;
		m_list.pop_back();
	}

	m_block_capacity = 64;
	m_front = m_end;
}

template< typename T >
void BucketStorage< T >::swap(BucketStorage& other) noexcept
{
	using std::swap;
	swap(m_block_capacity, other.m_block_capacity);
	swap(m_size, other.m_size);
	swap(m_front, other.m_front);
	swap(m_end, other.m_end);
	m_list.swap(other.m_list);
	m_stack.swap(other.m_stack);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::begin() noexcept
{
	return iterator(m_front, empty() ? nullptr : m_list.front()->m_value);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::begin() const noexcept
{
	return const_iterator(m_front, empty() ? nullptr : m_list.front()->m_value);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cbegin() noexcept
{
	return const_iterator(m_front, empty() ? nullptr : m_list.front()->m_value);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::end() noexcept
{
	return iterator(m_end, empty() ? nullptr : m_list.back()->m_value);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::end() const noexcept
{
	return const_iterator(m_end, empty() ? nullptr : m_list.back()->m_value);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cend() noexcept
{
	return const_iterator(m_end, empty() ? nullptr : m_list.back()->m_value);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::get_to_distance(iterator it, difference_type distance)
{
	if (distance >= 0)
	{
		for (difference_type i = 0; i < distance; ++i)
		{
			++it;
		}
	}
	else
	{
		for (difference_type i = -distance; i > 0; --i)
		{
			--it;
		}
	}
	return it;
}

// BSITERATOR IMPLEMENTATION

template< typename T >
template< typename U >
BucketStorage< T >::BSIterator< U >::BSIterator(const iterator& other) : m_node(other.node()), m_block(other.block())
{
}

template< typename T >
template< typename U >
BucketStorage< T >::BSIterator< U >::BSIterator(value_type node, Block* block) : m_node(node), m_block(block)
{
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U >& BucketStorage< T >::BSIterator< U >::operator=(const iterator& other)
{
	if (*this != other)
	{
		m_node = other.m_node;
		m_block = other.m_block;
	}
	return *this;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U > BucketStorage< T >::BSIterator< U >::operator++(int)
{
	BSIterator temp = *this;
	++*this;
	return temp;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U >& BucketStorage< T >::BSIterator< U >::operator++()
{
	if (!(m_node && m_block))
	{
		throw std::runtime_error("Attempt to increment uninitialized iterator.");
	}

	if (m_node->m_next && !m_node->m_next->m_next)
	{
		m_node = m_node->m_next;
		return *this;
	}

	if (m_node->m_next)
	{
		m_node = m_node->m_next;
		Node* curr_block = m_block->m_values;
		if (m_node->m_next && !(curr_block <= m_node && m_node <= curr_block + m_block->m_block_capacity))
		{
			m_block = m_block->m_node->m_next->m_value;
		}
	}
	else
	{
		throw std::runtime_error("Attempt to increment past the end of the container.");
	}

	return *this;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U > BucketStorage< T >::BSIterator< U >::operator--(int)
{
	BSIterator temp = *this;
	--*this;
	return temp;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U >& BucketStorage< T >::BSIterator< U >::operator--()
{
	if (!(m_node && m_block))
	{
		throw std::runtime_error("Attempt to decrement uninitialized iterator.");
	}

	if (!m_node->m_next)
	{
		m_node = m_node->m_prev;
		return *this;
	}

	if (m_node->m_prev)
	{
		m_node = m_node->m_prev;
		Node* curr_block = m_block->m_values;
		if (!(curr_block <= m_node && m_node <= curr_block + m_block->m_block_capacity))
		{
			m_block = m_block->m_node->m_prev->m_value;
		}
	}
	else
	{
		throw std::runtime_error("Attempt to decrement before the beginning of the container.");
	}

	return *this;
}

template< typename T >
template< typename U >
bool BucketStorage< T >::BSIterator< U >::operator<(const BSIterator& other) const
{
	if (!(m_node && m_block && other.m_node && other.m_block))
	{
		throw std::runtime_error("Attempt to compare uninitialized iterator.");
	}

	if (m_block->m_index != other.m_block->m_index)
	{
		return m_block->m_index < other.m_block->m_index;
	}

	return m_node < other.m_node;
}

template< typename T >
template< typename U >
bool BucketStorage< T >::BSIterator< U >::operator>(const BSIterator& other) const
{
	return other < *this;
}

template< typename T >
template< typename U >
bool BucketStorage< T >::BSIterator< U >::operator<=(const BSIterator& other) const
{
	return !(*this > other);
}

template< typename T >
template< typename U >
bool BucketStorage< T >::BSIterator< U >::operator>=(const BSIterator& other) const
{
	return !(*this < other);
}

template< typename T >
template< typename U >
bool BucketStorage< T >::BSIterator< U >::operator==(const BSIterator& other) const noexcept
{
	return m_node == other.m_node;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U >::pointer BucketStorage< T >::BSIterator< U >::operator->()
{
	if (m_node == nullptr)
	{
		throw std::runtime_error("Attempt to access uninitialized iterator.");
	}
	return m_node->m_value;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U >::reference BucketStorage< T >::BSIterator< U >::operator*()
{
	if (m_node == nullptr)
	{
		throw std::runtime_error("Attempt to dereference uninitialized iterator.");
	}
	return *m_node->m_value;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::template BSIterator< U >::value_type BucketStorage< T >::BSIterator< U >::node() const noexcept
{
	return m_node;
}

template< typename T >
template< typename U >
typename BucketStorage< T >::Block* BucketStorage< T >::BSIterator< U >::block() const noexcept
{
	return m_block;
}

// LIST IMPLEMENTATION

template< typename T >
detail::List< T >::List(const List& other)
{
	m_size = other.m_size;
	if (other.m_head)
	{
		for (NodeType* curr = other.m_head; curr != other.m_tail; curr = curr->m_next)
		{
			push_back(curr->m_value);
		}
	}
}

template< typename T >
detail::List< T >::List(List&& other) noexcept
{
	swap(other);
}

template< typename T >
detail::List< T >::~List()
{
	clear();
}

template< typename T >
detail::List< T >& detail::List< T >::operator=(const List& other) noexcept
{
	if (this != &other)
	{
		clear();
		List temp(other);
		swap(temp);
	}
	return *this;
}

template< typename T >
detail::List< T >& detail::List< T >::operator=(List&& other) noexcept
{
	if (this != &other)
	{
		clear();
		swap(other);
	}
	return *this;
}

template< typename T >
void detail::List< T >::push_back(const T& value)
{
	auto node = new NodeType(value, m_tail);
	if (!m_head)
	{
		m_head = node;
		m_tail = m_head;
	}
	else
	{
		m_tail->m_next = node;
		m_tail = node;
	}
	++m_size;
}

template< typename T >
void detail::List< T >::erase(NodeType* target)
{
	if (!m_size)
	{
		throw std::out_of_range("The list is empty.");
	}

	--m_size;

	if (target == m_head)
	{
		if (m_head->m_next)
		{
			m_head = m_head->m_next;
			m_head->m_prev = nullptr;
		}
		else
		{
			m_head = nullptr;
			m_tail = nullptr;
		}
	}
	else if (target == m_tail)
	{
		m_tail = m_tail->m_prev;
		m_tail->m_next = nullptr;
	}
	else
	{
		target->m_prev->m_next = target->m_next;
		target->m_next->m_prev = target->m_prev;
	}

	delete target;
}

template< typename T >
void detail::List< T >::pop_back()
{
	erase(m_tail);
}

template< typename T >
typename detail::List< T >::NodeType* detail::List< T >::front() const noexcept
{
	return m_head;
}

template< typename T >
typename detail::List< T >::NodeType* detail::List< T >::back() const noexcept
{
	return m_tail;
}

template< typename T >
bool detail::List< T >::empty() const noexcept
{
	return m_size == 0;
}

template< typename T >
detail::size_type detail::List< T >::size() const noexcept
{
	return m_size;
}

template< typename T >
void detail::List< T >::clear()
{
	while (m_head != nullptr)
	{
		pop_back();
	}
}

template< typename T >
void detail::List< T >::swap(List& other) noexcept
{
	using std::swap;
	swap(m_size, other.m_size);
	swap(m_head, other.m_head);
	swap(m_tail, other.m_tail);
}
