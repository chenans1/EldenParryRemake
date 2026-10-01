#pragma once

#include <vector>

namespace form_config {
    inline constexpr auto requirementsPath = "Data/SKSE/Plugins/EldenParryForms.ini";

    struct CoreForms {
        RE::SpellItem* EP_AttackerSpell = nullptr;
        RE::SpellItem* EP_StaggerSpell = nullptr;
        RE::EffectSetting* EP_StaggerMGEF = nullptr;
        RE::SpellItem* EP_AOEStaggerSpell = nullptr;
        RE::SpellItem* EP_BasherSpell = nullptr;
        RE::BGSSoundDescriptorForm* EP_SFXShield = nullptr;
        RE::BGSSoundDescriptorForm* EP_SFXWeapon = nullptr;
        std::vector<RE::BGSExplosion*> VFXshield;
        std::vector<RE::BGSExplosion*> VFXweapon;
        std::vector<RE::BGSExplosion*> VFXelse;
    };

    struct PerkRequirements {
        RE::BGSPerk* ShieldArrowReflection = nullptr;
        RE::BGSPerk* ShieldSpellReflection = nullptr;
        RE::BGSPerk* NonShieldArrowReflection = nullptr;
        RE::BGSPerk* NonShieldSpellReflection = nullptr;
    };

    struct Config {
        CoreForms core;
        PerkRequirements perks;
    };

    // Loads and resolves every configured form. Missing files are created with elden parry defaults.
    bool Load();
    const Config& Get();
}
