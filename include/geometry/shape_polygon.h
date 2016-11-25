/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SHAPE_POLYGON_H
#define __SHAPE_POLYGON_H

#include <vector>
#include <cstdio>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "clipper.hpp"


/**
 * Class SHAPE_POLYGON
 *
 * Represents a set of closed polygons. Polygons may be nonconvex, self-intersecting
 * and have holes. Provides boolean operations (using Clipper library as the backend).
 *
 * TODO: add convex partitioning & spatial index
 */
class SHAPE_POLYGON : public SHAPE
{
    public:
        ///> represents a single polygon outline with holes. The first entry is the outline,
        ///> the remaining (if any), are the holes
        typedef std::vector<SHAPE_LINE_CHAIN> POLYGON;

        /**
         * Class ITERATOR_TEMPLATE
         *
         * Base class for iterating over all vertices in a given SHAPE_POLYGON
         */
        template <class T>
        class ITERATOR_TEMPLATE
        {
        public:

            /**
             * Function IsEndContour
             * Checks  whether the current vertex is the last one of the current contour.
             * @return true if the current vertex is the last one of the current contour (outline
             * or hole), false in any other case
             */
            bool IsEndContour() const
            {
                return m_currentVertex + 1 == m_poly->CContour( m_currentContour ).PointCount();
            }

            /**
             * Function IsHole
             * Checks whether the current contour is a hole.
             * @return true if the current contour is a hole, false in any other case.
             */
            bool IsHole() const
            {
                return m_currentContour > 0;
            }

            operator bool() const
            {
                return m_currentContour <= m_lastContour;
            }

            void Advance()
            {
                m_currentVertex ++;
                if( m_currentVertex >= m_poly->CContour( m_currentContour ).PointCount() )
                {
                    m_currentVertex = 0;
                    m_currentContour++;
                }
            }

            void operator++( int dummy )
            {
                Advance();
            }

            void operator++()
            {
                Advance();
            }

            T& Get()
            {
                return m_poly->CContour( m_currentContour ).CPoint( m_currentVertex );
            }

            T& operator*()
            {
                return Get();
            }

            T* operator->()
            {
                return &Get();
            }


        private:
            friend class SHAPE_POLYGON;

            SHAPE_POLYGON* m_poly;
            int m_currentContour;
            int m_lastContour;
            int m_currentVertex;
        };

        typedef ITERATOR_TEMPLATE<VECTOR2I> ITERATOR;
        typedef ITERATOR_TEMPLATE<const VECTOR2I> CONST_ITERATOR;

        SHAPE_POLYGON();
        ~SHAPE_POLYGON();

        ///> Creates a new hole

        /**
         * Function NewHole
         * Creates a new hole on the polygon.
         * @return int - the index of the new hole.
         */
        int NewHole();

        /**
         * Function AddHole
         * Adds \p aHole to the polygon.
         * @return int - the index of the new hole.
         */
        int AddHole( const SHAPE_LINE_CHAIN& aHole );

        /**
         * Function Append
         * @param  x        is the x coordinate of the new vertex
         * @param  y        is the y coordinate of the new vertex
         * @param  aContour is the contour in which the vertex will be added; defaults to the last
         *                  hole.
         * @return          int - the number of vertices the contour modified has.
         */
        int Append( int x, int y, int aContour = -1 );

        /**
         * Function Append
         * @param  aP       is the position of the new vertex.
         * @param  aContour is the contour in which the vertex will be added; defaults to the last
         *                  hole.
         * @return          int - the number of vertices the contour modified has.
         */
        int Append( const VECTOR2I& aP, int aContour = -1 );

        ///> Returns the index-th vertex in a given contour

        /**
         * Function Vertex
         * Retrieves the \p aIdx-th vertex of the \p aContour-th contour.
         * @param  aVertexIdx is the index of the vertex on \p aContour to be retrieved.
         * @param  aContour   is the contour fom which the vertex will be retrieved.
         * @return VECTOR2I - a reference to the vertex.
         */
        VECTOR2I& Vertex( int aVertexIdx, int aContour = -1 );

        /**
         * Function CVertex
         * Constant version of SHAPE_POLYGON::Vertex()
         * @copydetails
         */
        const VECTOR2I& CVertex( int aVertexIdx, int aContour = -1 ) const;

        /**
         * Function ContourCount description
         * @return int - the number of contours in the polygon; i.e., the outline plus all the
         * holes.
         */
        int ContourCount() const;

        /**
         * Function VertexCount
         * @param  aContour is the index of the contour whose vertices will be counted; defaults
         * to the last hole.
         * @return          int - the number of vertices of the aContour-th contour.
         */
        int VertexCount( int aContour = -1 ) const;

        /**
         * Function HoleCount
         * @return int - the number of holes the polygon has.
         */
        int HoleCount() const;

        /**
         * Function Contour
         * @param  aIndex is the index of the expected contour (0 for the contour, >0 for
         * the holes)
         * @return SHAPE_LINE_CHAIN - a reference to the aIndex-th contour.
         */
        SHAPE_LINE_CHAIN& Contour( int aIndex );

