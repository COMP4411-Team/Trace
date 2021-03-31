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

void Sphere::setTexCoords(Isect& isect) const
{
	if (enableTexCoords)
	{
		isect.hasTexCoords = true;
		double u, v;
		if (isect.N[0] != 0.0)
			u = (atan2(isect.N[1], isect.N[0]) + PI) / (PI_2);
		v = asin(isect.N[2]) / PI + 0.5;
		isect.texCoords = {u, v};
		
		isect.tbn[0] = vec3f(-isect.N[1], isect.N[0], 0.0).normalize();
		isect.tbn[1] = isect.N.cross(isect.tbn[0]);
		isect.tbn[2] = isect.N;
		isect.tbn = isect.tbn.transpose();
	}
}

void SphereLight::calInfo()
{
	position = transform->xform * vec3f();
	radius = (transform->xform * vec3f(1.0, 0.0, 0.0) - position).length();
	area = 4.0 * PI * radius * radius;
}

Ray SphereLight::sample(vec3f& emit, double& pdf) const
{
	double theta = 2.0 * PI * getUniformReal(), phi = PI * getUniformReal();
    vec3f dir(cos(phi), sin(phi) * cos(theta), sin(phi) * sin(theta));
	Ray ray(position + radius * dir, dir);
    emit = emission;
    pdf = 1.0 / area;
	return ray;
}


bool movingSphere::intersect(const Ray& r, Isect& i) const
{
	vec3f center = getCurPosition(r.getTime());
	vec3f normal = r.normalToPoint(center);
	if (normal.length() > radius)
		return false;
	
	vec3f isectPos = center + normal - sqrt(radius * radius - normal.length_squared()) * r.getDirection();
	i.t = r.solveT(isectPos);
	if (i.t < 0.0)
		return false;
	i.N = (isectPos - center).normalize();
	i.obj = this;
	setTexCoords(i);
	return true;
}

vec3f movingSphere::getCurPosition(double time) const
{
	if (time1 == time0 || time < time0)
		return pos;
	if (time > time1)
		return target;
	return (target - pos) / (time1 - time0) * (time - time0) + pos;
}
