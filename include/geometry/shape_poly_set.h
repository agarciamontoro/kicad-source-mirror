/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SHAPE_POLY_SET_H
#define __SHAPE_POLY_SET_H

#include <vector>
#include <cstdio>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "clipper.hpp"


/**
 * Class SHAPE_POLY_SET
 *
 * Represents a set of closed polygons. Polygons may be nonconvex, self-intersecting
 * and have holes. Provides boolean operations (using Clipper library as the backend).
 *
 * TODO: add convex partitioning & spatial index
 */
class SHAPE_POLY_SET : public SHAPE
{
    public:
        ///> represents a single polygon outline with holes. The first entry is the outline,
        ///> the remaining (if any), are the holes
        typedef std::vector<SHAPE_LINE_CHAIN> POLYGON;

        /**
         * Struct VERTEX_INDEX
         * Holds the necessary information to index a vertex on a SHAPE_POLY_SET object:
         * the outline index, the hole index relative to the outline and the vertex index relative
         * the hole.
         */
        typedef struct{
            int m_polygon;   /*!< aPolygon is the index of the polygon. */
            int m_contour;      /*!< aContour is the index of the contour. */
            int m_vertex;    /*!< aVertex is the index of the vertex. */
        } VERTEX_INDEX;

        /**
         * Class ITERATOR_TEMPLATE
         *
         * Base class for iterating over all vertices in a given SHAPE_POLY_SET.
         *
         * Let us define the terms used on this class to clarify comments and methods names:
         *      - Polygon: each polygon in the set.
         *      - Outline: first polyline in each polygon; represents its outer contour.
         *      - Hole: second and following polylines in the polygon.
         *      - Contour: each polyline of each polygon in the set, whether or not it is an
         *      outline or a hole.
         *      - Vertex (or corner): each one of the points that define a contour.
         */
        template <class T>
        class ITERATOR_TEMPLATE
        {
        public:

            /**
             * Function IsEndContour.
             * @return True if the current vertex is the last one of the current contour (outline or hole).
             */
            bool IsEndContour() const
            {
                return m_currentVertex + 1 == m_poly->CPolygon( m_currentPolygon )[m_currentContour].PointCount();
            }

            /**
             * Function IsLastOutline.
             * @return True if the current outline is the last one.
             */
            bool IsLastPolygon() const
            {
                return m_currentPolygon == m_lastPolygon;
            }

            operator bool() const
            {
                return m_currentPolygon <= m_lastPolygon;
            }

            /**
             * Function Advance.
             *
             * Advance the indices of the current vertex/outline/contour, checking whether the
             * vertices in the holes have to be iterated through
             */
            void Advance()
            {
                // Advance vertex index
                m_currentVertex ++;

                // Check whether the user wants to iterate through the vertices of the holes
                // and behave accordingly
                if( m_iterateHoles )
                {
                    // If the last vertex of the contour was reached, advance the contour index
                    if( m_currentVertex >= m_poly->CPolygon( m_currentPolygon )[m_currentContour].PointCount() )
                    {
                        m_currentVertex = 0;
                        m_currentContour++;

                        // If the last contour of the current polygon was reached, advance the
                        // outline index
                        int totalContours = m_poly->CPolygon( m_currentPolygon ).size();

                        if( m_currentContour >= totalContours )
                        {
                            m_currentContour = 0;
                            m_currentPolygon++;
                        }
                    }
                }
                else
                {
                    // If the last vertex of the outline was reached, advance to the following polygon
                    if( m_currentVertex >= m_poly->CPolygon( m_currentPolygon )[0].PointCount() )
                    {
                        m_currentVertex = 0;
                        m_currentPolygon++;
                    }
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
                return m_poly->Polygon(m_currentPolygon)[m_currentContour].Point(m_currentVertex);
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
            friend class SHAPE_POLY_SET;

            SHAPE_POLY_SET* m_poly;
            int m_currentPolygon;
            int m_currentContour;
            int m_currentVertex;
            int m_lastPolygon;
            bool m_iterateHoles;
        };

        typedef ITERATOR_TEMPLATE<VECTOR2I> ITERATOR;
        typedef ITERATOR_TEMPLATE<const VECTOR2I> CONST_ITERATOR;

        SHAPE_POLY_SET();

        SHAPE_POLY_SET( const POLYGON& aPolygon );


        /**
         * Copy constructor SHAPE_POLY_SET
         * Performs a deep copy of \p aOther into \p this.
         * @param aOther is the SHAPE_POLY_SET object that will be copied.
         */
        SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther );

        ~SHAPE_POLY_SET();

