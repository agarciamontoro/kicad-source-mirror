/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Point in polygon algorithm adapted from Clipper Library (C) Angus Johnson,
 * subject to Clipper library license.
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


#include <vector>
#include <cstdio>
#include <set>
#include <list>
#include <algorithm>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <common.h>     // KiROUND

using namespace ClipperLib;

SHAPE_POLY_SET::SHAPE_POLY_SET() :
    SHAPE( SH_POLY_SET )
{

}


SHAPE_POLY_SET::~SHAPE_POLY_SET()
{
}


int SHAPE_POLY_SET::NewOutline()
{
    SHAPE_LINE_CHAIN empty_path;
    POLYGON poly;
    empty_path.SetClosed( true );
    poly.push_back( empty_path );
    m_polys.push_back( poly );
    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::NewHole( int aOutline )
{
    SHAPE_LINE_CHAIN empty_path;
    empty_path.SetClosed( true );

    m_polys.back().push_back( empty_path );

    return m_polys.back().size() - 2;
}


int SHAPE_POLY_SET::Append( int x, int y, int aOutline, int aHole )
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int)m_polys.size() );
    assert( idx < (int)m_polys[aOutline].size() );

    m_polys[aOutline][idx].Append( x, y );

    return m_polys[aOutline][idx].PointCount();
}


int SHAPE_POLY_SET::VertexCount( int aOutline , int aHole  ) const
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert ( aOutline < (int)m_polys.size() );
    assert ( idx < (int)m_polys[aOutline].size() );

    return m_polys[aOutline][idx].PointCount();
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( int index, int aOutline , int aHole ) const
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int)m_polys.size() );
    assert( idx < (int)m_polys[aOutline].size() );

    return m_polys[aOutline][idx].CPoint( index );
}


VECTOR2I& SHAPE_POLY_SET::Vertex( int index, int aOutline , int aHole )
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int)m_polys.size() );
    assert( idx < (int)m_polys[aOutline].size() );

    return m_polys[aOutline][idx].Point( index );
}


int SHAPE_POLY_SET::AddOutline( const SHAPE_LINE_CHAIN& aOutline )
{
    assert( aOutline.IsClosed() );

    POLYGON poly;

    poly.push_back( aOutline );

    m_polys.push_back( poly );

    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline )
{
    assert ( m_polys.size() );

    if( aOutline < 0 )
        aOutline += m_polys.size();

    POLYGON& poly = m_polys[aOutline];

    assert( poly.size() );

    poly.push_back( aHole );

    return poly.size() - 1;
}


const Path SHAPE_POLY_SET::convertToClipper( const SHAPE_LINE_CHAIN& aPath, bool aRequiredOrientation )
{
    Path c_path;

    for( int i = 0; i < aPath.PointCount(); i++ )
    {
        const VECTOR2I& vertex = aPath.CPoint( i );
        c_path.push_back( IntPoint( vertex.x, vertex.y ) );
    }

    if( Orientation( c_path ) != aRequiredOrientation )
        ReversePath( c_path );

    return c_path;
}


const SHAPE_LINE_CHAIN SHAPE_POLY_SET::convertFromClipper( const Path& aPath )
{
    SHAPE_LINE_CHAIN lc;

    for( unsigned int i = 0; i < aPath.size(); i++ )
        lc.Append( aPath[i].X, aPath[i].Y );

    return lc;
}

void SHAPE_POLY_SET::booleanOp( ClipType aType, const SHAPE_POLY_SET& aOtherShape,
                                POLYGON_MODE aFastMode )
{
    Clipper c;

    if( aFastMode == PM_STRICTLY_SIMPLE )
        c.StrictlySimple( true );

    for( const POLYGON& poly : m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptSubject, true );
    }

    for( const POLYGON& poly : aOtherShape.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptClip, true );
    }

    PolyTree solution;

    c.Execute( aType, solution, pftNonZero, pftNonZero );

    importTree( &solution );
}


void SHAPE_POLY_SET::booleanOp( ClipperLib::ClipType aType,
                                const SHAPE_POLY_SET& aShape,
                                const SHAPE_POLY_SET& aOtherShape,
                                POLYGON_MODE aFastMode )
{
    Clipper c;

    if( aFastMode == PM_STRICTLY_SIMPLE )
        c.StrictlySimple( true );

    for( const POLYGON& poly : aShape.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptSubject, true );
    }

    for( const POLYGON& poly : aOtherShape.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptClip, true );
    }

    PolyTree solution;

    c.Execute( aType, solution, pftNonZero, pftNonZero );

    importTree( &solution );
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctUnion, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctDifference, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanIntersection( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctIntersection, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctUnion, a, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctDifference, a, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanIntersection( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctIntersection, a, b, aFastMode );
}


