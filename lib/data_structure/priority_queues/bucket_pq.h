/******************************************************************************
 * bucket_pq.h
 *
 * Source of VieCut.
 *
 * Adapted from KaHIP.
 *
 ******************************************************************************
 * Copyright (C) 2013-2015 Christian Schulz <christian.schulz@kit.edu>
 * Copyright (C) 2018 Alexander Noe <alexander.noe@univie.ac.at>
 *
 * Published under the MIT license in the LICENSE file.
 *****************************************************************************/

#pragma once

#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "data_structure/priority_queues/priority_queue_interface.h"

namespace VieCut {
    class bucket_pq : public priority_queue_interface {
     public:
        bucket_pq(NodeID, EdgeWeight gain_span) : bucket_pq(gain_span) { }
        explicit bucket_pq(const EdgeWeight& gain_span);

        virtual ~bucket_pq() { }

        NodeID size();
        void insert(NodeID id, Gain gain);
        bool empty();

        Gain maxValue();
        NodeID maxElement();
        NodeID deleteMax();

        void decreaseKey(NodeID node, Gain newGain);
        void increaseKey(NodeID node, Gain newGain);

        void changeKey(NodeID element, Gain newKey);
        Gain getKey(NodeID element);
        void deleteNode(NodeID node);

        bool contains(NodeID node);
        Gain gain(NodeID Node);

     private:
        NodeID m_elements;
        EdgeWeight m_gain_span;
        unsigned m_max_idx;

        std::unordered_map<NodeID, std::pair<Count, Gain> > m_queue_index;
        std::vector<std::vector<NodeID> > m_buckets;
    };

    inline bucket_pq::bucket_pq(const EdgeWeight& gain_span_input) {
        m_elements = 0;
        m_gain_span = gain_span_input;
        m_max_idx = 0;

        m_buckets.resize(2 * m_gain_span + 1);
    }

    inline NodeID bucket_pq::size() {
        return m_elements;
    }

    inline void bucket_pq::insert(NodeID node, Gain gain) {
        unsigned address = gain + m_gain_span;
        if (address > m_max_idx) {
            m_max_idx = address;
        }

        m_buckets[address].push_back(node);
        m_queue_index[node].first = m_buckets[address].size() - 1;
        m_queue_index[node].second = gain;

        m_elements++;
    }

    inline bool bucket_pq::empty() {
        return m_elements == 0;
    }

    inline Gain bucket_pq::maxValue() {
        return m_max_idx - m_gain_span;
    }

    inline NodeID bucket_pq::maxElement() {
        return m_buckets[m_max_idx].back();
    }

    inline NodeID bucket_pq::deleteMax() {
        NodeID node = m_buckets[m_max_idx].back();
        m_buckets[m_max_idx].pop_back();
        m_queue_index.erase(node);

        if (m_buckets[m_max_idx].size() == 0) {
            // update max_idx
            while (m_max_idx != 0) {
                m_max_idx--;
                if (m_buckets[m_max_idx].size() > 0) {
                    break;
                }
            }
        }

        m_elements--;
        return node;
    }

    inline void bucket_pq::decreaseKey(NodeID node, Gain new_gain) {
        changeKey(node, new_gain);
    }

    inline void bucket_pq::increaseKey(NodeID node, Gain new_gain) {
        changeKey(node, new_gain);
    }

    inline Gain bucket_pq::getKey(NodeID node) {
        return m_queue_index[node].second;
    }

    inline void bucket_pq::changeKey(NodeID node, Gain new_gain) {
        deleteNode(node);
        insert(node, new_gain);
    }

    inline void bucket_pq::deleteNode(NodeID node) {
        VIECUT_ASSERT_TRUE(m_queue_index.find(node) != m_queue_index.end());
        Count in_bucket_idx = m_queue_index[node].first;
        Gain old_gain = m_queue_index[node].second;
        unsigned address = old_gain + m_gain_span;

        if (m_buckets[address].size() > 1) {
            // swap current element with last element and pop_back
            m_queue_index[m_buckets[address].back()].first = in_bucket_idx;
            std::swap(m_buckets[address][in_bucket_idx], m_buckets[address].back());
            m_buckets[address].pop_back();
        } else {
            // size is 1
            m_buckets[address].pop_back();
            if (address == m_max_idx) {
                // update max_idx
                while (m_max_idx != 0) {
                    m_max_idx--;
                    if (m_buckets[m_max_idx].size() > 0) {
                        break;
                    }
                }
            }
        }

        m_elements--;
        m_queue_index.erase(node);
    }

    inline bool bucket_pq::contains(NodeID node) {
        return m_queue_index.find(node) != m_queue_index.end();
    }

    inline Gain bucket_pq::gain(NodeID node) {
        auto it_node = m_queue_index.find(node);

        if (it_node == m_queue_index.end()) {
            return 0;
        } else {
            return it_node->second.second;
        }
    }
}
