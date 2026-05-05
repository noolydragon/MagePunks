#include <AICombat/HealerStateMachine.hpp>
#include <AICombat/MageStateMachine.hpp>
#include <AICombat/BrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <SuperPupUtilities/Bullet.hpp>
#include <SuperPupUtilities/SimpleObjectPool.hpp>

#include <Canis/Debug.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf healerStateMachineConf = {};
    }

    SelectState::SelectState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void SelectState::Enter()
    {
        if (HealerStateMachine* healerStatMachine = dynamic_cast<HealerStateMachine*>(m_stateMachine))
            healerStatMachine->ResetHammerPose();
    }

    void SelectState::Update(float)
    {
        if (HealerStateMachine* healerStatMachine = dynamic_cast<HealerStateMachine*>(m_stateMachine))
        {
            if (healerStatMachine->FindLowestHPTarget() != nullptr)
                healerStatMachine->ChangeState(HealState::Name);
        }
    }

    HealState::HealState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealState::Enter()
    {
        if (auto* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine)) {
            healer->hasFired = false;
            healer->ResetHammerPose();
        }
    }

    void HealState::Update(float _dt)
    {
        HealerStateMachine* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine);
        if (healer == nullptr)
            return;

        healer->currentTarget = healer->FindLowestHPTarget();
        if (!healer->currentTarget || !healer->currentTarget->active) {
            healer->currentTarget = nullptr;
            healer->ChangeState(SelectState::Name);
            return;
        }
        
        auto* mage = healer->currentTarget->GetScript<AICombat::MageStateMachine>();
        auto* brawler = healer->currentTarget->GetScript<AICombat::BrawlerStateMachine>();

        if (mage) {
            mage->isBeingHealed = true;
            mage->healerID = (unsigned int)healer->entity.id;
        } else if (brawler) {
            brawler->isBeingHealed = true;
            brawler->healerID = (unsigned int)healer->entity.id;
        }

        healer->FaceTarget(*healer->currentTarget);
        healer->MoveTowards(*healer->currentTarget, 5.0f, _dt);

        if (healer->GetStateTime() >= 0.5f && !healer->hasFired)
        {
            auto* pool = SuperPupUtilities::SimpleObjectPool::Instance; 
            if (pool && !healer->bulletPrefab.Empty() && healer->staffTipEntity)
            {
                Canis::Transform& tipTransform = healer->staffTipEntity->GetComponent<Canis::Transform>();
                Canis::Entity* bulletEnt = pool->Spawn("magic_bullet", tipTransform.GetGlobalPosition(), tipTransform.GetGlobalRotation());

                if (bulletEnt)
                {
                    if (auto* bullet = bulletEnt->GetScript<SuperPupUtilities::Bullet>())
                    {
                        bullet->speed = healer->bulletSpeed;
                        bullet->damage = -(healer->bulletHeal);
                        bullet->targetTags = { healer->targetTag };
                        bullet->destroyOnImpact = true;
                        bullet->Launch();
                    }
                    healer->hasFired = true;
                }
            }
        }

        if (healer->GetStateTime() >= attackDuration)
            healer->ChangeState(SelectState::Name);
    }

