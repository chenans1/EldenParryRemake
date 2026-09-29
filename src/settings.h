#pragma once

namespace settings {
    struct config {
        bool log = true;

        //bashing is valid if (!delay and window)
        float delay = 0.08f; 
        float window = 0.24f; 
        float staggerMagnitude = 1.0f;
        float meleeBlockExperience = 10.0f;
        float projectileBlockExperience = 20.0f;

        bool bShieldEnabled = true;
        bool bNonShieldEnabled = true;
        bool enablePowerBashParry = false;
        bool enableNPCParry = false;
        bool enableSoundEffects = true;
        bool enableSingleTargetStagger = false;
        bool enableAOEStagger = true;
        float AOEStaggerRadius = 128.0f;
        bool includeDirectAttackerInAOEStagger = true;
        bool bEnableArrowProjectileDeflection = true;
        bool bEnableMagicProjectileDeflection = true;
        bool enableRangedStagger = false;
        bool enableSpellCasterStagger = false;

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
        setting_definition<float>{ "meleeBlockExperience", &settings::config::meleeBlockExperience },
        setting_definition<float>{ "projectileBlockExperience", &settings::config::projectileBlockExperience },
        setting_definition<bool>{ "bShieldEnabled", &settings::config::bShieldEnabled },
        setting_definition<bool>{ "bNonShieldEnabled", &settings::config::bNonShieldEnabled },
        setting_definition<bool>{ "enablePowerBashParry", &settings::config::enablePowerBashParry },
        setting_definition<bool>{ "enableSingleTargetStagger", &settings::config::enableSingleTargetStagger },
        setting_definition<bool>{ "enableSoundEffects", &settings::config::enableSoundEffects },
        setting_definition<bool>{ "enableAOEStagger", &settings::config::enableAOEStagger },
        setting_definition<float>{ "AOEStaggerRadius", &settings::config::AOEStaggerRadius },
        setting_definition<bool>{ "includeDirectAttackerInAOEStagger", &settings::config::includeDirectAttackerInAOEStagger },
        setting_definition<bool>{ "enableNPCParry", &settings::config::enableNPCParry },
        setting_definition<bool>{ "bEnableArrowProjectileDeflection", &settings::config::bEnableArrowProjectileDeflection },
        setting_definition<bool>{ "bEnableMagicProjectileDeflection", &settings::config::bEnableMagicProjectileDeflection },
        setting_definition<bool>{ "enableRangedStagger", &settings::config::enableRangedStagger },
        setting_definition<bool>{ "enableSpellCasterStagger", &settings::config::enableSpellCasterStagger },
    };

    config Get();
    void Set(const config& value);
    void Load();
    bool Save();

    void __stdcall RenderMenuPage();

    void RegisterMenu();
}