void SHAPE_POLY_SET::Inflate( int aFactor, int aCircleSegmentsCount )
{
    // A static table to avoid repetitive calculations of the coefficient
    // 1.0 - cos( M_PI/aCircleSegmentsCount)
    // aCircleSegmentsCount is most of time <= 64 and usually 8, 12, 16, 32
    #define SEG_CNT_MAX 64
    static double arc_tolerance_factor[SEG_CNT_MAX+1];

    ClipperOffset c;

    for( const POLYGON& poly : m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), jtRound, etClosedPolygon );
    }

    PolyTree solution;

    // Calculate the arc tolerance (arc error) from the seg count by circle.
    // the seg count is nn = M_PI / acos(1.0 - c.ArcTolerance / abs(aFactor))
    // see:
    // www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/ClipperOffset/Properties/ArcTolerance.htm

    if( aCircleSegmentsCount < 6 )  // avoid incorrect aCircleSegmentsCount values
        aCircleSegmentsCount = 6;

    double coeff;

    if( aCircleSegmentsCount > SEG_CNT_MAX || arc_tolerance_factor[aCircleSegmentsCount] == 0 )
    {
        coeff = 1.0 - cos( M_PI/aCircleSegmentsCount);

        if( aCircleSegmentsCount <= SEG_CNT_MAX )
            arc_tolerance_factor[aCircleSegmentsCount] = coeff;
    }
    else
        coeff = arc_tolerance_factor[aCircleSegmentsCount];

    c.ArcTolerance = std::abs( aFactor ) * coeff;

    c.Execute( solution, aFactor );

    importTree( &solution );
}


void SHAPE_POLY_SET::importTree( PolyTree* tree )
{
    m_polys.clear();

    for( PolyNode* n = tree->GetFirst(); n; n = n->GetNext() )
    {
        if( !n->IsHole() )
        {
            POLYGON paths;
            paths.reserve( n->Childs.size() + 1 );
            paths.push_back( convertFromClipper( n->Contour ) );

            for( unsigned int i = 0; i < n->Childs.size(); i++ )
                paths.push_back( convertFromClipper( n->Childs[i]->Contour ) );

            m_polys.push_back(paths);
        }
    }
}

// Polygon fracturing code. Work in progress.

struct FractureEdge
{
    FractureEdge( bool connected, SHAPE_LINE_CHAIN* owner, int index ) :
        m_connected( connected ),
        m_next( NULL )
    {
        m_p1 = owner->CPoint( index );
        m_p2 = owner->CPoint( index + 1 );
    }

    FractureEdge( int y = 0 ) :
        m_connected( false ),
        m_next( NULL )
    {
        m_p1.x = m_p2.y = y;
    }

    FractureEdge( bool connected, const VECTOR2I& p1, const VECTOR2I& p2 ) :
        m_connected( connected ),
        m_p1( p1 ),
        m_p2( p2 ),
        m_next( NULL )
    {
    }

    bool matches( int y ) const
    {
        int y_min = std::min( m_p1.y, m_p2.y );
        int y_max = std::max( m_p1.y, m_p2.y );

        return ( y >= y_min ) && ( y <= y_max );
    }

    bool m_connected;
    VECTOR2I m_p1, m_p2;
    FractureEdge* m_next;
};


typedef std::vector<FractureEdge*> FractureEdgeSet;

