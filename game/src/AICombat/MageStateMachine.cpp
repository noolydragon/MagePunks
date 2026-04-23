#include <AICombat/MageStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf mageStateMachineConf = {};
    }

    SearchState::SearchState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void SearchState::Enter()
    {
        if (MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStatMachine->ResetHammerPose();
    }

    void SearchState::Update(float)
    {
        if (MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
        {
            if (mageStatMachine->FindClosestTarget() != nullptr)
                mageStatMachine->ChangeState(WandState::Name);
        }
    }

    WandState::WandState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void WandState::Enter()
    {
        if (MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStatMachine->SetHammerSwing(0.0f);
    }

    void WandState::Update(float)
    {
        MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine);
        if (mageStatMachine == nullptr)
            return;

        if (Canis::Entity* target = mageStatMachine->FindClosestTarget())
            mageStatMachine->FaceTarget(*target);

        const float duration = std::max(attackDuration, 0.001f);
        mageStatMachine->SetHammerSwing(mageStatMachine->GetStateTime() / duration);

        if (mageStatMachine->GetStateTime() < duration)
            return;

        if (mageStatMachine->FindClosestTarget() == nullptr)
            mageStatMachine->ChangeState(SearchState::Name);
    }

    void WandState::Exit()
    {
        if (MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStatMachine->ResetHammerPose();
    }

    MageStateMachine::MageStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        searchState(*this),
        wandState(*this) {}

    void RegisterMageStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, targetTag);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, detectionRange);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, bodyColliderSize);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, wandState, hammerRestDegrees);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, wandState, hammerSwingDegrees);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, wandState, attackRange);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, wandState, attackDuration);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, wandState, attackDamageTime);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, maxHealth);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, currentHealth);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, logStateChanges);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hammerVisual);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            mageStateMachineConf,
            AICombat::MageStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        mageStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::MageStateMachine);

        _app.RegisterScript(mageStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(mageStateMachineConf, MageStateMachine)

    void MageStateMachine::Create()
    {
        entity.GetComponent<Canis::Transform>();

        Canis::Rigidbody& rigidbody = entity.GetComponent<Canis::Rigidbody>();
        rigidbody.motionType = Canis::RigidbodyMotionType::KINEMATIC;
        rigidbody.useGravity = false;
        rigidbody.allowSleeping = false;
        rigidbody.linearVelocity = Canis::Vector3(0.0f);
        rigidbody.angularVelocity = Canis::Vector3(0.0f);

        entity.GetComponent<Canis::BoxCollider>().size = bodyColliderSize;

        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }
    }

    void MageStateMachine::Ready()
    {
        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }

        currentHealth = std::max(maxHealth, 1);
        m_stateTime = 0.0f;
        m_useFirstHitSfx = true;

        ClearStates();
        AddState(searchState);
        AddState(wandState);

        ResetHammerPose();
        ChangeState(SearchState::Name);
    }

    void MageStateMachine::Destroy()
    {
        hammerVisual = nullptr;
        SuperPupUtilities::StateMachine::Destroy();
    }

    void MageStateMachine::Update(float _dt)
    {
        if (!IsAlive())
            return;

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* MageStateMachine::FindClosestTarget() const
    {
        if (targetTag.empty() || !entity.HasComponent<Canis::Transform>())
            return nullptr;

        const Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 origin = transform.GetGlobalPosition();
        Canis::Entity* closestTarget = nullptr;
        float closestDistance = detectionRange;

        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            if (candidate == nullptr || candidate == &entity || !candidate->active)
                continue;

            if (!candidate->HasComponent<Canis::Transform>())
                continue;

            if (const MageStateMachine* other = candidate->GetScript<MageStateMachine>())
            {
                if (!other->IsAlive())
                    continue;
            }

            const Canis::Vector3 candidatePosition = candidate->GetComponent<Canis::Transform>().GetGlobalPosition();
            const float distance = glm::distance(origin, candidatePosition);

            if (distance > detectionRange || distance >= closestDistance)
                continue;

            closestDistance = distance;
            closestTarget = candidate;
        }

        return closestTarget;
    }

    float MageStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void MageStateMachine::FaceTarget(const Canis::Entity& _target)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.rotation.y = std::atan2(-direction.x, -direction.z);
    }

    void MageStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.position += direction * _speed * _dt;
    }

    void MageStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& MageStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float MageStateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    float MageStateMachine::GetAttackRange() const
    {
        return wandState.attackRange;
    }

    int MageStateMachine::GetCurrentHealth() const
    {
        return currentHealth;
    }

    void MageStateMachine::ResetHammerPose()
    {
        SetHammerSwing(0.0f);
    }

    void MageStateMachine::SetHammerSwing(float _normalized)
    {
        if (hammerVisual == nullptr || !hammerVisual->HasComponent<Canis::Transform>())
            return;

        Canis::Transform& hammerTransform = hammerVisual->GetComponent<Canis::Transform>();
        const float normalized = Clamp01(_normalized);
        const float swingBlend = (normalized <= 0.5f)
            ? normalized * 2.0f
            : (1.0f - normalized) * 2.0f;

        hammerTransform.rotation.x = DEG2RAD *
            (wandState.hammerRestDegrees + (wandState.hammerSwingDegrees * swingBlend));
    }

    void MageStateMachine::TakeDamage(int _damage)
    {
        if (!IsAlive())
            return;

        const int damageToApply = std::max(_damage, 0);
        if (damageToApply <= 0)
            return;

        currentHealth = std::max(0, currentHealth - damageToApply);
        PlayHitSfx();

        if (m_hasBaseColor && entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();
            const float healthRatio = (maxHealth > 0)
                ? (static_cast<float>(currentHealth) / static_cast<float>(maxHealth))
                : 0.0f;

            material.color = Canis::Vector4(
                m_baseColor.x * (0.5f + (0.5f * healthRatio)),
                m_baseColor.y * (0.5f + (0.5f * healthRatio)),
                m_baseColor.z * (0.5f + (0.5f * healthRatio)),
                m_baseColor.w);
        }

        if (currentHealth > 0)
            return;

        if (logStateChanges)
            Canis::Debug::Log("%s was defeated.", entity.name.c_str());

        SpawnDeathEffect();
        entity.Destroy();
    }

    void MageStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void MageStateMachine::SpawnDeathEffect()
    {
        if (deathEffectPrefab.Empty() || !entity.HasComponent<Canis::Transform>())
            return;

        const Canis::Transform& sourceTransform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 spawnPosition = sourceTransform.GetGlobalPosition();
        const Canis::Vector3 spawnRotation = sourceTransform.GetGlobalRotation();

        for (Canis::Entity* spawnedEntity : entity.scene.Instantiate(deathEffectPrefab))
        {
            if (spawnedEntity == nullptr || !spawnedEntity->HasComponent<Canis::Transform>())
                continue;

            Canis::Transform& spawnedTransform = spawnedEntity->GetComponent<Canis::Transform>();
            spawnedTransform.position = spawnPosition;
            spawnedTransform.rotation = spawnRotation;
        }
    }

    bool MageStateMachine::IsAlive() const
    {
        return currentHealth > 0;
    }
}
