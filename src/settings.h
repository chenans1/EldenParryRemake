#pragma once

namespace settings {
    struct config {
        bool log = true;

        //bashing is valid if (!delay and window)
        float delay = 0.0f; 
        float window = 0.3f; 

        bool enableAOEStagger = false;

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
    };

    config Get();
    void Set(const config& value);
    void Load();
    bool Save();

    void __stdcall RenderMenuPage();

    void RegisterMenu();
}
