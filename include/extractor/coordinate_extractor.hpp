#ifndef OSRM_EXTRACTOR_COORDINATE_EXTRACTOR_HPP_
#define OSRM_EXTRACTOR_COORDINATE_EXTRACTOR_HPP_

#include <vector>

#include "extractor/query_node.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "util/node_based_graph.hpp"

namespace osrm
{
namespace extractor
{
class CoordinateExtractor
{
  public:
    CoordinateExtractor(const util::NodeBasedDynamicGraph &node_based_graph,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const std::vector<extractor::QueryNode> &node_coordinates);

    // Find a interpolated coordinate a long the compressed geometries. The desired coordinate
    // should be in a certain distance. This method is dedicated to find representative coordinates
    // at turns.
    util::Coordinate GetCoordinateAlongRoad(const NodeID from_node,
                                            const EdgeID via_edge,
                                            const bool traversed_in_reverse,
                                            const NodeID to_node) const;

    // instead of finding only a single coordinate, we can also list all coordinates along a road.
    std::vector<util::Coordinate> GetCoordinatesAlongRoad(const NodeID from_node,
                                                          const EdgeID via_edge,
                                                          const bool traversed_in_reverse,
                                                          const NodeID to_node) const;

    // when looking at a set of coordinates, this function allows trimming the vector to a smaller,
    // only containing coordinates up to a given distance along the path. The last coordinate might
    // be interpolated
    std::vector<util::Coordinate> TrimCoordinatesToLength(std::vector<util::Coordinate> coordinates,
                                                          const double desired_length) const;

    // to correct for the initial offset, we move the lookahead coordinate close
    // to the original road. We do so by subtracting the difference between the
    // turn coordinate and the offset coordinate from the lookahead coordinge:
    //
    // a ------ b ------ c
    //          |
    //          d
    //             \
    //                \
    //                   e
    //
    // is converted to:
    //
    // a ------ b ------ c
    //             \
    //                \
    //                   e
    //
    // for fixpoint `b`, vector_base `d` and vector_head `e`
    util::Coordinate GetCorrectedCoordinate(const util::Coordinate &fixpoint,
                                            const util::Coordinate &vector_base,
                                            const util::Coordinate &vector_head) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const extractor::CompressedEdgeContainer &compressed_geometries;
    const std::vector<extractor::QueryNode> &node_coordinates;
};
} // namespace osm
} // namespace osrm

#endif // OSRM_EXTRACTOR_COORDINATE_EXTRACTOR_HPP_
