#include <SuperPupUtilities/FPSCounter.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

namespace SuperPupUtilities
{
    ScriptConf fpsCounterConf = {};

    void RegisterFPSCounterScript(App& _app)
    {
        DEFAULT_CONFIG_AND_REQUIRED(fpsCounterConf, SuperPupUtilities::FPSCounter, RectTransform);

        fpsCounterConf.DEFAULT_DRAW_INSPECTOR(SuperPupUtilities::FPSCounter);

        _app.RegisterScript(fpsCounterConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(fpsCounterConf, FPSCounter)

    void FPSCounter::Create() {}

    void FPSCounter::Ready() {}

    void FPSCounter::Destroy() {}

    void FPSCounter::Update(float _dt)
    {
        if (!entity.HasComponents<RectTransform, Text>())
            return;

        RectTransform& rect = entity.GetComponent<RectTransform>();
        Text& text = entity.GetComponent<Text>();

        text.text = ("FPS: " + std::to_string((int)entity.scene.app->FPS()));
    }
}
