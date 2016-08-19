#include "extractor/coordinate_extractor.hpp"

#include <iomanip>

namespace osrm
{
namespace extractor
{
namespace
{
// to use the corrected coordinate, we require it to be at least a bit further down the
// road than the offset coordinate. We postulate a minimum Distance of 2 Meters
const constexpr double DESIRED_COORDINATE_DIFFERENCE = 2.0;
const constexpr double LOOKAHEAD_DISTANCE = 2.5;
// the default distance we lookahead on a road. This distance prevents small mapping
// errors to impact the turn angles.
const constexpr double LOOKAHEAD_DISTANCE_WITHOUT_LANES = 10.0;
// The standard with of a interstate highway is 3.7 meters. Local roads have
// smaller widths, ranging from 2.5 to 3.25 meters. As a compromise, we use
// the 3.25 here for our angle calculations
const constexpr double ASSUMED_LANE_WIDTH = 3.25;
const constexpr double FAR_LOOKAHEAD_DISTANCE = 20.0;
}

CoordinateExtractor::CoordinateExtractor(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const std::vector<extractor::QueryNode> &node_coordinates)
    : node_based_graph(node_based_graph), compressed_geometries(compressed_geometries),
      node_coordinates(node_coordinates)
{
}

util::Coordinate CoordinateExtractor::GetCoordinateAlongRoad(const NodeID from_node,
                                                             const EdgeID via_edge,
                                                             const bool traversed_in_reverse,
                                                             const NodeID to_node) const
{
    auto coordinates = GetCoordinatesAlongRoad(from_node, via_edge, traversed_in_reverse, to_node);

    // if we are looking at a straight line, we don't care where exactly the coordinate
    // is. Simply return the final coordinate.
    if (coordinates.size() == 2)
        return coordinates.back();

    std::size_t count = coordinates.size();
    coordinates = TrimCoordinatesToLength(std::move(coordinates), FAR_LOOKAHEAD_DISTANCE);

    // if the next coordinate (except for an initial offset that could get trimmed) is
    // 30 meters away, we can use it directly.
    if (coordinates.size() == 2)
        return coordinates.back();

    const std::vector<double> coordinate_distances = [coordinates]() {
        std::vector<double> coordinate_distances;
        coordinate_distances.reserve(coordinates.size());
        coordinate_distances.push_back(0);
        for (std::size_t coord_index = 1; coord_index < coordinates.size(); ++coord_index)
            coordinate_distances.push_back(
                coordinate_distances.back() +
                util::coordinate_calculation::haversineDistance(coordinates[coord_index - 1],
                                                                coordinates[coord_index]));
        return coordinate_distances;
    }();

    const auto number_of_destination_lanes =
        node_based_graph.GetEdgeData(via_edge).road_classification.GetNumberOfLanes();
    if (coordinate_distances[1] >
        number_of_destination_lanes * ASSUMED_LANE_WIDTH * 0.5 + LOOKAHEAD_DISTANCE_WITHOUT_LANES)
        return coordinates[1];

    const double max_deviation_from_straight = [coordinates]() {
        double deviation = 0;
        for (std::size_t coord_index = 1; coord_index + 1 < coordinates.size(); ++coord_index)
        {
            auto coord_between = util::coordinate_calculation::projectPointOnSegment(
                                     coordinates[0], coordinates.back(), coordinates[coord_index])
                                     .second;
            deviation = std::max(deviation,
                                 util::coordinate_calculation::haversineDistance(
                                     coord_between, coordinates[coord_index]));
        }
        return deviation;
    }();

    // if the deviation from a straight line is small, we can savely use the coordinate
    if (max_deviation_from_straight < 0.5 * ASSUMED_LANE_WIDTH)
        return coordinates.back();

    if (coordinates.size() + 1 < count)
    {
        const util::Coordinate turn_coordinate = node_coordinates[from_node];
        const util::Coordinate destination_coordinate = node_coordinates[to_node];
        std::cout << "Swtiched size: " << count << " to " << coordinates.size()
                  << " at: " << std::setprecision(12) << toFloating(turn_coordinate.lat) << " "
                  << toFloating(turn_coordinate.lon) << " - "
                  << toFloating(destination_coordinate.lat) << " "
                  << toFloating(destination_coordinate.lon) << ":";
        double distance = 0;
        for (std::size_t i = 1; i < coordinates.size(); ++i)
        {
            distance +=
                util::coordinate_calculation::haversineDistance(coordinates[i - 1], coordinates[i]);
            std::cout << " " << distance;
            auto coord_between = util::coordinate_calculation::projectPointOnSegment(
                                     coordinates[0], coordinates.back(), coordinates[i])
                                     .second;
            std::cout << " (" << util::coordinate_calculation::haversineDistance(coord_between,
                                                                                 coordinates[i])
                      << ")";
        }
        std::cout << std::endl;
    }
}

std::vector<util::Coordinate>
CoordinateExtractor::GetCoordinatesAlongRoad(const NodeID from_node,
                                             const EdgeID via_edge,
                                             const bool traversed_in_reverse,
                                             const NodeID to_node) const
{
    if (!compressed_geometries.HasEntryForID(via_edge))
    {
        if (traversed_in_reverse)
            return {{node_coordinates[to_node]}, {node_coordinates[from_node]}};
        else
            return {{node_coordinates[from_node]}, {node_coordinates[to_node]}};
    }
    else
    {
        // extracts the geometry in coordinates from the compressed edge container
        std::vector<util::Coordinate> result;
        const auto &geometry = compressed_geometries.GetBucketReference(via_edge);
        result.reserve(geometry.size() + 2);

        // the compressed edges contain node ids, we transfer them to coordinates accessing the
        // node_coordinates array
        const auto compressedGeometryToCoordinate = [this](
            const CompressedEdgeContainer::CompressedEdge &compressed_edge) -> util::Coordinate {
            return node_coordinates[compressed_edge.node_id];
        };

        // add the coordinates to the result in either normal or reversed order, based on
        // traversed_in_reverse
        if (traversed_in_reverse)
        {
            result.push_back(node_coordinates[to_node]);
            std::transform(geometry.rbegin(),
                           geometry.rend(),
                           std::back_inserter(result),
                           compressedGeometryToCoordinate);
            result.push_back(node_coordinates[from_node]);
        }
        else
        {
            result.push_back(node_coordinates[from_node]);
            std::transform(geometry.begin(),
                           geometry.end(),
                           std::back_inserter(result),
                           compressedGeometryToCoordinate);
            result.push_back(node_coordinates[to_node]);
        }
        return result;
    }
}

std::vector<util::Coordinate>
CoordinateExtractor::TrimCoordinatesToLength(std::vector<util::Coordinate> coordinates,
                                             const double desired_length) const
{
    double distance_to_current_coordinate = 0;

    const auto getFactor = [desired_length](const double first_distance,
                                            const double second_distance) {
        BOOST_ASSERT(first_distance < desired_length);
        double segment_length = second_distance - first_distance;
        BOOST_ASSERT(segment_length > 0);
        BOOST_ASSERT(second_distance >= desired_length);
        double missing_distance = desired_length - first_distance;
        return std::max(0., std::min(missing_distance / segment_length, 1.0));
    };

    for (std::size_t coordinate_index = 1; coordinate_index < coordinates.size();
         ++coordinate_index)
    {
        const auto distance_to_next_coordinate =
            distance_to_current_coordinate +
            util::coordinate_calculation::haversineDistance(coordinates[coordinate_index - 1],
                                                            coordinates[coordinate_index]);

        // if we reached the number of coordinates, we can stop here
        if (distance_to_next_coordinate >= desired_length)
        {
            coordinates.resize(coordinate_index + 1);
            coordinates.back() = util::coordinate_calculation::interpolateLinear(
                getFactor(distance_to_current_coordinate, distance_to_next_coordinate),
                coordinates[coordinate_index - 1],
                coordinates[coordinate_index]);
            break;
        }

        // remember the accumulated distance
        distance_to_current_coordinate = distance_to_next_coordinate;
    }
    if (coordinates.size() > 2 &&
        util::coordinate_calculation::haversineDistance(coordinates[0], coordinates[1]) <= 1)
        coordinates.erase(coordinates.begin() + 1);
    return coordinates;
}

util::Coordinate
CoordinateExtractor::GetCorrectedCoordinate(const util::Coordinate &fixpoint,
                                            const util::Coordinate &vector_base,
                                            const util::Coordinate &vector_head) const
{
    // if the coordinates are close together, we were not able to look far ahead, so
    // we can use the end-coordinate
    if (util::coordinate_calculation::haversineDistance(vector_base, vector_head) <
        DESIRED_COORDINATE_DIFFERENCE)
        return vector_head;
    else
    {
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
        // for turn node `b`, vector_base `d` and vector_head `e`
        const auto corrected_lon = vector_head.lon - vector_base.lon + fixpoint.lon;
        const auto corrected_lat = vector_head.lat - vector_base.lat + fixpoint.lat;

        return util::Coordinate(corrected_lon, corrected_lat);
    }
};

} // namespace osm
} // namespace osrm
