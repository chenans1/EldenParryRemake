#pragma once

namespace settings {
    struct config {
        bool log = true;

        //bashing is valid if (!delay and window)
        float delay = 0.02f; 
        float window = 0.36f; 
        float staggerMagnitude = 1.0f;

        bool enableAOEStagger = false;
        bool bEnableArrowProjectileDeflection = true;
        bool bEnableMagicProjectileDeflection = true;

    };
    
    template <class T>
    struct setting_definition {
        const char* key;
        T settings::config::*member;
    };

    constexpr auto setting_definitions = std::tuple{
        setting_definition<bool>{ "log", &settings::config::log },
        setting_definition<float>{ "parryDelay", &settings::config::delay },
        setting_definition<float>{ "parryWindow", &settings::config::window },
        setting_definition<float>{ "staggerMagnitude", &settings::config::staggerMagnitude },
        setting_definition<bool>{ "enableAOEStagger", &settings::config::enableAOEStagger },
        setting_definition<bool>{ "bEnableArrowProjectileDeflection", &settings::config::bEnableArrowProjectileDeflection },
        setting_definition<bool>{ "bEnableMagicProjectileDeflection", &settings::config::bEnableMagicProjectileDeflection },
    };

    config Get();
    void Set(const config& value);
    void Load();
    bool Save();

    void __stdcall RenderMenuPage();

    void RegisterMenu();
}