        /**
         * Function GetRelativeIndices
         *
         * Converts a global vertex index ---i.e., a number that globally identifies a vertex in a
         * concatenated list of all vertices in all contours--- and get the index of the vertex
         * relative to the contour relative to the polygon in which it is.
         * @param  aGlobalIdx  is the global index of the corner whose structured index wants to
         *                     be found
         * @param  aPolygonIdx is the index of the polygon in which the expected vertex is.
         * @param  aContourIdx is the index of the contour in the aPolygonIdx-th poylgon in which
         *                     the expected vertex is.
         * @param  aVertexIdx  is the index of the vertex in the aContourIdx-th contour in which
         *                     the expected vertex is.
         * @return             [description]
         */
        bool GetRelativeIndices( int aGlobalIdx, VERTEX_INDEX* aRelativeIndices);

        bool GetGlobalIndex( VERTEX_INDEX aRelativeIndices, int& aGlobalIdx );

        /// @copydoc SHAPE::Clone()
        SHAPE* Clone() const override;

        ///> Creates a new empty polygon in the set and returns its index
        int NewOutline();

        ///> Creates a new hole in a given outline
        int NewHole( int aOutline = -1 );

        ///> Adds a new outline to the set and returns its index
        int AddOutline( const SHAPE_LINE_CHAIN& aOutline );

        ///> Adds a new hole to the given outline (default: last) and returns its index
        int AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline = -1 );

        ///> Appends a vertex at the end of the given outline/hole (default: the last outline)
        int Append( int x, int y, int aOutline = -1, int aHole = -1 );

        ///> Merges polygons from two sets.
        void Append( const SHAPE_POLY_SET& aSet );

        ///> Appends a vertex at the end of the given outline/hole (default: the last outline)
        void Append( const VECTOR2I& aP, int aOutline = -1, int aHole = -1 );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        VECTOR2I& Vertex( int index, int aOutline = -1, int aHole = -1 );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        const VECTOR2I& CVertex( int index, int aOutline = -1, int aHole = -1 ) const;

        ///> Returns the index-th vertex in a given hole outline within a given outline
        VECTOR2I& Vertex( VERTEX_INDEX index );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        const VECTOR2I& CVertex(VERTEX_INDEX index ) const;

        ///> Returns true if any of the outlines is self-intersecting
        bool IsSelfIntersecting();

        ///> Returns the number of outlines in the set
        int OutlineCount() const { return m_polys.size(); }

        ///> Returns the number of vertices in a given outline/hole
        int VertexCount( int aOutline = -1, int aHole = -1 ) const;

        ///> Returns the number of holes in a given outline
        int HoleCount( int aOutline ) const
        {
            if( (aOutline > (int)m_polys.size()) || (m_polys[aOutline].size() < 2) )
                return 0;
            return m_polys[aOutline].size() - 1;
        }

        ///> Returns the reference to aIndex-th outline in the set
        SHAPE_LINE_CHAIN& Outline( int aIndex )
        {
            return m_polys[aIndex][0];
        }

        SHAPE_POLY_SET Subset( int aFirstPolygon, int aLastPolygon )
        {
            assert( aFirstPolygon >= 0 && aLastPolygon < OutlineCount() );

            SHAPE_POLY_SET newPolySet;

            for( int index = aFirstPolygon; index < aLastPolygon; index++ )
            {
                newPolySet.m_polys.push_back( Polygon( index ) );
            }

            return newPolySet;
        }

        SHAPE_POLY_SET UnitSet( int aPolygonIndex )
        {
            return Subset( aPolygonIndex, aPolygonIndex + 1 );
        }

        ///> Returns the reference to aHole-th hole in the aIndex-th outline
        SHAPE_LINE_CHAIN& Hole( int aOutline, int aHole )
        {
            return m_polys[aOutline][aHole + 1];
        }

        ///> Returns the aIndex-th subpolygon in the set
        POLYGON& Polygon( int aIndex )
        {
            return m_polys[aIndex];
        }

        const SHAPE_LINE_CHAIN& COutline( int aIndex ) const
        {
            return m_polys[aIndex][0];
        }

        const SHAPE_LINE_CHAIN& CHole( int aOutline, int aHole ) const
        {
            return m_polys[aOutline][aHole + 1];
        }

        const POLYGON& CPolygon( int aIndex ) const
        {
            return m_polys[aIndex];
        }

        ///> Returns an iterator object, for iterating between aFirst and aLast outline, with or
        /// without holes (default: without)
        ITERATOR Iterate( int aFirst, int aLast, bool aIterateHoles = false )
        {
            ITERATOR iter;

            iter.m_poly = this;
            iter.m_currentPolygon = aFirst;
            iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentVertex = 0;
            iter.m_iterateHoles = aIterateHoles;

            return iter;
        }

