#include <cmath>

#include "Square.h"

bool Square::intersectLocal( const Ray& r, Isect& i ) const
{
	vec3f p = r.getPosition();
	vec3f d = r.getDirection();

	if( d[2] == 0.0 ) {
		return false;
	}

	double t = -p[2]/d[2];

	if( t <= RAY_EPSILON ) {
		return false;
	}

	vec3f P = r.at( t );

	if( P[0] < -0.5 || P[0] > 0.5 ) {	
		return false;
	}

	if( P[1] < -0.5 || P[1] > 0.5 ) {	
		return false;
	}

	i.obj = this;
	i.t = t;
	if( d[2] > 0.0 ) {
		i.N = i.tbn[2] = vec3f( 0.0, 0.0, -1.0 );
		i.tbn[0] = vec3f(1.0, 0.0, 0.0);
		i.tbn[1] = vec3f(0.0, 1.0, 0.0);
		
	} else {
		i.N = i.tbn[2] = vec3f( 0.0, 0.0, 1.0 );
		i.tbn[0] = vec3f(-1.0, 0.0, 0.0);
		i.tbn[1] = vec3f(0.0, 1.0, 0.0);
	}

	if (enableTexCoords)
	{
		i.hasTexCoords = true;
		i.texCoords = {P[0] + 0.5, P[1] + 0.5};
		i.tbn = i.tbn.transpose();
	}

	return true;
}