void HealState::Exit()
{
    HealerStateMachine* healer = dynamic_cast<HealerStateMachine*>(m_stateMachine);
    if (!healer) return;

    healer->ResetHammerPose();

    if (healer->currentTarget) 
    {
        auto* mage = healer->currentTarget->GetScript<AICombat::MageStateMachine>();
        auto* brawler = healer->currentTarget->GetScript<AICombat::BrawlerStateMachine>();

        if (mage && mage->healerID == (unsigned int)healer->entity.id) {
            mage->isBeingHealed = false;
            mage->healerID = 0;
        } 
        else if (brawler && brawler->healerID == (unsigned int)healer->entity.id) {
            brawler->isBeingHealed = false;
            brawler->healerID = 0;
        }
    }
    healer->currentTarget = nullptr;
}

    HealerStateMachine::HealerStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        selectState(*this),
        healState(*this) {}

    void RegisterHealerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, targetTag);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, detectionRange);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, bodyColliderSize);
        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, healState, hammerRestDegrees);
        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, healState, hammerSwingDegrees);
        RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, healState, attackDuration);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, bulletPrefab);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, bulletSpeed);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, bulletHeal);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, staffTipEntity);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, maxHealth);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, currentHealth);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, logStateChanges);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hammerVisual);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            healerStateMachineConf,
            AICombat::HealerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        healerStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::HealerStateMachine);

        _app.RegisterScript(healerStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(healerStateMachineConf, HealerStateMachine)

    void HealerStateMachine::Create()
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

    void HealerStateMachine::Ready()
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
        AddState(selectState);
        AddState(healState);

        ResetHammerPose();
        ChangeState(SelectState::Name);
    }

    void HealerStateMachine::Destroy()
    {
        hammerVisual = nullptr;
        SuperPupUtilities::StateMachine::Destroy();
    }

    void HealerStateMachine::Update(float _dt)
    {
        if (!IsAlive()) return;

        if (entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();
            float healthRatio = (maxHealth > 0) ? ((float)currentHealth / (float)maxHealth) : 0.0f;
            float brightness = 0.5f + (0.5f * healthRatio);

            Canis::Vector4 targetColor = Canis::Vector4(
                m_baseColor.x * brightness,
                m_baseColor.y * brightness,
                m_baseColor.z * brightness,
                m_baseColor.w
            );
            float lerpFactor = std::clamp(_dt * 8.0f, 0.0f, 1.0f);
            material.color = material.color + (targetColor - material.color) * lerpFactor;
        }

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

Canis::Entity* HealerStateMachine::FindLowestHPTarget() const
{
    if (targetTag.empty() || !entity.HasComponent<Canis::Transform>())
        return nullptr;

    const Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
    const Canis::Vector3 origin = transform.GetGlobalPosition();
    
    Canis::Entity* bestTarget = nullptr;
    float lowestHealthRatio = 1.001f; 

    for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
    {
        if (candidate == nullptr || candidate == &entity || !candidate->active)
            continue;

        auto* mage = candidate->GetScript<AICombat::MageStateMachine>();
        auto* brawler = candidate->GetScript<AICombat::BrawlerStateMachine>();

        if (!mage && !brawler)
            continue;
        
        int tCurrentHealth = mage ? mage->currentHealth : brawler->currentHealth;
        int tMaxHealth = mage ? mage->maxHealth : brawler->maxHealth;
        bool tIsBeingHealed = mage ? mage->isBeingHealed : brawler->isBeingHealed;
        unsigned int tHealerID = mage ? mage->healerID : brawler->healerID;
        bool tIsAlive = mage ? mage->IsAlive() : brawler->IsAlive();
        
        if (!tIsAlive || tCurrentHealth >= tMaxHealth)
            continue;
        if (tIsBeingHealed && tHealerID != (unsigned int)entity.id)
            continue;

        const Canis::Vector3 candidatePosition = candidate->GetComponent<Canis::Transform>().GetGlobalPosition();
        const float distance = glm::distance(origin, candidatePosition);

        if (distance > detectionRange)
            continue;

        float healthRatio = (float)tCurrentHealth / (float)tMaxHealth;

        if (healthRatio < lowestHealthRatio)
        {
            lowestHealthRatio = healthRatio;
            bestTarget = candidate;
        }
    }

    return bestTarget;
}

    float HealerStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void HealerStateMachine::FaceTarget(const Canis::Entity& _target)
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

    void HealerStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
        return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        const Canis::Vector3 targetPosition = _target.GetComponent<Canis::Transform>().GetGlobalPosition();
        
        Canis::Vector3 direction = targetPosition - selfPosition;
        direction.y = 0.0f; // Keep movement on a 2D plane

        float distance = glm::length(direction);

        const float minDistance = 2.0f;
        if (distance <= minDistance)
            return;

        direction = glm::normalize(direction);

        float moveAmount = _speed * _dt;

        if (distance - moveAmount < minDistance)
            moveAmount = distance - minDistance;

        transform.position += direction * moveAmount;
    }

    void HealerStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& HealerStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float HealerStateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    int HealerStateMachine::GetCurrentHealth() const
    {
        return currentHealth;
    }

    void HealerStateMachine::ResetHammerPose()
    {
        SetHammerSwing(0.0f);
    }

    void HealerStateMachine::SetHammerSwing(float _normalized)
    {
        if (hammerVisual == nullptr || !hammerVisual->HasComponent<Canis::Transform>())
            return;

        Canis::Transform& hammerTransform = hammerVisual->GetComponent<Canis::Transform>();
        const float normalized = Clamp01(_normalized);
        const float swingBlend = (normalized <= 0.5f)
            ? normalized * 2.0f
            : (1.0f - normalized) * 2.0f;

        hammerTransform.rotation.x = DEG2RAD *
            (healState.hammerRestDegrees + (healState.hammerSwingDegrees * swingBlend));
    }

    void HealerStateMachine::TakeDamage(int _damage)
    {
        if (!IsAlive())
            return;

        if (_damage < 0)
        {
            currentHealth = std::min(maxHealth, currentHealth - _damage);

            if (entity.HasComponent<Canis::Material>())
            {
                entity.GetComponent<Canis::Material>().color = Canis::Vector4(0.0f, 1.0f, 0.0f, 1.0f);
            }
            return;
        }

        const int damageToApply = _damage;
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

    void HealerStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void HealerStateMachine::SpawnDeathEffect()
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

    bool HealerStateMachine::IsAlive() const
    {
        return currentHealth > 0;
    }
}
