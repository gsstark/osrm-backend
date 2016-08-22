#include "extractor/coordinate_extractor.hpp"
#include "extractor/guidance/toolkit.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <utility>

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
const constexpr double FAR_LOOKAHEAD_DISTANCE = 30.0;
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
    // we first extract all coordinates from the road
    auto coordinates = GetCoordinatesAlongRoad(from_node, via_edge, traversed_in_reverse, to_node);

    // if we are looking at a straight line, we don't care where exactly the coordinate
    // is. Simply return the final coordinate. Turn angles/turn vectors are the same no matter which
    // coordinate we look at.
    if (coordinates.size() == 2)
        return coordinates.back();

    // Our very first step trims the coordinates to the FAR_LOOKAHEAD_DISTANCE. The idea here is to
    // filter all coordinates at the end of the road and consider only the form close to the
    // intersection:
    //
    // a -------------- v ----------.
    //                                 .
    //                                   .
    //                                   .
    //                                   b
    //
    // For calculating the turn angle for the intersection at `a`, we do not care about the turn
    // between `v` and `b`. This calculation trims the coordinates to the ones immediately at the
    // intersection.
    std::size_t count = coordinates.size();
    coordinates = TrimCoordinatesToLength(std::move(coordinates), FAR_LOOKAHEAD_DISTANCE);
    // If this reduction leaves us with only two coordinates, the turns/angles are represented in a
    // valid way. Only curved roads and other difficult scenarios will require multiple coordinates.
    if (coordinates.size() == 2)
        return coordinates.back();

    // The coordinates along the road are in different distances from the source. If only very few
    // coordinates are close to the intersection, It might just be we simply looked to far down the
    // road. We can decide to weight coordinates differently based on their distance from the
    // intersection.
    // In addition, changes very close to an intersection indicate graphical representation of the
    // intersection over perceived turn angles.
    //
    // a -
    //    \
    //     -------------------- b
    //
    // Here the initial angle close to a might simply be due to OSM-Ways being located in the middle
    // of the actual roads. If a road splits in two, the ways for the separate direction can be
    // modeled very far apart with a steep angle at the split, even though the roads actually don't
    // take a turn. The distance between the coordinates can be an indicator for these small changes
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

    // We use the sum of least squares to calculate a linea regression through our coordinates. This
    // regression gives a good idea of how the road can be perceived and corrects for initial and
    // final corrections
    const auto regression_vector =
        [coordinates]() -> std::pair<util::Coordinate, util::Coordinate> {
        double sum_lon = 0, sum_lat = 0, sum_lon_lat = 0, sum_lon_lon = 0;
        double min_lon = (double)toFloating(coordinates.front().lon);
        double max_lon = (double)toFloating(coordinates.front().lon);
        for (const auto coord : coordinates)
        {
            min_lon = std::min(min_lon, (double)toFloating(coord.lon));
            max_lon = std::max(max_lon, (double)toFloating(coord.lon));
            sum_lon += (double)toFloating(coord.lon);
            sum_lon_lon += (double)toFloating(coord.lon) * (double)toFloating(coord.lon);
            sum_lat += (double)toFloating(coord.lat);
            sum_lon_lat += (double)toFloating(coord.lon) * (double)toFloating(coord.lat);
        }

        const auto dividend = coordinates.size() * sum_lon_lat - sum_lon * sum_lat;
        const auto divisor = coordinates.size() * sum_lon_lon - sum_lon * sum_lon;
        if (std::abs(divisor) < std::numeric_limits<double>::epsilon())
            return std::make_pair(coordinates.front(), coordinates.back());

        // slope of the regression line
        const auto slope = dividend / divisor;
        const auto intercept = (sum_lat - slope * sum_lon) / coordinates.size();

        const auto GetLatatLon =
            [intercept, slope](const util::FloatLongitude longitude) -> util::FloatLatitude {
            return {intercept + slope * (double)(longitude)};
        };

        const util::Coordinate regression_first = {
            toFixed(util::FloatLongitude{min_lon - 1}),
            toFixed(util::FloatLatitude(GetLatatLon(util::FloatLongitude{min_lon - 1})))};
        const util::Coordinate regression_end = {
            toFixed(util::FloatLongitude{max_lon + 1}),
            toFixed(util::FloatLatitude(GetLatatLon(util::FloatLongitude{max_lon + 1})))};

        return {regression_first, regression_end};
    }();

    // In an ideal world, the road would only have two coordinates if it goes mainly straigt. Since
    // OSM is operating on noisy data, we have some variations going straight.
    //
    //              b                       d
    // a ---------------------------------------------- e
    //                           c
    //
    // The road from a-e offers a lot of variation, even though it is mostly straight. Here we
    // calculate the distances of all nodes in between to the straight line between a and e. If the
    // distances inbetween are small, we assume a straight road. To calculate these distances, we
    // don't use the coordinates of the road itself but our just calculated regression vector
    const double max_deviation_from_straight = [coordinates, regression_vector]() {
        double deviation = 0;
        for (std::size_t coord_index = 0; coord_index + 1 < coordinates.size(); ++coord_index)
        {
            // find the projected coordinate
            auto coord_between =
                util::coordinate_calculation::projectPointOnSegment(
                    regression_vector.first, regression_vector.second, coordinates[coord_index])
                    .second;
            // and calculate the distance between the intermediate coordinate and the coordinate on
            // the osrm-way
            deviation = std::max(deviation,
                                 util::coordinate_calculation::haversineDistance(
                                     coord_between, coordinates[coord_index]));
        }
        return deviation;
    }();

    // if the deviation from a straight line is small, we can savely use the coordinate. We use half
    // a lane as heuristic to determine if the road is straight enough.
    if (max_deviation_from_straight < 0.5 * ASSUMED_LANE_WIDTH)
        return coordinates.back();

    // The second indicater is the lanes at the intersections. We have to consider both the lanes at
    // the incoming road and the destination way since both counts can indicate how far a road would
    // have to go to connect two ways.
    const auto number_of_destination_lanes =
        node_based_graph.GetEdgeData(via_edge).road_classification.GetNumberOfLanes();
    if (coordinate_distances[1] >
        number_of_destination_lanes * ASSUMED_LANE_WIDTH * 0.5 + LOOKAHEAD_DISTANCE_WITHOUT_LANES)
        return coordinates[1];

    BOOST_ASSERT(coordinates.size() >= 3);
    const auto turn_angles = [coordinates]() {
        std::vector<double> turn_angles;
        turn_angles.reserve(coordinates.size() - 2);
        for (std::size_t index = 0; index + 2 < coordinates.size(); ++index)
        {
            turn_angles.push_back(util::coordinate_calculation::computeAngle(
                coordinates[index], coordinates[index + 1], coordinates[index + 2]));
        }
        return turn_angles;
    }();

    // detect curves: If we see many coordinates that follow a similar turn angle, we assume a curve
    const bool has_many_coordinates = coordinates.size() >= 5;
    const bool all_angles_are_similar = [&turn_angles]() {
        for (std::size_t i = 1; i < turn_angles.size(); ++i)
        {
            const constexpr double ANGLE_SIMILARITY_ALLOWNESS = 5;
            if (guidance::angularDeviation(turn_angles[i - 1], turn_angles[i]) >
                ANGLE_SIMILARITY_ALLOWNESS)
                return false;
        }
        return true;
    }();

    // the curve is still best described as looking at the very first vector for the turn angle.
    // Consider:
    //
    // |
    // a - 1
    // |       o
    // |         2
    // |          o
    // |           3
    // |           o
    // |           4
    //
    // The turn itself from a-1 would be considered as a 90 degree turn, even though the road is
    // taking a turn later.
    // In this situaiton we return the very first coordinate, describing the road just at the turn.
    // As an added benefit, we get a straight turn at a curved road:
    //
    //            o   b   o
    //      o                   o
    //   o                         o
    //  o                           o
    //  o                           o
    //  a                           c
    //
    // The turn from a-b to b-c is straight. With every vector we go further down the road, the turn
    // angle would get stronger. Therefore we consider the very first coordinate as our best choice
    if (has_many_coordinates && all_angles_are_similar)
        return coordinates[1];

    if (coordinates.size() + 1 < count)
    {
        const util::Coordinate turn_coordinate =
            node_coordinates[traversed_in_reverse ? to_node : from_node];
        const util::Coordinate destination_coordinate =
            node_coordinates[traversed_in_reverse ? from_node : to_node];
        std::cout << "Swtiched size: " << count << " to " << coordinates.size()
                  << " at: " << std::setprecision(12) << toFloating(turn_coordinate.lat) << " "
                  << toFloating(turn_coordinate.lon) << " - " << toFloating(coordinates.back().lat)
                  << " " << toFloating(coordinates.back().lon) << " - "
                  << toFloating(destination_coordinate.lat) << " "
                  << toFloating(destination_coordinate.lon)
                  << " Regression: " << toFloating(regression_vector.first.lat) << " "
                  << toFloating(regression_vector.first.lon) << " - "
                  << toFloating(regression_vector.second.lat) << " "
                  << toFloating(regression_vector.second.lon) << ":\n\t" << std::setprecision(5);
        std::cout << "Angles:";
        for (auto angle : turn_angles)
            std::cout << " " << (int)angle;
        std::cout << " Distances:";
        double distance = 0;
        for (std::size_t i = 0; i < coordinates.size(); ++i)
        {
            if (i > 0)
                distance += util::coordinate_calculation::haversineDistance(coordinates[i - 1],
                                                                            coordinates[i]);
            std::cout << " " << distance;
            auto coord_between =
                util::coordinate_calculation::projectPointOnSegment(
                    regression_vector.first, regression_vector.second, coordinates[i])
                    .second;
            std::cout << " (" << util::coordinate_calculation::haversineDistance(coord_between,
                                                                                 coordinates[i])
                      << ")";
        }
        std::cout << std::endl;
    }
    return TrimCoordinatesToLength(std::move(coordinates), 10.0).back();
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
