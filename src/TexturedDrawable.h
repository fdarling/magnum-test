#ifndef TEXTUREDDRAWABLE_H
#define TEXTUREDDRAWABLE_H

#include "Object3D.h"

#include <Magnum/SceneGraph/Drawable.h>

namespace Magnum {
namespace Shaders {

class PhongGL;

} // namespace Shaders
namespace GL {

class Mesh;
template<UnsignedInt> class Texture;
typedef Texture<2> Texture2D;

} // namespace GL
} // namespace Magnum

class TexturedDrawable: public Magnum::SceneGraph::Drawable3D
{
public:
    explicit TexturedDrawable(Object3D &object, Magnum::Shaders::PhongGL &shader, Magnum::GL::Mesh &mesh, Magnum::GL::Texture2D &texture, Magnum::SceneGraph::DrawableGroup3D &group);
private:
    void draw(const Magnum::Matrix4 &transformationMatrix, Magnum::SceneGraph::Camera3D &camera) override;

    Magnum::Shaders::PhongGL &_shader;
    Magnum::GL::Mesh &_mesh;
    Magnum::GL::Texture2D &_texture;
};

#endif // TEXTUREDDRAWABLE_H