        ///> Returns an iterator object, for iterating aOutline-th outline
        ITERATOR Iterate( int aOutline )
        {
            return Iterate( aOutline, aOutline );
        }

        ///> Returns an iterator object, for iterating aOutline-th outline
        ITERATOR IterateWithHoles( int aOutline )
        {
            return Iterate( aOutline, aOutline, true );
        }

        ///> Returns an iterator object, for all outlines in the set (no holes)
        ITERATOR Iterate()
        {
            return Iterate( 0, OutlineCount() - 1 );
        }

        ///> Returns an iterator object, for all outlines in the set (with holes)
        ITERATOR IterateWithHoles()
        {
            return Iterate( 0, OutlineCount() - 1, true );
        }


        CONST_ITERATOR CIterate( int aFirst, int aLast, bool aIterateHoles = false ) const
        {
            CONST_ITERATOR iter;

            iter.m_poly = const_cast<SHAPE_POLY_SET*>( this );
            iter.m_currentPolygon = aFirst;
            iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentVertex = 0;
            iter.m_iterateHoles = aIterateHoles;

            return iter;
        }

        CONST_ITERATOR CIterate( int aOutline ) const
        {
            return CIterate( aOutline, aOutline );
        }

        CONST_ITERATOR CIterateWithHoles( int aOutline ) const
        {
            return CIterate( aOutline, aOutline, true );
        }

        CONST_ITERATOR CIterate() const
        {
            return CIterate( 0, OutlineCount() - 1 );
        }

        CONST_ITERATOR CIterateWithHoles() const
        {
            return CIterate( 0, OutlineCount() - 1, true );
        }

        /** operations on polygons use a aFastMode param
         * if aFastMode is PM_FAST (true) the result can be a weak polygon
         * if aFastMode is PM_STRICTLY_SIMPLE (false) (default) the result is (theorically) a strictly
         * simple polygon, but calculations can be really significantly time consuming
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
        void BooleanAdd( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset difference
        ///> For aFastMode meaning, see function booleanOp
        void BooleanSubtract( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset intersection
        ///> For aFastMode meaning, see function booleanOp
        void BooleanIntersection( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset union between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanAdd( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b,
                         POLYGON_MODE aFastMode );

        ///> Performs boolean polyset difference between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanSubtract( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b,
                              POLYGON_MODE aFastMode );

        ///> Performs boolean polyset intersection between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanIntersection( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b,
                                  POLYGON_MODE aFastMode );

        ///> Performs outline inflation/deflation, using round corners.
        void Inflate( int aFactor, int aCircleSegmentsCount );

        ///> Converts a set of polygons with holes to a singe outline with "slits"/"fractures" connecting the outer ring
        ///> to the inner holes
        ///> For aFastMode meaning, see function booleanOp
        void Fracture( POLYGON_MODE aFastMode );

        ///> Converts a set of slitted polygons to a set of polygons with holes
        void Unfracture();

        ///> Returns true if the polygon set has any holes.
        bool HasHoles() const;

        ///> Simplifies the polyset (merges overlapping polys, eliminates degeneracy/self-intersections)
        ///> For aFastMode meaning, see function booleanOp
        void Simplify( POLYGON_MODE aFastMode );

        /**
         * Function NormalizeAreaOutlines
         * Convert a self-intersecting polygon to one (or more) non self-intersecting polygon(s)
         * Removes null segments.
         * @return the polygon count (always >= 1, because there is at least one polygon)
         * There are new polygons only if the polygon count  is > 1
         */
        int NormalizeAreaOutlines();

        /// @copydoc SHAPE::Format()
        const std::string Format() const override;

        /// @copydoc SHAPE::Parse()
        bool Parse( std::stringstream& aStream ) override;

        /// @copydoc SHAPE::Move()
        void Move( const VECTOR2I& aVector ) override;

        /// @copydoc SHAPE::IsSolid()
        bool IsSolid() const override
        {
            return true;
        }

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
         * @param  aP         is the VECTOR2I point whose collision with respect to the poly set
         *                    will be tested.
         * @param  aClearance is the security distance; if the point lies closer to the polygon
         *                    than aClearance distance, then there is a collision.
         * @return true if the point aP collides with the polygon; false in any other case.
         */
        bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const override;

        // fixme: add collision support
        bool Collide( const SEG& aSeg, int aClearance = 0 ) const override { return false; }

