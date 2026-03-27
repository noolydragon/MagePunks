#include <Items/PickupEffect.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>


ScriptConf pickupEffectConf = {};

void RegisterPickupEffectScript(App& _app)
{
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, rotationSpeed);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, fallSpeed);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, amplitued);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, falling);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, recheckGround);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, targetGroundOffset);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, groundCollisionMask);
    
    DEFAULT_CONFIG_AND_REQUIRED(pickupEffectConf, PickupEffect, RectTransform);

    pickupEffectConf.DEFAULT_DRAW_INSPECTOR(PickupEffect);

    _app.RegisterScript(pickupEffectConf);
}

DEFAULT_UNREGISTER_SCRIPT(pickupEffectConf, PickupEffect)

void PickupEffect::Create() {}

void PickupEffect::Ready() {
    RefreshTargetPosition();
}

void PickupEffect::Destroy() {}

void PickupEffect::Update(float _dt) {
    if (!entity.HasComponents<Transform, Rigidbody>())
        return;

    Transform& transform = entity.GetComponent<Transform>();

    if (falling) {
        float targetLow = m_targetPosition.y - amplitued;
        
        transform.position.y -= fallSpeed * _dt;

        if (transform.position.y <= targetLow) {
            falling = false;
            m_direction = Vector3(0.0f, 1.0f, 0.0f);
        }
    } else {
        Vector3 target = m_targetPosition + (m_direction * amplitued);
        transform.position += m_direction * fallSpeed * _dt;

        if (m_direction.y == 1.0f && transform.position.y >= target.y)
        {
             m_direction.y = -1.0f;
        } else if (m_direction.y == -1.0f && transform.position.y <= target.y) {
             m_direction.y = 1.0f;
        }
    }

    transform.rotation.y += rotationSpeed * DEG2RAD * _dt;
    
    m_timer += _dt;
    m_groundCheckTimer -= _dt;

    if (m_groundCheckTimer <= 0.0f)
        RefreshTargetPosition();
}

void PickupEffect::RefreshTargetPosition() {
    if (!entity.HasComponents<Transform, Rigidbody>())
        return;

    m_groundCheckTimer = targetGroundOffset;

    Transform& transform = entity.GetComponent<Transform>();

    RaycastHit groundHit = {};
    const Vector3 groundRayOrigin = transform.GetGlobalPosition();
    
    if(entity.scene.Raycast(
        groundRayOrigin,
        Vector3(0.0f, -1.0f, 0.0f),
        groundHit, 10.0f, groundCollisionMask))
    {
        Vector3 oldPosition = m_targetPosition;
        m_targetPosition = groundHit.point + Vector3(0.0f, targetGroundOffset, 0.0f);

        if (oldPosition != m_targetPosition)
            falling = true;
    }
}
