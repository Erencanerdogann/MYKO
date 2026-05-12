#pragma once

#include <vector>
#include <mutex>
#include <cstdint>

template <typename T, size_t BlockSize = 256>
class MemoryPool {
public:
	MemoryPool() : m_freeList(nullptr), m_totalAllocated(0) {
		AllocateBlock();
	}

	~MemoryPool() {
		for (auto* block : m_blocks)
			::operator delete(block);
	}

	T* Acquire() {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_freeList == nullptr)
			AllocateBlock();

		Node* node = m_freeList;
		m_freeList = node->next;
		m_totalAllocated++;
		return reinterpret_cast<T*>(node);
	}

	void Release(T* ptr) {
		if (ptr == nullptr) return;
		std::lock_guard<std::mutex> lock(m_mutex);
		Node* node = reinterpret_cast<Node*>(ptr);
		node->next = m_freeList;
		m_freeList = node;
		m_totalAllocated--;
	}

	size_t GetAllocatedCount() const { return m_totalAllocated; }
	size_t GetBlockCount() const { return m_blocks.size(); }

private:
	union Node {
		Node* next;
		alignas(T) uint8_t storage[sizeof(T)];
	};

	void AllocateBlock() {
		Node* block = static_cast<Node*>(::operator new(sizeof(Node) * BlockSize));
		m_blocks.push_back(block);

		for (size_t i = 0; i < BlockSize - 1; i++)
			block[i].next = &block[i + 1];
		block[BlockSize - 1].next = m_freeList;
		m_freeList = &block[0];
	}

	Node* m_freeList;
	std::vector<Node*> m_blocks;
	std::mutex m_mutex;
	size_t m_totalAllocated;
};
