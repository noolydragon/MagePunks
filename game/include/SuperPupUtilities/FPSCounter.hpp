#pragma once

#include <Canis/Entity.hpp>

namespace SuperPupUtilities
{
    class FPSCounter : public Canis::ScriptableEntity
    {
    private:

    public:
        static constexpr const char* ScriptName = "SuperPupUtilities::FPSCounter";

        float moveForce = 35.0f;
        float jumpImpulse = 7.5f;
        float groundCheckDistance = 0.75f;
        Canis::Mask groundCollisionMask = Canis::Rigidbody::DefaultLayer;
        float pickupRadius = 1.15f;

        FPSCounter(Canis::Entity &_entity) : Canis::ScriptableEntity(_entity) {}

        void Create();
        void Ready();
        void Destroy();
        void Update(float _dt);
    };

    extern void RegisterFPSCounterScript(Canis::App& _app);
    extern void UnRegisterFPSCounterScript(Canis::App& _app);
}