        /**
         * Function Outline.
         * @return SHAPE_LINE_CHAIN - a reference to the outline, namely the 0th contour.
         */
        SHAPE_LINE_CHAIN& Outline();

        /**
         * Function Hole.
         * @param  aHole is the index of the hole, starting at 0-
         * @return SHAPE_LINE_CHAIN - a reference to the aHole-th hole, namely the (aHole+1)th
         * contour.
         */
        SHAPE_LINE_CHAIN& Hole( int aHole );

        /**
         * Function CContour
         * Constant version of SHAPE_POLYGON::Contour()
         * @copydetails
         */
        const SHAPE_LINE_CHAIN& CContour( int aIndex ) const;

        /**
         * Function COutline
         * Constant version of SHAPE_POLYGON::Outline()
         * @copydetails
         */
        const SHAPE_LINE_CHAIN& COutline() const;

        /**
         * Function CHole
         * Constant version of SHAPE_POLYGON::Hole()
         * @copydetails
         */
        const SHAPE_LINE_CHAIN& CHole( int aHole ) const;

        /**
         * Function Iterate
         * Create and returns an iterator object to iterate through the vertices of the contours
         * between \p aFirst and \p aLast.
         * @param  aFirst is the index of the first contour whose vertices will be iterated.
         * @param  aLast  is the index of the last contour whose vertices will be iterated.
         * @return        ITERATOR - the vertex iterator.
         */
        ITERATOR Iterate( int aFirst, int aLast );

        /**
         * Function IterateOutline description
         * Creates and return an iterator object to iterate through all the vertices of the
         * outline contour.
         * @return ITERATOR - the vertex iterator.
         */
        ITERATOR IterateOutline();

        /**
         * Function IterateOutline description
         * Creates and return an iterator object to iterate through all the vertices of the
         * outline contour and its holes.
         * @return ITERATOR - the vertex iterator.
         */
        ITERATOR IterateOutlineWithHoles();

        /**
         * Function CIterate
         * Constant version of ITERATOR::Iterate()
         * @copydetails
         */
        CONST_ITERATOR CIterate( int aFirst, int aLast ) const;

        /**
         * Function CIterateOutline
         * Constant version of ITERATOR::IterateOutline()
         * @copydetails
         */
        CONST_ITERATOR CIterateOutline() const;

        /**
         * Function CIterateOutlineWithHoles
         * Constant version of ITERATOR::IterateOutlineWithHoles()
         * @copydetails
         */
        CONST_ITERATOR CIterateOutlineWithHoles() const;

        /** operations on polygons use a aFastMode param
         * if aFastMode is PM_FAST (true) the result can be a weak polygon
         * if aFastMode is PM_STRICTLY_SIMPLE (false) (default) the result is (theorically) a
         * strictly simple polygon, but calculations can be really significantly time consuming
         * Most of time PM_FAST is preferable.
         * PM_STRICTLY_SIMPLE can be used in critical cases (Gerber output for instance)
         */
        enum POLYGON_MODE
        {
            PM_FAST = true,
            PM_STRICTLY_SIMPLE = false
        };

