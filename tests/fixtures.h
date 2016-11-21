#ifndef __FIXTURES_H
#define __FIXTURES_H

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <polygon/PolyLine.h>

/**
 * Common data for the tests: a polyset containing one single squared outline with two
 * holes: a non-convex pentagon and a triangle.
 */
struct CommonTestData
{
    // Polygon set common for all the tests
    SHAPE_POLY_SET polySet;

    CommonTestData()
    {
        SHAPE_LINE_CHAIN polyLine, hole;

        // Adds a new squared outline
        polyLine.Append( 100,100 );
        polyLine.Append( 0,100 );
        polyLine.Append( 0,0 );
        polyLine.Append( 100,0 );
        polyLine.SetClosed( true );

        polySet.AddOutline(polyLine);

        // Adds a new hole (a pentagon)
        hole.Append( 10,10 );
        hole.Append( 10,20 );
        hole.Append( 15,15 );
        hole.Append( 20,20 );
        hole.Append( 20,10 );
        hole.SetClosed( true );
        polySet.AddHole( hole );

        hole.Clear();

        // Adds a new hole (a triangle)
        hole.Append( 40,10 );
        hole.Append( 40,20 );
        hole.Append( 60,10 );
        hole.SetClosed( true );
        polySet.AddHole( hole );
    }

    ~CommonTestData(){}
};

/**
 * Fixture for the ChamferFillet test suite. It contains a copy of the common polygon set and the
 * same polygon replicated as a CPolyLine to test behaviour of old and new Chamfer and Fillet
 * mehods.
 */
struct ChamferFilletFixture {
    // Polygon set common for all the tests
    SHAPE_POLY_SET polySet;

    // CPolyLine representing the same polygon in polySet;
    CPolyLine legacyPolyLine;

    ChamferFilletFixture()
    {
        // Get a copy of the polySet
        polySet = CommonTestData().polySet;

        // Replicate the vertices in the polySet outline
        legacyPolyLine.Start( 0, 100, 100, CPolyLine::NO_HATCH );
        legacyPolyLine.AppendCorner( 0,100 );
        legacyPolyLine.AppendCorner( 0,0 );
        legacyPolyLine.AppendCorner( 100,0 );
        legacyPolyLine.CloseLastContour();

        // Add the non-convex pentagon hole
        legacyPolyLine.Start( 1, 10, 10, CPolyLine::NO_HATCH );
        legacyPolyLine.AppendCorner( 10,20 );
        legacyPolyLine.AppendCorner( 15,15 );
        legacyPolyLine.AppendCorner( 20,20 );
        legacyPolyLine.AppendCorner( 20,10 );
        legacyPolyLine.CloseLastContour();

        // Add the triangle hole
        legacyPolyLine.Start( 1, 40, 10, CPolyLine::NO_HATCH );
        legacyPolyLine.AppendCorner( 40,20 );
        legacyPolyLine.AppendCorner( 60,10 );
        legacyPolyLine.CloseLastContour();
    }

    ~ChamferFilletFixture(){}
};

/**
 * Fixture for the Collision test suite. It contains a copy of the common polygon set and two
 * vector containing colliding and non-colliding points.
 */
 struct CollisionFixture {
     // Polygon set common for all the tests
     SHAPE_POLY_SET polySet;

     // Vectors containing colliding and non-colliding points
     std::vector<VECTOR2I> collidingPoints, nonCollidingPoints;

     CollisionFixture()
     {
         // Get a copy of the polySet
         polySet = CommonTestData().polySet;

         // CREATE POINTS NOT COLLIDING WITH THE POLY SET

         // Inside the polygon
         collidingPoints.push_back( VECTOR2I( 10,90 ) );

         // Inside the polygon, but on a reentrant angle of a hole
         collidingPoints.push_back( VECTOR2I( 15,16 ) );

         // On a hole edge => inside the polygon
         collidingPoints.push_back( VECTOR2I( 40,25 ) );

         // On the outline edge => inside the polygon
         collidingPoints.push_back( VECTOR2I( 0,10 ) );

         // CREATE POINTS COLLIDING WITH THE POLY SET

         // Completely outside of the polygon
         nonCollidingPoints.push_back( VECTOR2I( 200,200 ) );

         // Inside the outline and inside a hole => outside the polygon
         nonCollidingPoints.push_back( VECTOR2I( 15,12 ) );
     }

     ~CollisionFixture(){}
 };

#endif //__FIXTURES_H