static int processEdge( FractureEdgeSet& edges, FractureEdge* edge )
{
    int x = edge->m_p1.x;
    int y = edge->m_p1.y;
    int min_dist = std::numeric_limits<int>::max();
    int x_nearest = 0;

    FractureEdge* e_nearest = NULL;

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
    {
        if( !(*i)->matches( y ) )
            continue;

        int x_intersect;

        if( (*i)->m_p1.y == (*i)->m_p2.y ) // horizontal edge
            x_intersect = std::max ( (*i)->m_p1.x, (*i)->m_p2.x );
        else
            x_intersect = (*i)->m_p1.x + rescale((*i)->m_p2.x - (*i)->m_p1.x,   y - (*i)->m_p1.y,   (*i)->m_p2.y - (*i)->m_p1.y );

        int dist = ( x - x_intersect );

        if( dist >= 0 && dist < min_dist && (*i)->m_connected )
        {
            min_dist = dist;
            x_nearest = x_intersect;
            e_nearest = (*i);
        }
    }

    if( e_nearest && e_nearest->m_connected )
    {
        int count = 0;

		FractureEdge* lead1 = new FractureEdge( true, VECTOR2I( x_nearest, y ), VECTOR2I( x, y ) );
        FractureEdge* lead2 = new FractureEdge( true, VECTOR2I( x, y ), VECTOR2I( x_nearest, y ) );
        FractureEdge* split_2 = new FractureEdge( true, VECTOR2I( x_nearest, y ), e_nearest->m_p2 );

        edges.push_back( split_2 );
        edges.push_back( lead1 );
        edges.push_back( lead2 );

        FractureEdge* link = e_nearest->m_next;

        e_nearest->m_p2 = VECTOR2I( x_nearest, y );
        e_nearest->m_next = lead1;
        lead1->m_next = edge;

        FractureEdge*last;
        for( last = edge; last->m_next != edge; last = last->m_next )
        {
            last->m_connected = true;
            count++;
        }

        last->m_connected = true;
        last->m_next = lead2;
        lead2->m_next = split_2;
        split_2->m_next = link;

        return count + 1;
    }

    return 0;
}

void SHAPE_POLY_SET::fractureSingle( POLYGON& paths )
{
    FractureEdgeSet edges;
    FractureEdgeSet border_edges;
    FractureEdge* root = NULL;

    bool first = true;

    if( paths.size() == 1 )
        return;

    int num_unconnected = 0;

    for( SHAPE_LINE_CHAIN& path : paths )
    {
        int index = 0;

        FractureEdge *prev = NULL, *first_edge = NULL;

        int x_min = std::numeric_limits<int>::max();

        for( int i = 0; i < path.PointCount(); i++ )
        {
            const VECTOR2I& p = path.CPoint( i );

            if( p.x < x_min )
                x_min = p.x;
        }

        for( int i = 0; i < path.PointCount(); i++ )
        {
            FractureEdge* fe = new FractureEdge( first, &path, index++ );

            if( !root )
                root = fe;

            if( !first_edge )
                first_edge = fe;

            if( prev )
                prev->m_next = fe;

            if( i == path.PointCount() - 1 )
                fe->m_next = first_edge;

            prev = fe;
            edges.push_back( fe );

            if( !first )
            {
                if( fe->m_p1.x == x_min )
                    border_edges.push_back( fe );
            }

            if( !fe->m_connected )
                num_unconnected++;
        }
        first = false; // first path is always the outline
    }

    // keep connecting holes to the main outline, until there's no holes left...
    while( num_unconnected > 0 )
    {
        int x_min = std::numeric_limits<int>::max();

        FractureEdge* smallestX = NULL;

        // find the left-most hole edge and merge with the outline
        for( FractureEdgeSet::iterator i = border_edges.begin(); i != border_edges.end(); ++i )
        {
            int xt = (*i)->m_p1.x;

            if( ( xt < x_min ) && ! (*i)->m_connected )
            {
                x_min = xt;
                smallestX = *i;
            }
        }

        num_unconnected -= processEdge( edges, smallestX );
    }

    paths.clear();
    SHAPE_LINE_CHAIN newPath;

    newPath.SetClosed( true );

    FractureEdge* e;

    for( e = root; e->m_next != root; e = e->m_next )
        newPath.Append( e->m_p1 );

    newPath.Append( e->m_p1 );

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
        delete *i;

    paths.push_back( newPath );
}


void SHAPE_POLY_SET::Fracture( POLYGON_MODE aFastMode )
{
    Simplify( aFastMode ); // remove overlapping holes/degeneracy

    for( POLYGON& paths : m_polys )
    {
        fractureSingle( paths );
    }
}


bool SHAPE_POLY_SET::HasHoles() const
{
    // Iterate through all the polygons on the set
    for( const POLYGON& paths : m_polys )
    {
        // If any of them has more than one contour, it is a hole.
        if( paths.size() > 1 )
            return true;
    }

    // Return false if and only if every polygon has just one outline, without holes.
    return false;
}

void SHAPE_POLY_SET::Simplify( POLYGON_MODE aFastMode )
{
    SHAPE_POLY_SET empty;

    booleanOp( ctUnion, empty, aFastMode );
}


