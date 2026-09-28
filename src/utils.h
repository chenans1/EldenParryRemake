#pragma once
#include "settings.h"
#include "form_config.h"

namespace utils {
    inline static bool ApplySpell(RE::Actor* a_caster, RE::Actor* a_target, RE::SpellItem* a_spell) {
        if (!a_caster || !a_target || !a_spell) {
            return false;
        }

        if (auto* caster = a_caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
            caster->CastSpellImmediate(a_spell, false, a_target, 1.0f, false, 0.0f, a_caster);
            // if (const auto cfg = settings::Get().log) { 
            //     SKSE::log::info("[ApplySpell]: Cast spell={:08X} on target={:08X} caster={:08X}",  
            //         a_spell->GetFormID(), a_target ? a_target->GetFormID() : 0, a_caster ? a_caster->GetFormID() : 0);
            // }
            return true;
        }
        return false;
    }

    // inline static void SendTBModEvent(RE::Actor* a_defender, RE::Actor* a_attacker) {
    //     const auto attacker_ID = a_attacker ? a_attacker->GetFormID() : 0x0;
    //     const auto level = a_attacker ? a_attacker->GetLevel() : 0x0;
    //     const auto level_arg = static_cast<float>(level);

    //     const SKSE::ModCallbackEvent modEvent{ .eventName = RE::BSFixedString("STBL_OnTimedBlockDefender"), .strArg = RE::BSFixedString(std::to_string(attacker_ID)), .numArg = level_arg, .sender = a_defender };
    //     const SKSE::ModCallbackEvent modEventATK{ .eventName = RE::BSFixedString("STBL_OnTimedBlockAttacker"), .strArg = RE::BSFixedString(), .numArg = level_arg, .sender = a_attacker };
    //     SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    //     SKSE::GetModCallbackEventSource()->SendEvent(&modEventATK);
    // }

    // plays sound if it exists on the actor. adapted from DTRY's payload and spell hotbar2
    inline static RE::BSSoundHandle play_sound(RE::Actor* actor, RE::BGSSoundDescriptorForm* sound_form) {
        RE::BSSoundHandle handle;
        handle.soundID = static_cast<uint32_t>(-1);
        handle.assumeSuccess = false;
        handle.state = RE::BSSoundHandle::AssumedState::kInitialized;
        /*assumption: not doing this causes the game to crash if the audio engine is paused,
        eg: always active, mute on focus loss mod installed, tab out during casting*/
        auto audio_manager = RE::BSAudioManager::GetSingleton();
        if (audio_manager) {
            // 16 is used by payload & spellhotbar 2
            audio_manager->BuildSoundDataFromDescriptor(handle, sound_form, 16);
            if (handle.SetPosition(actor->data.location)) {
                handle.SetObjectToFollow(actor->Get3D());
                handle.Play();
            }
        }
        return handle;
    }

    // inline static void StaggerNearby(RE::Actor* a_defender, float radius) {
    //     auto* cell = a_defender ? a_defender->GetParentCell() : nullptr;
    //     if (!cell || !cell->IsAttached() || radius <= 0.0f) {
    //         return;
    //     }
    //     radius = std::min(radius, 4095.0f);
    //     cell->ForEachReferenceInRange(a_defender->GetPosition(), radius, [&](RE::TESObjectREFR* ref){
    //         auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
    //         if (!actor || actor->IsDisabled() || !actor->Is3DLoaded() || actor == a_defender) {
    //             return RE::BSContainer::ForEachResult::kContinue;
    //         }
    //         SKSE::log::info("[Utils] Staggering={:08X} ", actor ? actor->GetFormID() : 0);
    //         ApplySpell(a_defender, actor, hooks::timedBlockStaggerSpell);
    //         return RE::BSContainer::ForEachResult::kContinue;
    //     });
    // }   

    inline static bool hasMGEF(RE::Actor* actor, RE::EffectSetting* a_effect) {
        if (!actor || !a_effect) {
            return false;
        }
        auto* magicTarget = actor->GetMagicTarget();
        if (!magicTarget) {
            return false;
        }

        if (magicTarget->HasMagicEffect(a_effect)) {
            return true;
        }
        return false;
    }

    inline static RE::Effect* FindEffect(RE::SpellItem* a_spell, RE::EffectSetting* a_mgef) {
        if (!a_spell || !a_mgef) {
            return nullptr;
        }
        for (auto* effect : a_spell->effects) {
            if (effect && effect->baseEffect == a_mgef) {
                return effect;
            }
        }
        return nullptr;
    }

    //overrides the first stagger instance mgef magnitude. 0.0f means disabled.
    inline static void overrideStaggerMagnitude(RE::SpellItem* a_spell, RE::EffectSetting* a_mgef, float overrideMag) {
        if (overrideMag <= 0.0 || !a_spell || !a_mgef) {
            return;
        }
        if (auto* effect = FindEffect(a_spell, a_mgef)) {
            effect->SetMagnitude(overrideMag);
        }
    }

    //case insensitive compare for anim events
    static inline bool compare(std::string_view left, std::string_view right) {
        if (left.size() != right.size()) {
            return false;
        }
        for (std::size_t i = 0; i < left.size(); ++i) {
            const auto lower = [](char character) {
                return character >= 'A' && character <= 'Z' ? static_cast<char>(character + ('a' - 'A')) : character;
            };
            if (lower(left[i]) != lower(right[i])) {
                return false;
            }
        }
        return true;
    }

    static inline bool applyMGEFDuration(RE::EffectSetting* a_effect, float duration) {
        if (!a_effect) {
            return false;
        }
        a_effect->data.taperDuration = std::clamp(duration, 0.0f, 1.0f);
        return true;
    }

    static inline bool isShield(const RE::Actor* actor) {
        if (!actor) {
            return false;
        }

        const auto* leftHand = actor->GetEquippedObject(true);
        const auto* armor = leftHand ? leftHand->As<RE::TESObjectARMO>() : nullptr;
        return armor && armor->IsShield();
    }

    static inline bool isPowerAttacking(RE::Actor* a_actor) {
		if (a_actor->GetActorRuntimeData().currentProcess && a_actor->GetActorRuntimeData().currentProcess->high) {
			auto atkData = a_actor->GetActorRuntimeData().currentProcess->high->attackData.get();
			if (atkData) {
				return atkData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack);
			}
		}
		return false;
	}

}
