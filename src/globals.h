#ifndef GLOBALS_H
#define GLOBALS_H

static const Magnum::Float BALL_MASS = 5.0f;
static const Magnum::Float BALL_RADIUS = 0.25;
static const Magnum::Vector3 BALL_SCALE{BALL_RADIUS, BALL_RADIUS, BALL_RADIUS};

static const Magnum::Float PLAYER_MASS = 20.0;
static const Magnum::Float PLAYER_HEIGHT = 1.8;
static const Magnum::Float PLAYER_RADIUS = 0.3;
static const Magnum::Vector3 PLAYER_SCALE{0.3, 0.3, 0.3};
static const btScalar PLAYER_JUMP_VELOCITY = 8.0;
static const btScalar PLAYER_WALK_SPEED = 5.0;
static const btScalar PLAYER_WALK_ACCEL = 200.0;

namespace PhysicsUserIndex { // TODO: use enum class while allowing conversion to int

enum Enum
{
    None,
    Player,
    JumpPad
};

} // namespace PhysicsUserIndex

#endif // GLOBALS_H