        /**
         * Function CollideVertex
         * Checks whether aPoint collides with any vertex of any of the contours of the polygon.
         * @param  aPoint     is the VECTOR2I point whose collision with respect to the polygon
         *                    will be tested
         * @param  aClearance is the security distance; if \p aPoint lies closer to a vertex than
         *                    aClearance distance, then there is a collision.
         * @param aClosestVertex is the index of the closes vertex to \p aPoint.
         * @return bool - true if there is a collision, false in any other case.
         */
        bool CollideVertex( const VECTOR2I& aPoint, VERTEX_INDEX& aClosestVertex,
                int aClearance = 0 );

        /**
         * Function CollideEdge
         * Checks whether aPoint collides with any edge of any of the contours of the polygon.
         * @param  aPoint     is the VECTOR2I point whose collision with respect to the polygon
         *                    will be tested.
         * @param  aClearance is the security distance; if \p aPoint lies closer to a vertex than
         *                    aClearance distance, then there is a collision.
         * @param aClosestVertex is the index of the closes vertex to \p aPoint.
         * @return bool - true if there is a collision, false in any other case.
         */
        bool CollideEdge( const VECTOR2I& aPoint, VERTEX_INDEX& aClosestVertex,
                int aClearance = 0 );

        ///> Returns true if a given subpolygon contains the point aP. If aSubpolyIndex < 0 (default value),
        ///> checks all polygons in the set
        bool Contains( const VECTOR2I& aP, int aSubpolyIndex = -1 ) const;

        ///> Returns true if the set is empty (no polygons at all)
        bool IsEmpty() const
        {
            return m_polys.size() == 0;
        }

        ///> Removes all outlines & holes (clears) the polygon set.
        void RemoveAllContours();

        ///> Returns total number of vertices stored in the set.
        int TotalVertices() const;

        ///> Deletes aIdx-th polygon from the set
        void DeletePolygon( int aIdx );

        /**
         * Function Chamfer
         * Returns a chamfered version of the aIndex-th polygon.
         * @param aDistance is the chamfering distance.
         * @param aIndex is the index of the polygon to be chamfered.
         * @return A POLYGON object containing the chamfered version of the aIndex-th polygon.
         */
        POLYGON ChamferPolygon( unsigned int aDistance, int aIndex = 0 );

        /**
         * Function Fillet
         * Returns a filleted version of the aIndex-th polygon.
         * @param aRadius is the fillet radius.
         * @param aSegments is the number of segments / fillet.
         * @param aIndex is the index of the polygon to be filleted
         * @return A POLYGON object containing the filleted version of the aIndex-th polygon.
         */
        POLYGON FilletPolygon( unsigned int aRadius, unsigned int aSegments, int aIndex = 0 );

    private:

        SHAPE_LINE_CHAIN& getContourForCorner( int aCornerId, int& aIndexWithinContour );
        VECTOR2I& vertex( int aCornerId );
        const VECTOR2I& cvertex( int aCornerId ) const;


        void fractureSingle( POLYGON& paths );
        void importTree( ClipperLib::PolyTree* tree );

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
                        const SHAPE_POLY_SET& aOtherShape, POLYGON_MODE aFastMode );

        void booleanOp( ClipperLib::ClipType aType,
                        const SHAPE_POLY_SET& aShape,
                        const SHAPE_POLY_SET& aOtherShape, POLYGON_MODE aFastMode );

        bool pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const;

        const ClipperLib::Path convertToClipper( const SHAPE_LINE_CHAIN& aPath, bool aRequiredOrientation );
        const SHAPE_LINE_CHAIN convertFromClipper( const ClipperLib::Path& aPath );

        /**
         * containsSingle function
         * Checks whether the point aP is inside the aSubpolyIndex-th polygon of the polyset. If
         * the points lies on an edge, the polygon is considered to contain it.
         * @param  aP            is the VECTOR2I point whose position with respect to the inside of
         *                       the aSubpolyIndex-th polygon will be tested.
         * @param  aSubpolyIndex is an integer specifying which polygon in the set has to be
         *                       checked.
         * @return bool - true if aP is inside aSubpolyIndex-th polygon; false in any other
         *         case.
         */
        bool containsSingle( const VECTOR2I& aP, int aSubpolyIndex ) const;

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
         * @return POLYGON - the chamfered/filleted version of the polygon.
         */
        POLYGON chamferFilletPolygon( CORNER_MODE aMode, unsigned int aDistance,
                                      int aIndex, int aSegments = -1 );

        typedef std::vector<POLYGON> Polyset;

        Polyset m_polys;
};

#endif