const std::string SHAPE_POLY_SET::Format() const
{
    std::stringstream ss;

    ss << "polyset " << m_polys.size() << "\n";

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        ss << "poly " << m_polys[i].size() << "\n";
        for( unsigned j = 0; j < m_polys[i].size(); j++)
        {
            ss << m_polys[i][j].PointCount() << "\n";
            for( int v = 0; v < m_polys[i][j].PointCount(); v++)
                ss << m_polys[i][j].CPoint( v ).x << " " << m_polys[i][j].CPoint( v ).y << "\n";
        }
        ss << "\n";
    }

    return ss.str();
}


bool SHAPE_POLY_SET::Parse( std::stringstream& aStream )
{
    std::string tmp;

    aStream >> tmp;

    if( tmp != "polyset" )
        return false;

    aStream >> tmp;

    int n_polys = atoi( tmp.c_str() );

    if( n_polys < 0 )
        return false;

    for( int i = 0; i < n_polys; i++ )
    {
        POLYGON paths;

        aStream >> tmp;

        if( tmp != "poly" )
            return false;

        aStream >> tmp;
        int n_outlines = atoi( tmp.c_str() );

        if( n_outlines < 0 )
            return false;

        for( int j = 0; j < n_outlines; j++ )
        {
            SHAPE_LINE_CHAIN outline;

            outline.SetClosed( true );

            aStream >> tmp;
            int n_vertices = atoi( tmp.c_str() );
            for( int v = 0; v < n_vertices; v++ )
            {
                VECTOR2I p;

                aStream >> tmp; p.x = atoi( tmp.c_str() );
                aStream >> tmp; p.y = atoi( tmp.c_str() );
                outline.Append( p );
            }

            paths.push_back( outline );
        }

        m_polys.push_back( paths );
    }
    return true;
}


const BOX2I SHAPE_POLY_SET::BBox( int aClearance ) const
{
    BOX2I bb;

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        if( i == 0 )
            bb = m_polys[i][0].BBox();
        else
            bb.Merge( m_polys[i][0].BBox() );
    }

    bb.Inflate( aClearance );
    return bb;
}

bool SHAPE_POLY_SET::PointOnEdge( const VECTOR2I& aP ) const
{
    // Iterate through all the polygons in the set
    for( const POLYGON& polygon : m_polys )
    {
        // Iterate through all the line chains in the polygon
        for( const SHAPE_LINE_CHAIN& lineChain : polygon )
        {
            if( lineChain.PointOnEdge( aP ) )
                return true;
        }
    }

    return false;
}

bool SHAPE_POLY_SET::Collide( const VECTOR2I& aP, int aClearance ) const
{
    SHAPE_POLY_SET polySet = SHAPE_POLY_SET(*this);

    // Inflate the polygon if necessary.
    if( aClearance > 0 ){
        // fixme: the number of arc segments should not be hardcoded
        polySet.Inflate( aClearance, 8 );
    }

    // There is a collision if and only if the point is inside of the polygon.
    return polySet.Contains( aP );
}


void SHAPE_POLY_SET::RemoveAllContours()
{
    m_polys.clear();
}


void SHAPE_POLY_SET::DeletePolygon( int aIdx )
{
    m_polys.erase( m_polys.begin() + aIdx );
}


void SHAPE_POLY_SET::Append( const SHAPE_POLY_SET& aSet )
{
    m_polys.insert( m_polys.end(), aSet.m_polys.begin(), aSet.m_polys.end() );
}


void SHAPE_POLY_SET::Append( const VECTOR2I& aP, int aOutline, int aHole )
{
    Append( aP.x, aP.y, aOutline, aHole );
}


bool SHAPE_POLY_SET::Contains( const VECTOR2I& aP, int aSubpolyIndex ) const
{
    if( m_polys.size() == 0 ) // empty set?
        return false;

    // If there is a polygon specified, check the condition against that polygon
    if( aSubpolyIndex >= 0 )
        return containsSingle( aP, aSubpolyIndex );

    // In any other case, check it against all polygons in the set
    for( int polygonIdx = 0; polygonIdx < OutlineCount(); polygonIdx++ )
    {
        if( containsSingle( aP, polygonIdx ) )
            return true;
    }

    return false;

}


