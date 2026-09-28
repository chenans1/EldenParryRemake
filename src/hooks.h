#pragma once

#include "form_config.h"

class hooks {
    public:
        //gameplay effect carrier spells
        static inline RE::SpellItem* EP_AttackerSpell = nullptr;
        static inline RE::SpellItem* EP_StaggerSpell = nullptr;
        static inline RE::SpellItem* EP_AOEStaggerSpell = nullptr;
        static inline RE::SpellItem* EP_BasherSpell = nullptr;

        static inline RE::BGSSoundDescriptorForm* EP_SFXshield = nullptr;
        static inline RE::BGSSoundDescriptorForm* EP_SFXweapon = nullptr;

        static void Install() {
            SKSE::log::info("Installing Hooks...");

            SKSE::log::info("Finished Installing Hooks. ");
        }

        static bool LoadForms() {
            if (!form_config::Load()) {
                return false;
            }

            const auto& forms = form_config::Get().core;

            struct NamedForm {
                const char* name;
                const RE::TESForm* form;
            };
            const NamedForm coreForms[] = {
                { "EP_AttackerSpell", forms.EP_Spell },
                { "EP_StaggerSpell", forms.EP_StaggerSpell },
                { "EP_AOEStaggerSpell", forms.EP_AOEStaggerSpell },
                { "EP_BasherSpell", forms.EP_BuffSpell },
                { "EP_SFXShield", forms.SFXshield },
                { "EP_SFXWeapon", forms.SFXweapon }
            };

            bool allLoaded = true;
            for (const auto& [name, form] : coreForms) {
                if (form) {
                    SKSE::log::info("Correctly loaded core form: {}={:08X}", name, form->GetFormID());
                } else {
                    SKSE::log::error("Failed to load core form: {}={:08X}", name, 0);
                    allLoaded = false;
                }
            }

            if (!allLoaded) {
                return false;
            }

            EP_AttackerSpell = forms.EP_Spell;
            EP_StaggerSpell = forms.EP_StaggerSpell;
            EP_AOEStaggerSpell = forms.EP_AOEStaggerSpell;
            EP_BasherSpell = forms.EP_BuffSpell;
            EP_SFXshield = forms.SFXshield;
            EP_SFXweapon = forms.SFXweapon;
            return true;
        }

    private:


};
