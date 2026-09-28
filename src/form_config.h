#pragma once

namespace form_config {
    inline constexpr auto requirementsPath = "Data/SKSE/Plugins/EldenParryForms.ini";

    struct PerkRequirement {
        RE::BGSPerk* perk = nullptr;
        bool configured = false;

        bool IsMetBy(const RE::Actor* actor) const {
            if (!configured) {
                // SKSE::log::info("[IsMetBy] unrestricted");
                return true;
            }

            const bool hasPerk = actor && perk && actor->HasPerk(perk);
            // SKSE::log::info("[IsMetBy] required perk {:08X}, hasPerk={}", perk ? perk->GetFormID() : 0, hasPerk);
            return hasPerk;
        }
    };

    struct CoreForms {
        RE::SpellItem* EP_Spell = nullptr;
        RE::SpellItem* EP_StaggerSpell = nullptr;
        RE::SpellItem* EP_AOEStaggerSpell = nullptr;
        RE::SpellItem* EP_BuffSpell = nullptr;
        RE::BGSSoundDescriptorForm* SFXshield = nullptr;
        RE::BGSSoundDescriptorForm* SFXweapon = nullptr;
        //todo: convert into lists of vfx instead.
        // RE::BGSExplosion* VFXshield = nullptr;
        // RE::BGSExplosion* VFXweapon = nullptr;


    };

    struct Config {
        CoreForms core;
    };

    // Loads and resolves every configured form. Missing files are created with elden parry defaults.
    bool Load();
    const Config& Get();
}
