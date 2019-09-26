/******************************************************************************
 * heavy_edges.h
 *
 * Source of VieCut
 *
 ******************************************************************************
 * Copyright (C) 2019 Alexander Noe <alexander.noe@univie.ac.at>
 *
 * Published under the MIT license in the LICENSE file.
 *****************************************************************************/

#pragma once

#include <algorithm>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/definitions.h"
#include "data_structure/mutable_graph.h"

class heavy_edges {
 public:
    
    using NeighboursAndContents = 
    std::vector<std::pair<std::vector<std::pair<NodeID, EdgeWeight> >, 
        std::vector<NodeID> > >;
    heavy_edges(EdgeWeight mincut) : mincut(mincut) { }

    std::vector<std::tuple<NodeID, std::vector<NodeID> > > removeHeavyEdges(
        std::shared_ptr<mutable_graph> G) {
        std::vector<std::tuple<NodeID, std::vector<NodeID> > > cactusEdge;
        std::unordered_map<NodeID, std::vector<NodeID> > contract;
        std::vector<NodeID> markForCactus;
        for (NodeID n : G->nodes()) {
            if (G->isEmpty(n))
                continue;

            for (EdgeID e : G->edges_of(n)) {
                EdgeWeight wgt = G->getEdgeWeight(n, e);
                NodeID target = G->getEdgeTarget(n, e);

                if (G->isEmpty(target))
                    continue;

                if (wgt > mincut) {
                    NodeID v1 = G->containedVertices(n)[0];
                    NodeID v2 = G->containedVertices(target)[0];
                    NodeID min = std::min(v1, v2);
                    NodeID max = std::max(v1, v2);

                    if (contract.find(min) == contract.end()) {
                        contract[min] = { max };
                    } else {
                        contract[min].emplace_back(max);
                    }
                }

                if (wgt == mincut) {
                    if (G->get_first_invalid_edge(n) == 1) {
                        // each edge is seen from both adjacent nodes
                        // so we get all edges
                        markForCactus.emplace_back(G->containedVertices(n)[0]);
                    }
                }
            }
        }

        for (const auto& [lowest, others] : contract) {
            std::unordered_set<NodeID> vtxset;
            vtxset.insert(G->getCurrentPosition(lowest));
            for (const auto& v : others) {
                vtxset.insert(G->getCurrentPosition(v));
            }
            if (vtxset.size() > 1) {
                G->contractVertexSet(vtxset);
            }
        }

        for (const NodeID& e : markForCactus) {
            if (G->n() > 2) {
                NodeID n = G->getCurrentPosition(e);
                VIECUT_ASSERT_EQ(G->get_first_invalid_edge(n), 1);
                NodeID t = G->getEdgeTarget(n, 0);
                if (G->isEmpty(t)) {
                    continue;
                }
                NodeID vtx_in_t = G->containedVertices(t)[0];
                cactusEdge->emplace_back(vtx_in_t, G->containedVertices(n));
                G->deleteVertex(n);
            }
        }
        return cactusEdge;
    }

    std::vector<std::tuple<std::pair<NodeID, NodeID>, std::vector<NodeID> > >
    contractCycleEdges(std::shared_ptr<mutable_graph> G) {
        std::vector<std::tuple<std::pair<NodeID, NodeID>,
                               std::vector<NodeID> > > cycleEdges;
        // as we contract edges, we use basic for loop so G->n() can update
        for (NodeID n = 0; n < G->n(); ++n) {
            if (G->get_first_invalid_edge(n) == 2
                && G->getWeightedNodeDegree(n) == mincut) {
                NodeID n0 = G->getEdgeTarget(n, 0);
                NodeID n1 = G->getEdgeTarget(n, 1);
                if (G->isEmpty(n0) || G->isEmpty(n1))
                    continue;
                // if the edges have different weights, the heavier of them will
                // be contracted in local routines
                if (G->getEdgeWeight(n, 0) != mincut / 2
                    || G->getEdgeWeight(n, 1) != mincut / 2)
                    continue;
                
                NodeID p0 = G->containedVertices(n0)[0];
                NodeID p1 = G->containedVertices(n1)[0];
                auto contained = G->containedVertices(n);
                G->setContainedVertices(n, { });
                for (const auto& c : contained) {
                    G->setCurrentPosition(c, UNDEFINED_NODE);
                }
                G->contractEdge(n0, G->getReverseEdge(n, 0));
                cycleEdges.emplace_back(std::make_pair(p0, p1), contained);
            }
        }
        return cycleEdges;
    }

    }

        }
    }

    void reInsertCycles(
        std::shared_ptr<mutable_graph> G,
        std::vector<std::tuple<std::pair<NodeID, NodeID>,
                               std::vector<NodeID> > > toInsert) {
        for (size_t i = toInsert.size(); i-- > 0; ) {
            const auto& [p, cont] = toInsert[i];
            NodeID n0 = G->getCurrentPosition(p.first);
            NodeID n1 = G->getCurrentPosition(p.second);
            if (n0 == n1) {
                NodeID reIns = G->new_empty_node();
                G->new_edge_order(n0, reIns, mincut);
                G->setContainedVertices(reIns, cont);
                for (NodeID v : cont) {
                    G->setCurrentPosition(v, reIns);
                }
            } else {
                EdgeID e = UNDEFINED_EDGE;
                for (EdgeID arc : G->edges_of(n0)) {
                    if (G->getEdgeTarget(n0, arc) == n1) {
                        e = arc;
                        break;
                    }
                }

                NodeID reIns = G->new_empty_node();
                G->new_edge_order(n0, reIns, mincut / 2);
                G->new_edge_order(n1, reIns, mincut / 2);

                EdgeWeight w01 = G->getEdgeWeight(n0, e);
                if (w01 == (mincut / 2)) {
                    G->deleteEdge(n0, e);
                } else {
                    G->setEdgeWeight(n0, e, w01 - (mincut / 2));
                }
                G->setContainedVertices(reIns, cont);
                for (NodeID v : cont) {
                    G->setCurrentPosition(v, reIns);
                }
            }
        }
    }

    void reInsertVertices(
        std::shared_ptr<mutable_graph> G,
        std::vector<std::tuple<NodeID, std::vector<NodeID> > > toInsert) {
        for (size_t i = toInsert.size(); i-- > 0; ) {
            const auto& [t, cont] = toInsert[i];
            NodeID curr = G->getCurrentPosition(t);
            NodeID vtx = G->new_empty_node();
            G->new_edge_order(curr, vtx, mincut);
            G->setContainedVertices(vtx, cont);
            for (const auto& e : G->containedVertices(vtx)) {
                G->setCurrentPosition(e, vtx);
            }
        }
    }

 private:
    EdgeWeight mincut;
};