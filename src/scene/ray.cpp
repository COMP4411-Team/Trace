#include "Ray.h"
#include "material.h"
#include "scene.h"

const Material &
Isect::getMaterial() const
{
    return material ? *material : obj->getMaterial();
}
