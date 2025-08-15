#include "ColoredDrawable.h"
#include "RigidBody.h"

#include <Magnum/GL/Mesh.h>

class Player : public Object3D, public ColoredDrawable
{
public:
    explicit Player(Object3D *parent, Magnum::Shaders::PhongGL &shader, Magnum::SceneGraph::DrawableGroup3D &group, btDynamicsWorld &bWorld);
    btRigidBody & rigidBody()
    {
        return _body.rigidBody();
    }
    const btRigidBody & rigidBody() const
    {
        return _body.rigidBody();
    }
    void walkInDirection(const Magnum::Vector3 &dir);
    void jump();
    bool isOnGround();

    Magnum::GL::Mesh _mesh;
    btCapsuleShape _capsuleShape;
    RigidBody _body;
};
