@routing  @guidance @turn-angles
Feature: Simple Turns

    Background:
        Given the profile "car"
        Given a grid size of 1 meters

    Scenario: Offset Turn
        Given the node map
            | a |   |   |   |   |   |   |   |   | b |   |   |   |   |   |   |   |   |   | c |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | d |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | e |   |

        And the ways
            | nodes  | highway | name | lanes |
            | abc    | primary | road | 4     |
            | bde    | primary | turn | 2     |

       When I route I should get
            | waypoints | route          | turns                           |
            | a,c       | road,road      | depart,arrive                   |
            | a,e       | road,turn,turn | depart,turn slight right,arrive |
            | e,a       | turn,road,road | depart,turn slight left,arrive  |
            | e,c       | turn,road,road | depart,turn sharp right,arrive  |

    Scenario: Road Taking a Turn after Intersection
        Given the node map
            | a |   |   |   |   |   |   |   |   | b |   |   |   |   |   |   |   |   |   | c |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | d |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | e |   |

        And the ways
            | nodes  | highway | name | lanes |
            | abc    | primary | road | 4     |
            | bde    | primary | turn | 2     |

       When I route I should get
            | waypoints | route          | turns                    |
            | a,c       | road,road      | depart,arrive            |
            | a,e       | road,turn,turn | depart,turn right,arrive |
            | e,a       | turn,road,road | depart,turn left,arrive  |
            | e,c       | turn,road,road | depart,turn right,arrive |

    Scenario: U-Turn Lane
        Given the node map
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | j |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | i |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | g |   | h |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | f |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   | d |   |   |   |   |   |   |   |   | e |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | c |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | a |   | k |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | b |

        And the ways
            | nodes   | highway      | name | lanes | oneway |
            | akb     | primary      | road | 4     | yes    |
            | hgi     | primary      | road | 4     | yes    |
            | akcdefg | primary_link |      | 1     | yes    |
            | gj      | tertiary     | turn | 1     |        |

       When I route I should get
            | waypoints | route          | turns                        |
            | a,b       | road,road      | depart,arrive                |
            | a,i       | road,road,road | depart,continue uturn,arrive |
            | a,j       | road,turn,turn | depart,turn left,arrive      |

    #http://www.openstreetmap.org/#map=19/52.50871/13.26127
    Scenario: Curved Turn
        Given the node map
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | f |   |   |   |   | e |   |   |   |   |   |   |   |   |   |   |   |   |   |   | d |
            |   |   |   |   |   |   |   |   | l |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   | k |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | j |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | i |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | h |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | a |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | b |   | c |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | g |   |   |

        And the ways
            | nodes   | highway      | name | lanes | oneway |
            | abc     | primary      | road | 5     | no     |
            | gb      | secondary    | turn | 3     | yes    |
            | bhijkle | unclassified | turn | 2     | yes    |
            | de      | residential  | road |       | yes    |
            | ef      | residential  | road | 2     | yes    |

       When I route I should get
            | waypoints | route               | turns                              |
            | a,c       | road,road           | depart,arrive                      |
            | c,a       | road,road           | depart,arrive                      |
            | g,a       | turn,road,road      | depart,turn left,arrive            |
            | g,c       | turn,road,road      | depart,turn right,arrive           |
            | g,f       | turn,road,road      | depart,turn left,arrive            |
            | c,f       | road,turn,road,road | depart,turn right,turn left,arrive |

    # http://www.openstreetmap.org/#map=19/52.48753/13.52838
    Scenario: Traffic Circle
        Given the node map
            |   |   |   |   |   |   |   |   |   | l |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   | m |   |   |   |   |   |   | k |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | j |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   | n |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | i |   |   |   | p |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   | o |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | h |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | a |   | b |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | g |   |   |   |   |   |
            |   |   |   | c |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   | d |   |   |   |   |   |   |   | f |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | e |   |   |   |   |   |   |   |   |   |   |   |

        And the ways
            | nodes           | highway     | name | lanes | oneway | junction   |
            | ab              | residential | road | 1     | yes    |            |
            | ip              | residential | road | 1     | yes    |            |
            | bcdefghijklmnob | residential | road | 1     | yes    | roundabout |

       When I route I should get
            | waypoints | route               | turns                                   | intersections |
            | a,c       | road,road,road      | depart,roundabout-exit-undefined,arrive |               |


    #http://www.openstreetmap.org/#map=19/52.47587/13.53600
    Scenario: Curved Turn
        Given the node map
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | h |
            | a |   |   |   |   |   |   | b |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   | c |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   | d |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   | e |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | f |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | g |

        And the ways
            | nodes | highway     | name   | lanes |
            | abcd  | residential | first  | 1     |
            | defg  | residential | second | 1     |
            | dh    | residential | turn   | 1     |

        When I route I should get
            | waypoints | route               | turns                        |
            | a,g       | first,second,second | depart,new name right,arrive |
            | a,h       | first,turn,turn     | depart,turn left,arrive      |
            | g,h       | second,turn,turn    | depart,turn right,arrive     |
            | g,a       | second,first,first  | depart,new name left,arrive  |

    Scenario: Splitting Road with many lanes
        Given the node map
            |   |   |   |   |   |   |   |   |   | f |   | e |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            | a |   |   |   |   |   | b |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |   |   |   |
            |   |   |   |   |   |   |   |   |   | c |   | d |

        And the ways
            | nodes | highway | name | lanes | oneway |
            | ab    | primary | road | 4     | no     |
            | bcd   | primary | road | 2     | yes    |
            | feb   | primary | road | 2     | yes    |

        When I route I should get
            | waypoints | route     | turns         | intersections |
            | a,d       | road,road | depart,arrive |               |
            | e,a       | road,road | depart,arrive |               |
