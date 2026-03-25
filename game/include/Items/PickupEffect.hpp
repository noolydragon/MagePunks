#pragma once

#include <Canis/Entity.hpp>

class PickupEffect : public Canis::ScriptableEntity
{
private:
    float m_timer = Canis::PI*0.5f;
    Canis::Vector3 m_targetPosition;
public:
    static constexpr const char* ScriptName = "PickupEffect";

    float rotationSpeed = 120.0f;
    float fallSpeed = 1.0f;
    float amplitued = 0.2f;
    float targetGroundOffset = 1.0f;
    Canis::Mask groundCollisionMask = Canis::Rigidbody::DefaultLayer;

    PickupEffect(Canis::Entity &_entity) : Canis::ScriptableEntity(_entity) {}

    void Create();
    void Ready();
    void Destroy();
    void Update(float _dt);
};

extern void RegisterPickupEffectScript(Canis::App& _app);
extern void UnRegisterPickupEffectScript(Canis::App& _app);