/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>

#include <tests/fixtures.h>

/**
 * Declares the CommonTestData as the boost test suite fixture.
 */
BOOST_FIXTURE_TEST_SUITE( PolygonIterator, IteratorFixture )

/**
 *
 */
BOOST_AUTO_TEST_CASE( VertexIterator )
{
    SHAPE_POLY_SET::ITERATOR iterator;

    for( iterator = polySet.IterateWithHoles(); iterator; iterator++ )
    {
        SHAPE_POLY_SET::VERTEX_INDEX index = iterator.GetIndex();

        printf("V [%d, %d, %d] --> (%d, %d)\n", index.m_polygon, index.m_contour, index.m_vertex,
                                                iterator->x, iterator->y);
        if( iterator.IsEndContour() )
            printf("------------------\n");
    }
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( SegmentIterator )
{
    SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;

    for( iterator = polySet.IterateSegmentsWithHoles(); iterator; iterator++ ){
        SHAPE_POLY_SET::VERTEX_INDEX index = iterator.GetIndex();
        SEG segment = *iterator;

        printf("S [%d, %d, %d] --> (%d, %d) - (%d, %d)\n", index.m_polygon, index.m_contour, index.m_vertex, segment.A.x, segment.A.y, segment.B.x, segment.B.y);
    }
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( EmptyPolygon )
{
    SHAPE_POLY_SET emptySet;
    SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;

    for( iterator = emptySet.IterateSegmentsWithHoles(); iterator; iterator++ )
    {
        BOOST_FAIL( "Empty set is being iterated!" );
    }
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( UniqueVertex )
{
    SHAPE_POLY_SET uniqueVertexSet;

    SHAPE_LINE_CHAIN polyLine;

    polyLine.Append( 100, 50 );
    polyLine.SetClosed( true );
    uniqueVertexSet.AddOutline(polyLine);

    SHAPE_POLY_SET::SEGMENT_ITERATOR iterator = uniqueVertexSet.IterateSegmentsWithHoles();

    SEG segment = *iterator;
    BOOST_CHECK( segment.A.x == 100 );
    BOOST_CHECK( segment.B.x == 100 );
    BOOST_CHECK( segment.A.y == 50  );
    BOOST_CHECK( segment.B.y == 50  );

    iterator++;

    BOOST_CHECK( !iterator );
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( TotalVertices )
{
    SHAPE_POLY_SET emptySet;
    BOOST_CHECK_EQUAL( emptySet.TotalVertices(), 0 );
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( RemoveLastNullSegment )
{
    SHAPE_POLY_SET polySet;
    SHAPE_LINE_CHAIN polyLine, hole;

    // Adds a new outline with a null segment
    polyLine.Append( 100,100 );
    polyLine.Append( 0,100 );
    polyLine.Append( 0,0 );
    polyLine.Append( 0,0,true );
    polyLine.SetClosed( true );

    polySet.AddOutline(polyLine);

    BOOST_CHECK_EQUAL( polySet.TotalVertices(), 4 );

    BOOST_CHECK_EQUAL( polySet.RemoveNullSegments(), 1);

    BOOST_CHECK_EQUAL( polySet.TotalVertices(), 3 );
}


/**
 *
 */
BOOST_AUTO_TEST_CASE( RemoveFirstNullSegment )
{
    SHAPE_POLY_SET polySet;
    SHAPE_LINE_CHAIN polyLine, hole;

    // Adds a new outline with a null segment
    polyLine.Append( 100,100 );
    polyLine.Append( 100,100,true );
    polyLine.Append( 0,0 );
    polyLine.Append( 100,0 );
    polyLine.SetClosed( true );

    polySet.AddOutline(polyLine);

    BOOST_CHECK_EQUAL( polySet.TotalVertices(), 4 );

    BOOST_CHECK_EQUAL( polySet.RemoveNullSegments(), 1);

    BOOST_CHECK_EQUAL( polySet.TotalVertices(), 3 );
}



/**
 *
 */
BOOST_AUTO_TEST_CASE( RemoveInsideNullSegment )
{
    SHAPE_POLY_SET polySet;
    SHAPE_LINE_CHAIN polyLine, hole;

    // Adds a new outline with a null segment
    polyLine.Append( 100,100 );
    polyLine.Append( 0,100 );
    polyLine.Append( 0,100,true );
    polyLine.Append( 100,0 );
    polyLine.SetClosed( true );

    polySet.AddOutline(polyLine);

    BOOST_CHECK_EQUAL( polySet.TotalVertices(), 4 );

    BOOST_CHECK_EQUAL( polySet.RemoveNullSegments(), 1);

    BOOST_CHECK_EQUAL( polySet.TotalVertices(), 3 );
}

BOOST_AUTO_TEST_SUITE_END()
