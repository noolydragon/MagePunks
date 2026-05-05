#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class HealerStateMachine;

    class SelectState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "SelectState";

        explicit SelectState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class HealState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealState";
        float hammerRestDegrees = 140.0f;
        float hammerSwingDegrees = -120.0f;
        float attackDuration = 0.75f;
        float fireDelay = 0.5f;

        explicit HealState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;
    };

    class HealerStateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::HealerStateMachine";

        Canis::SceneAssetHandle bulletPrefab = { .path = "assets/prefabs/magic_bullet.scene" };
        float bulletSpeed = 20.0f;
        int bulletHeal = 2;
        Canis::Entity* staffTipEntity = nullptr;

        std::string targetTag = "";
        float detectionRange = 20.0f;
        Canis::Vector3 bodyColliderSize = Canis::Vector3(1.0f);
        int maxHealth = 40;
        int currentHealth = 0;
        Canis::Entity* currentTarget = nullptr;
        bool logStateChanges = true;
        Canis::Entity* hammerVisual = nullptr;
        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        float hitSfxVolume = 1.0f;
        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_particles.scene" };

        explicit HealerStateMachine(Canis::Entity& _entity);

        SelectState selectState;
        HealState healState;

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

        Canis::Entity* FindLowestHPTarget() const;
        float DistanceTo(const Canis::Entity& _other) const;
        void FaceTarget(const Canis::Entity& _target);
        void MoveTowards(const Canis::Entity& _target, float _speed, float _dt);
        void ChangeState(const std::string& _stateName);
        const std::string& GetCurrentStateName() const;
        float GetStateTime() const;
        float GetAttackRange() const;
        int GetCurrentHealth() const;

        void ResetHammerPose();
        void SetHammerSwing(float _normalized);
        void TakeDamage(int _damage);
        bool IsAlive() const;
        bool hasFired = false;

    private:
        void PlayHitSfx();
        void SpawnDeathEffect();

        float m_stateTime = 0.0f;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;
        bool m_useFirstHitSfx = true;
    };

    void RegisterHealerStateMachineScript(Canis::App& _app);
    void UnRegisterHealerStateMachineScript(Canis::App& _app);
}
