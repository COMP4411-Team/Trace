#include <cmath>

#include "Sphere.h"

const double PI = 3.14159265358979323846;
const double PI_2 = 6.28318530717958647692;

bool Sphere::intersectLocal( const Ray& r, Isect& i ) const
{
	vec3f v = -r.getPosition();
	double b = v.dot(r.getDirection());
	double discriminant = b*b - v.dot(v) + 1;

	if( discriminant < 0.0 ) {
		return false;
	}

	discriminant = sqrt( discriminant );
	double t2 = b + discriminant;

	if( t2 <= RAY_EPSILON ) {
		return false;
	}

	i.obj = this;

	double t1 = b - discriminant;

	if( t1 > RAY_EPSILON ) {
		i.t = t1;
		i.N = r.at( t1 ).normalize();
	} else {
		i.t = t2;
		i.N = r.at( t2 ).normalize();
	}

	if (enableTexCoords)
	{
		i.hasTexCoords = true;
		double u, v;
		if (i.N[0] != 0.0)
			u = (atan2(i.N[1], i.N[0]) + PI) / (PI_2);
		v = asin(i.N[2]) / PI + 0.5;
		i.texCoords = {u, v};
		
		i.tbn[0] = vec3f(-i.N[1], i.N[0], 0.0).normalize();
		i.tbn[1] = i.N.cross(i.tbn[0]);
		i.tbn[2] = i.N;
		i.tbn = i.tbn.transpose();
	}

	return true;
}

