#include "TexturedDrawable.h"

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Shaders/PhongGL.h>

TexturedDrawable::TexturedDrawable(Object3D &object, Magnum::Shaders::PhongGL &shader, Magnum::GL::Mesh &mesh, Magnum::GL::Texture2D &texture, Magnum::SceneGraph::DrawableGroup3D &group) :
    Magnum::SceneGraph::Drawable3D{object, &group},
    _shader(shader),
    _mesh(mesh),
    _texture(texture)
{
}

void TexturedDrawable::draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera)
{
    _shader
        .setLightPositions({
            {camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}
        })
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_texture)
        .draw(_mesh);
}
