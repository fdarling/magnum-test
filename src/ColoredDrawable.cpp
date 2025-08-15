#include "ColoredDrawable.h"

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Shaders/PhongGL.h>

ColoredDrawable::ColoredDrawable(Object3D &object, const Magnum::Vector3 &scaling, Magnum::Shaders::PhongGL &shader, Magnum::GL::Mesh &mesh, const Magnum::Color4& color, Magnum::SceneGraph::DrawableGroup3D &group) :
    Magnum::SceneGraph::Drawable3D{object, &group},
    _shader(shader),
    _mesh(mesh),
    _color{color},
    _scaling{scaling}
{
}

void ColoredDrawable::draw(const Magnum::Matrix4 &transformationMatrix, Magnum::SceneGraph::Camera3D &camera)
{
    const Magnum::Matrix4 transformation = transformationMatrix*Magnum::Matrix4::scaling(_scaling);

    _shader
        .setAmbientColor(_color*0.1f) // HACK
        .setDiffuseColor(_color)
        .setTransformationMatrix(transformation)
        .setNormalMatrix(transformation.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}
