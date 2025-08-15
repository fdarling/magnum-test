#include "RigidBody.h"
#include "globals.h"

static btVector3 generateInertia(btScalar mass, btCollisionShape *bShape)
{
    btVector3 bInertia{0.0f, 0.0f, 0.0f};
    if(!Magnum::Math::TypeTraits<Magnum::Float>::equals(mass, 0.0f))
        bShape->calculateLocalInertia(mass, bInertia);
    return bInertia;
}

RigidBody::RigidBody(Object3D &object, Magnum::Float mass, btCollisionShape *bShape, btDynamicsWorld &bWorld) :
    Magnum::BulletIntegration::MotionState{object},
    _bWorld(bWorld),
    _body(btRigidBody::btRigidBodyConstructionInfo{mass, &btMotionState(), bShape, generateInertia(mass, bShape)})
{
    // _body.forceActivationState(DISABLE_DEACTIVATION);
    bWorld.addRigidBody(&_body);
}

RigidBody::~RigidBody()
{
    _bWorld.removeRigidBody(&_body);
}
