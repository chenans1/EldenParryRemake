#pragma once

namespace form_config {
    inline constexpr auto requirementsPath = "Data/SKSE/Plugins/EldenParryForms.ini";

    struct CoreForms {
        RE::SpellItem* EP_AttackerSpell = nullptr;
        RE::SpellItem* EP_StaggerSpell = nullptr;
        RE::SpellItem* EP_AOEStaggerSpell = nullptr;
        RE::SpellItem* EP_BasherSpell = nullptr;
        RE::BGSSoundDescriptorForm* EP_SFXShield = nullptr;
        RE::BGSSoundDescriptorForm* EP_SFXWeapon = nullptr;
        //todo: convert into lists of vfx instead.
        // RE::BGSExplosion* VFXshield = nullptr;
        // RE::BGSExplosion* VFXweapon = nullptr;
    };

    struct Config {
        CoreForms core;
        //PerkRequirements
    };

    // Loads and resolves every configured form. Missing files are created with elden parry defaults.
    bool Load();
    const Config& Get();
}
