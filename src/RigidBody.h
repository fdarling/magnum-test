#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <btBulletDynamicsCommon.h>

#include <Magnum/BulletIntegration/MotionState.h>

#include "Object3D.h"

class RigidBody : public Magnum::BulletIntegration::MotionState
{
public:
    explicit RigidBody(Object3D &object, Magnum::Float mass, btCollisionShape *bShape, btDynamicsWorld &bWorld);

    ~RigidBody() override;

    btRigidBody & rigidBody()
    {
        return _body;
    }
    const btRigidBody & rigidBody() const
    {
        return _body;
    }
    btDynamicsWorld & dynamicsWorld()
    {
        return _bWorld;
    }
    const btDynamicsWorld & dynamicsWorld() const
    {
        return _bWorld;
    }

    // needed after changing the pose from Magnum side
    // void syncPose() {
        // _body->setWorldTransform(btTransform(transformationMatrix()));
    // }

protected:
    btDynamicsWorld &_bWorld;
    btRigidBody _body;
};

#endif // RIGIDBODY_H
