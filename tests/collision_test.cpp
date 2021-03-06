/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Michele Castellana <michele.castellana@cern.ch>
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
 * Declares the CollisionFixture as the boost test suite fixture.
 */
BOOST_FIXTURE_TEST_SUITE( Collision, CollisionFixture )

/**
 * Simple dummy test to check that HasHoles() definition is right
 */
BOOST_AUTO_TEST_CASE( HasHoles )
{
    SHAPE_POLY_SET solidPolySet;

    // Add dummy outlines in the solid polygon set
    solidPolySet.NewOutline();
    solidPolySet.NewOutline();
    solidPolySet.NewOutline();

    BOOST_CHECK( !solidPolySet.HasHoles() );
    BOOST_CHECK(  polySet.HasHoles() );
}

/**
 *
 */
BOOST_AUTO_TEST_CASE( PointOnEdge )
{
    // Check points on corners
    BOOST_CHECK( polySet.PointOnEdge( VECTOR2I( 0,50 ) ) );

    // Check points on outline edges
    BOOST_CHECK( polySet.PointOnEdge( VECTOR2I( 0,10 ) ) );

    // Check points on hole edges
    BOOST_CHECK( polySet.PointOnEdge( VECTOR2I( 10,11 ) ) );
}

/**
 * This test checks that the function Contains, whose behaviour has been updated to also manage
 * holey polygons, does the right work.
 */
BOOST_AUTO_TEST_CASE( pointInPolygonSet )
{
    // Check that the set contains the point that collide with it
    for( const VECTOR2I& point : collidingPoints )
    {
        BOOST_CHECK( polySet.Contains( point ) );
    }

    // Check that the set does not contain any point outside of it
    for( const VECTOR2I& point : nonCollidingPoints )
    {
        BOOST_CHECK( !polySet.Contains( point ) );
    }
}

/**
 * This test checks that the behaviour of the Collide (with a point) method works well.
 */
BOOST_AUTO_TEST_CASE( Collide )
{
    // When clearence = 0, the behaviour should be the same as with Contains

    // Check that the set collides with the colliding points
    for( const VECTOR2I& point : collidingPoints )
    {
        BOOST_CHECK( polySet.Collide( point, 0 ) );
    }

    // Check that the set does not collide with the non colliding points
    for( const VECTOR2I& point : nonCollidingPoints )
    {
        BOOST_CHECK( !polySet.Collide( point, 0 ) );
    }

    // Checks with clearence > 0

    // Point at the offseted zone outside of the outline => collision!
    BOOST_CHECK( polySet.Collide( VECTOR2I( -1,10 ), 5 ) );

    // Point at the offseted zone outside of a hole => collision!
    BOOST_CHECK( polySet.Collide( VECTOR2I( 11,11 ), 5 ) );
}

BOOST_AUTO_TEST_SUITE_END()
