#include "Player.h"
#include "globals.h"

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/BulletIntegration/Integration.h>

Player::Player(Object3D *parent, Magnum::Shaders::PhongGL &shader, Magnum::SceneGraph::DrawableGroup3D &group, btDynamicsWorld &bWorld) :
    Object3D{parent},
    ColoredDrawable{*this, PLAYER_SCALE, shader, _mesh, Magnum::Color3::fromLinearRgbInt(0xffaaaa), group},
    _mesh{Magnum::MeshTools::compile(Magnum::Primitives::capsule3DSolid(
        16, // hemisphereRings
        4, // cylinderRings
        32, // segments
        ((PLAYER_HEIGHT-2.0*PLAYER_RADIUS)/PLAYER_RADIUS)/2.0, // halfLength
        Magnum::Primitives::CapsuleFlags{} // flags
    ))},
    _capsuleShape{PLAYER_RADIUS, PLAYER_HEIGHT-2.0*PLAYER_RADIUS},
    _body{*this, PLAYER_MASS, &_capsuleShape, bWorld}
{
    btTransform playerTransform;
    playerTransform.setIdentity();
    playerTransform.setOrigin(btVector3(6, PLAYER_HEIGHT/2.0+0.01, 0));

    rigidBody().setAngularFactor(btVector3(0, 0, 0)); // prevent tipping over
    rigidBody().setFriction(0.5f);
    rigidBody().setDamping(0.2f, 0.2f);
    rigidBody().setUserIndex(PhysicsUserIndex::Player);
    rigidBody().setWorldTransform(playerTransform);
}

void Player::walkInDirection(const Magnum::Vector3 &dir)
{
    const btVector3 bDir{dir};
    const btVector3 currentVelocity = rigidBody().getLinearVelocity();
    const btScalar mass = rigidBody().getMass();
    const btScalar speedInDesiredDirection = currentVelocity.dot(bDir);
    const btScalar walk_accel = PLAYER_WALK_ACCEL*std::max(0.0, std::min(1.0, 1.0 - speedInDesiredDirection/PLAYER_WALK_SPEED));
    const btVector3 force = mass*walk_accel*bDir;
    if (!dir.isZero() && !force.isZero())
    {
        rigidBody().activate();
        rigidBody().applyCentralForce(force);
        // std::cout << "force: (" << force.x() << "," << force.y() << "," << force.z() << ")" << std::endl;
    }
}

void Player::jump()
{
    // if (_standingOnGround && jmpPressed)
    {
        btVector3 v = rigidBody().getLinearVelocity();
        v.setY(PLAYER_JUMP_VELOCITY);  // Adjust for desired jump strength
        rigidBody().activate();
        rigidBody().setLinearVelocity(v);
    }
}

struct ContactCallback : public btCollisionWorld::ContactResultCallback
{
    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int, int, const btCollisionObjectWrapper* colObj1Wrap, int, int) override
    {
        btVector3 normal = cp.m_normalWorldOnB;
        if (colObj1Wrap->m_collisionObject->getUserIndex() == 1)
            normal = -normal; // Player is second body
        if (normal.getY() > 0.4 && cp.getDistance() < 0.f)
            onGround = true;
        return 0.0;
    }
    bool onGround{false};
};

bool Player::isOnGround()
{
    ContactCallback callback;
    _body.dynamicsWorld().contactTest(&rigidBody(), callback);
    return callback.onGround;
}