        ///> Performs boolean polyset union
        ///> For aFastMode meaning, see function booleanOp
        void BooleanAdd( const SHAPE_POLYGON& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset difference
        ///> For aFastMode meaning, see function booleanOp
        void BooleanSubtract( const SHAPE_POLYGON& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset intersection
        ///> For aFastMode meaning, see function booleanOp
        void BooleanIntersection( const SHAPE_POLYGON& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset union between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanAdd( const SHAPE_POLYGON& a, const SHAPE_POLYGON& b,
                         POLYGON_MODE aFastMode );

        ///> Performs boolean polyset difference between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanSubtract( const SHAPE_POLYGON& a, const SHAPE_POLYGON& b,
                              POLYGON_MODE aFastMode );

        ///> Performs boolean polyset intersection between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanIntersection( const SHAPE_POLYGON& a, const SHAPE_POLYGON& b,
                                  POLYGON_MODE aFastMode );

        ///> Performs outline inflation/deflation, using round corners.
        void Inflate( int aFactor, int aCircleSegmentsCount );

        ///> Converts the polygon with holes to a singe outline with "slits"/"fractures"
        ///> connecting the outer ring to the inner holes.
        ///> For aFastMode meaning, see function booleanOp
        void Fracture( POLYGON_MODE aFastMode );

        ///> Returns true if the polygon has any holes.
        bool HasHoles() const;

        ///> Simplifies the polygon (eliminates degeneracy/self-intersections)
        ///> For aFastMode meaning, see function booleanOp
        void Simplify( POLYGON_MODE aFastMode );

        /// @copydoc SHAPE::Format()
        const std::string Format() const override;

        /// @copydoc SHAPE::Parse()
        bool Parse( std::stringstream& aStream ) override;

        /// @copydoc SHAPE::Move()
        void Move( const VECTOR2I& aVector ) override;

        /// @copydoc SHAPE::IsSolid()
        bool IsSolid() const override;

        /// @copydoc SHAPE::BBox()
        const BOX2I BBox( int aClearance = 0 ) const override;

        /**
         * Function PointOnEdge()
         *
         * Checks if point aP lies on an edge or vertex of some of the outlines or holes.
         * @param aP is the point to check
         * @return true if the point lies on the edge of any polygon.
         */
        bool PointOnEdge( const VECTOR2I& aP ) const;

        /**
         * Function Collide
         * Checks whether the point aP collides with the inside of the polygon set; if the point
         * lies on an edge or on a corner of any of the polygons, there is no collision: the edges
         * does not belong to the polygon itself.
         * @param  aP         is the point whose collision with respect to the polygon will be
         *                    tested.
         * @param  aClearance is the security distance; if the point lies closer to the polygon
         *                    than aClearance distance, then there is a collision.
         * @return true if the point aP collides with the polygon; false in any other case.
         */
        bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const override;

        // fixme: add collision support
        bool Collide( const SEG& aSeg, int aClearance = 0 ) const override;

        /**
         * Function Contains
         * @param  aP            is the point that wants to be checked.
         * @return               true if \p aP is inside the polygon and false in any other case.
         */
        bool Contains( const VECTOR2I& aP ) const;

        ///> Removes the outline & holes. Clears the polygon.
        void RemoveAllContours();

        ///> Returns total number of vertices stored in the set: vertices of the outline +
        /// vertices of the holes
        int TotalVertices() const;

        /**
         * Function Chamfer
         * Returns a chamfered version of the polygon.
         * @param aDistance is the chamfering distance.
         * @return A SHAPE_POLYGON object containing the chamfered version of the polygon.
         */
        SHAPE_POLYGON Chamfer( unsigned int aDistance );

        /**
         * Function Fillet
         * Returns a filleted version of the aIndex-th polygon.
         * @param aRadius is the fillet radius.
         * @param aSegments is the number of segments / fillet.
         * @return A SHAPE_POLYGON object containing the filleted version of the aIndex-th polygon.
         */
        SHAPE_POLYGON Fillet( unsigned int aRadius, unsigned int aSegments );

    private:
        /**
         * Function importNode
         * Imports a Clipper node into this polygon
         * @param node is a PolyNode object from Clipper defining a polygon with holes.
         */
         void importNode( ClipperLib::PolyNode* node );

        /** Function booleanOp
         * this is the engine to execute all polygon boolean transforms
         * (AND, OR, ... and polygon simplification (merging overlaping  polygons)
         * @param aType is the transform type ( see ClipperLib::ClipType )
         * @param aOtherShape is the SHAPE_LINE_CHAIN to combine with me.
         * @param aFastMode is an option to choose if the result can be a weak polygon
         * or a stricty simple polygon.
         * if aFastMode is PM_FAST the result can be a weak polygon
         * if aFastMode is PM_STRICTLY_SIMPLE (default) the result is (theorically) a strictly
         * simple polygon, but calculations can be really significantly time consuming
         */
        void booleanOp( ClipperLib::ClipType aType,
                        const SHAPE_POLYGON& aOtherShape, POLYGON_MODE aFastMode );

        void booleanOp( ClipperLib::ClipType aType,
                        const SHAPE_POLYGON& aShape,
                        const SHAPE_POLYGON& aOtherShape, POLYGON_MODE aFastMode );

        bool pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const;

        const ClipperLib::Path convertToClipper( const SHAPE_LINE_CHAIN& aPath,
                                                 bool aRequiredOrientation );
        const SHAPE_LINE_CHAIN convertFromClipper( const ClipperLib::Path& aPath );

        /**
         * Operations ChamferPolygon and FilletPolygon are computed under the private chamferFillet
         * method; this enum is defined to make the necessary distinction when calling this method
         * from the public ChamferPolygon and FilletPolygon methods.
         */
        enum CORNER_MODE
        {
            CHAMFERED,
            FILLETED
        };

        /**
         * Function chamferFilletPolygon
         * Returns the camfered or filleted version of the aIndex-th polygon in the set, depending
         * on the aMode selected
         * @param  aMode     represent which action will be taken: CORNER_MODE::CHAMFERED will
         *                   return a chamfered version of the polygon, CORNER_MODE::FILLETED will
         *                   return a filleted version of the polygon.
         * @param  aDistance is the chamfering distance if aMode = CHAMFERED; if aMode = FILLETED,
         *                   is the filleting radius.
         * @param  aIndex    is the index of the polygon that will be chamfered/filleted.
         * @param  aSegments is the number of filleting segments if aMode = FILLETED. If aMode =
         *                   CHAMFERED, it is unused.
         * @return SHAPE_POLYGON - the chamfered/filleted version of the polygon.
         */
        SHAPE_POLYGON chamferFillet( CORNER_MODE aMode, unsigned int aDistance,
                                     int aSegments = -1 );

        POLYGON m_contours;
};

#endif