bool SHAPE_POLY_SET::containsSingle( const VECTOR2I& aP, int aSubpolyIndex ) const
{
    // Check that the point is inside the outline
    if( pointInPolygon( aP, m_polys[aSubpolyIndex][0] ) )
    {
        // Check that the point is not in any of the holes
        for( int holeIdx = 0; holeIdx < HoleCount( aSubpolyIndex ); holeIdx++ )
        {
            const SHAPE_LINE_CHAIN hole = CHole( aSubpolyIndex, holeIdx );
            // If the point is inside a hole (and not on its edge),
            // it is outside of the polygon
            if( pointInPolygon( aP, hole ) && !hole.PointOnEdge( aP ) )
                return false;
        }

        return true;
    }

    return false;
}


bool SHAPE_POLY_SET::pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const
{
    int result = 0;
    int cnt = aPath.PointCount();

    if ( !aPath.BBox().Contains( aP ) ) // test with bounding box first
        return false;

    if( cnt < 3 )
        return false;

    VECTOR2I ip = aPath.CPoint( 0 );

    for( int i = 1; i <= cnt; ++i )
    {
        VECTOR2I ipNext = ( i == cnt ? aPath.CPoint( 0 ) : aPath.CPoint( i ) );

        if( ipNext.y == aP.y )
        {
            if( ( ipNext.x == aP.x ) || ( ip.y == aP.y &&
                ( ( ipNext.x > aP.x ) == ( ip.x < aP.x ) ) ) )
                return true;
        }

        if( ( ip.y < aP.y ) != ( ipNext.y < aP.y ) )
        {
            if( ip.x >= aP.x )
            {
                if( ipNext.x > aP.x )
                    result = 1 - result;
                else
                {
                    int64_t d = (int64_t)( ip.x - aP.x ) * (int64_t)( ipNext.y - aP.y ) -
                                (int64_t)( ipNext.x - aP.x ) * (int64_t)( ip.y - aP.y );

                    if( !d )
                        return true;

                    if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                        result = 1 - result;
                }
            }
            else
            {
                if( ipNext.x > aP.x )
                {
                    int64_t d = (int64_t)( ip.x - aP.x ) * (int64_t)( ipNext.y - aP.y ) -
                                (int64_t)( ipNext.x - aP.x ) * (int64_t)( ip.y - aP.y );

                if( !d )
                    return true;

                if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                    result = 1 - result;
                }
            }
        }

        ip = ipNext;
    }

    return result ? true : false;
}


void SHAPE_POLY_SET::Move( const VECTOR2I& aVector )
{
    for( POLYGON &poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN &path : poly )
        {
            path.Move( aVector );
        }
    }
}


