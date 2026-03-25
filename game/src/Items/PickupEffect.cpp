#include <Items/PickupEffect.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>


ScriptConf pickupEffectConf = {};

void RegisterPickupEffectScript(App& _app)
{
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, rotationSpeed);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, fallSpeed);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, amplitued);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, targetGroundOffset);
    REGISTER_PROPERTY(pickupEffectConf, PickupEffect, groundCollisionMask);
    
    DEFAULT_CONFIG_AND_REQUIRED(pickupEffectConf, PickupEffect, RectTransform);

    pickupEffectConf.DEFAULT_DRAW_INSPECTOR(PickupEffect);

    _app.RegisterScript(pickupEffectConf);
}

DEFAULT_UNREGISTER_SCRIPT(pickupEffectConf, PickupEffect)

void PickupEffect::Create() {}

void PickupEffect::Ready() {
    if (!entity.HasComponents<Transform, Rigidbody>())
        return;

    Transform& transform = entity.GetComponent<Transform>();

    RaycastHit groundHit = {};
    const Vector3 groundRayOrigin = transform.GetGlobalPosition();
    
    if(entity.scene.Raycast(
        groundRayOrigin,
        Vector3(0.0f, -1.0f, 0.0f),
        groundHit, 10.0f, groundCollisionMask))
    {
        m_targetPosition = groundHit.point + Vector3(0.0f, targetGroundOffset, 0.0f);
    }
}

void PickupEffect::Destroy() {}

void PickupEffect::Update(float _dt) {
    if (!entity.HasComponents<Transform, Rigidbody>())
        return;

    Transform& transform = entity.GetComponent<Transform>();
    transform.rotation.y += rotationSpeed * DEG2RAD * _dt;

    transform.position = m_targetPosition + Vector3(0.0f, std::sin(m_timer * fallSpeed * PI) * amplitued, 0.0f);
    m_timer += _dt;
}
