#ifndef COLOREDDRAWABLE_H
#define COLOREDDRAWABLE_H

#include "Object3D.h"

#include <Magnum/Math/Color.h>
#include <Magnum/SceneGraph/Drawable.h>

namespace Magnum {
namespace Shaders {

class PhongGL;

} // namespace Shaders
namespace GL {

class Mesh;

} // namespace GL
} // namespace Magnum

class ColoredDrawable : public Magnum::SceneGraph::Drawable3D
{
public:
    explicit ColoredDrawable(Object3D &object, const Magnum::Vector3 &scaling, Magnum::Shaders::PhongGL &shader, Magnum::GL::Mesh &mesh, const Magnum::Color4& color, Magnum::SceneGraph::DrawableGroup3D &group);
private:
    void draw(const Magnum::Matrix4 &transformationMatrix, Magnum::SceneGraph::Camera3D &camera) override;

    Magnum::Shaders::PhongGL &_shader;
    Magnum::GL::Mesh &_mesh;
    Magnum::Color4 _color;
    Magnum::Vector3 _scaling;
};

#endif // COLOREDDRAWABLE_H