int SHAPE_POLY_SET::TotalVertices() const
{
    int c = 0;

    for( const POLYGON& poly : m_polys )
    {
        for( const SHAPE_LINE_CHAIN& path : poly )
        {
            c += path.PointCount();
        }
    }

    return c;
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::ChamferPolygon( unsigned int aDistance, int aIndex )
{
    return chamferFilletPolygon(CORNER_MODE::CHAMFERED, aDistance, aIndex);
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::FilletPolygon( unsigned int aRadius,
                                                       unsigned int aSegments,
                                                       int aIndex )
{
    return chamferFilletPolygon(CORNER_MODE::FILLETED, aRadius, aIndex, aSegments );
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::chamferFilletPolygon( CORNER_MODE aMode,
                                                              unsigned int aDistance,
                                                              int aIndex,
                                                              int aSegments )
{
    // Null segments create serious issues in calculations. Remove them:
    Simplify( PM_FAST );

    SHAPE_POLY_SET::POLYGON currentPoly = Polygon( aIndex );
    SHAPE_POLY_SET::POLYGON newPoly;

    // If the chamfering distance is zero, then the polygon remain intact.
    if( aDistance == 0 )
    {
        return currentPoly;
    }

    // Iterate through all the contours (outline and holes) of the polygon.
    for( SHAPE_LINE_CHAIN &currContour : currentPoly)
    {
        // Generate a new contour in the new polygon
        SHAPE_LINE_CHAIN newContour;

        // Iterate through the vertices of the contour
        for( int currVertex = 0; currVertex < currContour.PointCount(); currVertex++ )
        {
            // Current vertex
            int x1  = currContour.Point( currVertex ).x;
            int y1  = currContour.Point( currVertex ).y;

            // Indices for previous and next vertices.
            int prevVertex;
            int nextVertex;

            // Previous and next vertices indices computation. Necessary to manage the edge cases.

            // Previous vertex is the last one if the current vertex is the first one
            prevVertex = currVertex == 0 ? currContour.PointCount() - 1 : currVertex - 1;

            // next vertex is the first one if the current vertex is the last one.
            nextVertex = currVertex == currContour.PointCount() - 1 ? 0 : currVertex + 1;

            // Previous vertex computation
            double xa = currContour.Point( prevVertex ).x - x1;
            double ya = currContour.Point( prevVertex ).y - y1;

            // Next vertex computation
            double xb = currContour.Point( nextVertex ).x - x1;
            double yb = currContour.Point( nextVertex ).y - y1;

            // Compute the new distances
            double lena = hypot( xa, ya );
            double lenb = hypot( xb, yb );

            // Make the final computations depending on the mode selected, chamfered or filleted.
            if( aMode == CORNER_MODE::CHAMFERED )
            {
                double distance = aDistance;

                // Chamfer one half of an edge at most
                if( 0.5 * lena < distance )
                    distance = 0.5 * lena;

                if( 0.5 * lenb < distance )
                    distance = 0.5 * lenb;

                int nx1 = KiROUND( distance * xa / lena );
                int ny1 = KiROUND( distance * ya / lena );

                newContour.Append( x1 + nx1, y1 + ny1 );

                int nx2 = KiROUND( distance * xb / lenb );
                int ny2 = KiROUND( distance * yb / lenb );

                newContour.Append( x1 + nx2, y1 + ny2 );
            }
            else // CORNER_MODE = FILLETED
            {
                double cosine = ( xa * xb + ya * yb ) / ( lena * lenb );

                double radius = aDistance;
                double denom  = sqrt( 2.0 / ( 1 + cosine ) - 1 );

                // Do nothing in case of parallel edges
                if( std::isinf( denom ) )
                    continue;

                // Limit rounding distance to one half of an edge
                if( 0.5 * lena * denom < radius )
                    radius = 0.5 * lena * denom;

                if( 0.5 * lenb * denom < radius )
                    radius = 0.5 * lenb * denom;

                // Calculate fillet arc absolute center point (xc, yx)
                double k     = radius / sqrt( .5 * ( 1 - cosine ) );
                double lenab = sqrt( ( xa / lena + xb / lenb ) * ( xa / lena + xb / lenb ) +
                                        ( ya / lena + yb / lenb ) * ( ya / lena + yb / lenb ) );
                double xc = x1 + k * ( xa / lena + xb / lenb ) / lenab;
                double yc = y1 + k * ( ya / lena + yb / lenb ) / lenab;

                // Calculate arc start and end vectors
                k = radius / sqrt( 2 / ( 1 + cosine ) - 1 );
                double xs = x1 + k * xa / lena - xc;
                double ys = y1 + k * ya / lena - yc;
                double xe = x1 + k * xb / lenb - xc;
                double ye = y1 + k * yb / lenb - yc;

                // Cosine of arc angle
                double argument = ( xs * xe + ys * ye ) / ( radius * radius );

                // Make sure the argument is in [-1,1], interval in which the acos function is
                // defined
                if( argument < -1 )
                    argument = -1;
                else if( argument > 1 )
                    argument = 1;

                double arcAngle = acos( argument );

                // Calculate the number of segments
                unsigned int segments = ceil( (double) aSegments * ( arcAngle / ( 2 * M_PI ) ) );

                double deltaAngle = arcAngle / segments;
                double startAngle = atan2( -ys, xs );

                // Flip arc for inner corners
                if( xa * yb - ya * xb <= 0 )
                    deltaAngle *= -1;

                double nx = xc + xs;
                double ny = yc + ys;

                newContour.Append( KiROUND( nx ), KiROUND( ny ) );

                // Store the previous added corner to make a sanity check
                int prevX = KiROUND( nx );
                int prevY = KiROUND( ny );

                for( unsigned int j = 0; j < segments; j++ )
                {
                    nx = xc + cos( startAngle + (j + 1) * deltaAngle ) * radius;
                    ny = yc - sin( startAngle + (j + 1) * deltaAngle ) * radius;

                    // Sanity check: the rounding can produce repeated corners; do not add them.
                    if( KiROUND( nx ) != prevX || KiROUND( ny ) != prevY )
                    {
                        newContour.Append( KiROUND( nx ), KiROUND( ny ) );
                        prevX = KiROUND( nx );
                        prevY = KiROUND( ny );
                    }
                }
            }
        }

        // Close the current contour and add it the new polygon
        newContour.SetClosed( true );
        newPoly.push_back( newContour );
    }

    return newPoly;
}
