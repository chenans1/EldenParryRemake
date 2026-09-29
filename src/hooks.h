#pragma once

#include "form_config.h"
#include "settings.h"
#include "utils.h"
#include "extern/PrecisionAPI.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>


class hooks {
    public:
        static inline RE::SpellItem* parryDelaySpell = nullptr;
        static inline RE::EffectSetting* parryDelayEffect = nullptr;
        static inline RE::SpellItem* parryWindowSpell = nullptr;
        static inline RE::EffectSetting* parryWindowEffect = nullptr;
        static inline float _GMST_fCombatHitConeAngle = 45.0f;
        inline static form_config::Config loadedForms{};

        static void Install() {
            SKSE::log::info("Installing Hooks...");
            REL::Relocation<std::uintptr_t> arrowProjectileVtbl{ RE::VTABLE_ArrowProjectile[0] };
			REL::Relocation<std::uintptr_t> missileProjectileVtbl{ RE::VTABLE_MissileProjectile[0] }; 
			
            //notification hooks
            {
                REL::Relocation<uintptr_t> vtblPC{RE::VTABLE_PlayerCharacter[3]};
                _original_PC_Notify = vtblPC.write_vfunc(0x1, PC_NotifyAnimationGraph);
            }
            SKSE::log::info("Player->NotifyAnimationGraph() Hooked");

            {
                REL::Relocation<std::uintptr_t> vtblNPC{RE::VTABLE_Character[3]};
                _original_NPC_notify = vtblNPC.write_vfunc(0x1, NPC_NotifyAnimationGraph);
            }
            SKSE::log::info("NPC->NotifyAnimationGraph() Hooked");

            auto& trampoline = SKSE::GetTrampoline();
			_ProcessHit = trampoline.write_call<5>(RELOCATION_ID(37650, 38603).address() + REL::Relocate(0x38B, 0x45A), processHit); // SE:627930 + 38B AE:64D350 + 40A / 45A
			SKSE::log::info("Melee Hit hook installed."); 
            
            _arrowCollission = arrowProjectileVtbl.write_vfunc(190, OnArrowCollision);
			_missileCollission = missileProjectileVtbl.write_vfunc(190, OnMissileCollision);
            SKSE::log::info("Ranged Hooks installed");
            auto* api = static_cast<PRECISION_API::IVPrecision1*>(PRECISION_API::RequestPluginAPI(PRECISION_API::InterfaceVersion::V4));

            if (api) {
                const auto status = api->AddPreHitCallback(SKSE::GetPluginHandle(), OnPrecisionPreHit);
                SKSE::log::info("Precision pre-hit registration: {}", status == PRECISION_API::APIResult::OK);
            }
            SKSE::log::info("Finished Installing Hooks.");
        }

        static bool Load() {
            if (!form_config::Load()) {
                // SKSE::log::info("Finished Installing Hooks.");
                return false;
            }
            auto* dataHandler = RE::TESDataHandler::GetSingleton();
            // const auto& configured = form_config::Get().core;
            parryDelaySpell = dataHandler->LookupForm<RE::SpellItem>(0x808, "EldenParryRemake.esp");
            parryDelayEffect = dataHandler->LookupForm<RE::EffectSetting>(0x80A, "EldenParryRemake.esp");
            parryWindowSpell = dataHandler->LookupForm<RE::SpellItem>(0x809, "EldenParryRemake.esp");
            parryWindowEffect = dataHandler->LookupForm<RE::EffectSetting>(0x80B, "EldenParryRemake.esp");

            if (!parryDelaySpell|| !parryDelayEffect || !parryWindowSpell || !parryWindowEffect) {
                SKSE::log::error("Failed to load effect forms: parryDelaySpell={}, parryDelayEffect={}, parryWindowSpell={}, parryWindowEffect={}", 
                    static_cast<void*>(parryDelaySpell), 
                    static_cast<void*>(parryDelayEffect), 
                    static_cast<void*>(parryWindowSpell),
                    static_cast<void*>(parryWindowEffect));
                return false;
            }
            SKSE::log::info("Correctly loaded forms: parryDelaySpell={:08X}, parryDelayEffect={:08X}, parryWindowSpell={:08X}, parryWindowEffect={:08X}", 
                    parryDelaySpell->GetFormID(), parryDelayEffect->GetFormID(), parryWindowSpell->GetFormID(), parryWindowEffect->GetFormID());
            loadedForms = form_config::Get();
            if (auto* settings = RE::GameSettingCollection::GetSingleton()) {
                if (auto* setting = settings->GetSetting("fCombatHitConeAngle")) {
                    _GMST_fCombatHitConeAngle = setting->GetFloat();
                }
            }
            return true;
        }

    private:
        static bool parryStyleEnabled(const RE::Actor* actor, const settings::config& cfg) {
            return utils::isShield(actor) ? cfg.bShieldEnabled : cfg.bNonShieldEnabled;
        }

        static void playParryEffects(RE::Actor* actor) {
            utils::ApplySpell(actor, actor, loadedForms.core.EP_BasherSpell);
            if (utils::isShield(actor)) {
                utils::play_sound(actor, loadedForms.core.EP_SFXWeapon);
            } else {
                utils::play_sound(actor, loadedForms.core.EP_SFXWeapon);
            }
        }

        // Check the current collision without applying any effects.
        static bool canParry(RE::Actor* attacker, RE::Actor* defender) {
            if (!attacker || !defender) return false;
            const auto cfg = settings::Get();
            if (!parryStyleEnabled(defender, cfg)) return false;
            if (!defender->IsPlayerRef() && !cfg.enableNPCParry) return false;
            const auto attackerState = attacker->AsActorState()->GetAttackState();
            const auto defenderState = defender->AsActorState()->GetAttackState();
            // const bool attackerSwinging = attackerState == RE::ATTACK_STATE_ENUM::kSwing || attackerState == RE::ATTACK_STATE_ENUM::kHit;
            const bool attackerSwinging = attackerState == RE::ATTACK_STATE_ENUM::kSwing;
            const bool defenderBashing = defenderState == RE::ATTACK_STATE_ENUM::kBash;
            if (!cfg.enablePowerBashParry && defender->IsPowerAttacking()) return false;
            if (cfg.log) {
                SKSE::log::info("[canParry] attackerState={} defenderState={}",
                    static_cast<std::uint32_t>(attackerState), static_cast<std::uint32_t>(defenderState));
            }
            if (defenderBashing && attackerSwinging) {
                const bool hasDelay = utils::hasMGEF(defender, parryDelayEffect);
                const bool hasWindow = utils::hasMGEF(defender, parryWindowEffect);
                if (cfg.log) {
                    SKSE::log::info("[canParry] attacker={:08X} defender={:08X} delayEffect={} windowEffect={}",
                        attacker->GetFormID(), defender->GetFormID(), hasDelay, hasWindow);
                }
                return !hasDelay && hasWindow;
            }
            return false;
        }

        static void applyParryEffects(RE::Actor* defender, RE::Actor* attacker) {
            const auto cfg = settings::Get();
            if (cfg.log) SKSE::log::info("[parry] effects defender={:08X} attacker={:08X}", defender->GetFormID(), attacker->GetFormID());
            playParryEffects(defender);
            utils::ApplySpell(defender, attacker, loadedForms.core.EP_AttackerSpell);
            if (cfg.enableAOEStagger) {
                auto* excluded = cfg.includeDirectAttackerInAOEStagger ? nullptr : attacker;
                utils::StaggerNearby(defender, cfg.AOEStaggerRadius, loadedForms.core.EP_AOEStaggerSpell, excluded);
            }
            if (cfg.enableSingleTargetStagger) {
                utils::overrideStaggerMagnitude(loadedForms.core.EP_StaggerSpell, loadedForms.core.EP_StaggerMGEF, cfg.staggerMagnitude);
                utils::ApplySpell(defender, attacker, loadedForms.core.EP_StaggerSpell);
            }
            utils::sendMeleeEvent(attacker);
        }

        static void queueParryEffects(RE::Actor* defender, RE::Actor* attacker) {
            const auto defenderHandle = defender->GetHandle();
            const auto attackerHandle = attacker->GetHandle();
            if (auto* tasks = SKSE::GetTaskInterface()) {
                tasks->AddTask([defenderHandle, attackerHandle]() {
                    auto resolvedDefender = defenderHandle.get();
                    auto resolvedAttacker = attackerHandle.get();
                    if (resolvedDefender && resolvedAttacker) {
                        applyParryEffects(resolvedDefender.get(), resolvedAttacker.get());
                    }
                });
            } else {
                SKSE::log::error("[parry] task interface unavailable; effects were not applied");
            }
        }

        // Both directional collisions can arrive in either order. Remember a successful
        // pair for this bash so only the attacker hit is suppressed and effects run once.
        static bool resolveParry(RE::Actor* attacker, RE::Actor* defender) {
            if (!attacker || !defender || defender->AsActorState()->GetAttackState() != RE::ATTACK_STATE_ENUM::kBash) {
                return false;
            }

            bool firstCollision = false;
            {
                std::lock_guard lock(_parryHitsMutex);
                const auto defenderID = defender->GetFormID();
                const auto attackerID = attacker->GetFormID();
                auto it = _parriedAttackers.find(defenderID);
                if (it != _parriedAttackers.end() && it->second.contains(attackerID)) {
                    return true;
                }
                if (!canParry(attacker, defender)) {
                    return false;
                }
                _parriedAttackers[defenderID].insert(attackerID);
                firstCollision = true;
            }

            if (firstCollision) {
                queueParryEffects(defender, attacker);
            }
            return true;
        }

        static PRECISION_API::PreHitCallbackReturn OnPrecisionPreHit(const PRECISION_API::PrecisionHitData& hit) {
            PRECISION_API::PreHitCallbackReturn result{};
            const auto cfg = settings::Get();
            if (!hit.attacker) {
                return result;
            }
            //remove attacker prec hitframe
            const auto attackerState = hit.attacker->AsActorState()->GetAttackState();
            auto* victim = hit.target ? hit.target->As<RE::Actor>() : nullptr;
            if (!victim) {
                return result;
            }
            if (attackerState == RE::ATTACK_STATE_ENUM::kBash) {
                if (!parryStyleEnabled(hit.attacker, cfg)) return result;
                if (!cfg.enablePowerBashParry) {
                    if (hit.attacker->IsPowerAttacking()) return result;
                }
                if (!hit.attacker->IsPlayerRef() && !cfg.enableNPCParry){
                    return result;
                }
                if (resolveParry(victim, hit.attacker)) {
                    if (cfg.log) SKSE::log::info("[Precision pre-hit] allowing parry bash hit attacker={:08X} target={:08X}", hit.attacker->GetFormID(), victim->GetFormID());
                } else {
                    result.bIgnoreHit = true;
                    if (cfg.log) SKSE::log::info("[Precision pre-hit] ignoring bash hit attacker={:08X} target={:08X}", hit.attacker->GetFormID(), victim->GetFormID());
                }
                return result;
            }

            if (cfg.log) SKSE::log::info("[Precision pre-hit] attacker={:08X} victim={:08X}", hit.attacker->GetFormID(), victim->GetFormID());

            if (!victim->IsPlayerRef() && !cfg.enableNPCParry){
                return result;
            }
            if (resolveParry(hit.attacker, victim)) {
                result.bIgnoreHit = true;
                if (cfg.log) SKSE::log::info("[Precision pre-hit] ignoring parried attack attacker={:08X} defender={:08X}", hit.attacker->GetFormID(), victim->GetFormID());
            }
            return result;
        }

        //melee collision hook. same as the original. 
        static void processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr) {
            if (!a_aggressor || !a_victim) {
                return _ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
            }
            const auto cfg = settings::Get();
            if (a_aggressor && a_victim && cfg.log) {
                SKSE::log::info("[processHit] attacker={:08X} victim={:08X}",
                    a_aggressor->GetFormID(), a_victim->GetFormID());
            }
            // Apply the same directional decision when Precision does not handle the hit.
            if (a_aggressor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
                if (!parryStyleEnabled(a_aggressor, cfg) ||
                    (!a_aggressor->IsPlayerRef() && !cfg.enableNPCParry) ||
                    (!cfg.enablePowerBashParry && a_aggressor->IsPowerAttacking())) {
                    return _ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
                }
                if (!resolveParry(a_victim, a_aggressor)) {
                    if (cfg.log) SKSE::log::info("[processHit] ignoring bash hit attacker={:08X} target={:08X}", a_aggressor->GetFormID(), a_victim->GetFormID());
                    return;
                }
                if (cfg.log) SKSE::log::info("[processHit] allowing parry bash hit attacker={:08X} target={:08X}", a_aggressor->GetFormID(), a_victim->GetFormID());
                return _ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
            }
            if (resolveParry(a_aggressor, a_victim)) {
                if (cfg.log) SKSE::log::info("[processHit] ignoring parried attack attacker={:08X} defender={:08X}", a_aggressor->GetFormID(), a_victim->GetFormID());
                return;
            }
			_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
		}

        static bool applyParryWindow(RE::Actor* actor) {
            if (!actor) {
                return false;
            }
            {
                std::lock_guard lock(_parryHitsMutex);
                _parriedAttackers.erase(actor->GetFormID());
            }
            const auto cfg = settings::Get();
            if (!parryStyleEnabled(actor, cfg)) {
                return false;
            }
            if (!actor->IsPlayerRef() && !cfg.enableNPCParry) {
                return false;
            }

            utils::applyMGEFDuration(parryDelayEffect, cfg.delay);
            utils::applyMGEFDuration(parryWindowEffect, cfg.window);
            utils::ApplySpell(actor, actor, parryDelaySpell);
            utils::ApplySpell(actor, actor, parryWindowSpell);
            return true;
        }

        static bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
            const bool result = _original_PC_Notify(a_this, a_eventName);
            static const RE::BSFixedString bashStart{ "bashStart" };
            static auto* const player = RE::PlayerCharacter::GetSingleton();

            if (a_eventName == bashStart) {
                if (settings::Get().log) SKSE::log::info("[PC_NotifyAnimationGraph] bashStart actor={:08X}", player->GetFormID());
                applyParryWindow(player);
            } 
            return result;
        }

        static bool NPC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
            const bool result = _original_NPC_notify(a_this, a_eventName);
            static const RE::BSFixedString bashStart{ "bashStart" };
            // auto* refr = static_cast<RE::TESObjectREFR*>(a_this);
            auto* refr = SKSE::stl::adjust_pointer<RE::TESObjectREFR>(a_this, -0x38);
            auto* actor = refr ? refr->As<RE::Actor>() : nullptr;
            if (!actor) {
                return result;
            }
            if (a_eventName == bashStart) {
                if (settings::Get().log) SKSE::log::info("[NPC_NotifyAnimationGraph] bashStart actor={:08X}", actor->GetFormID());
                applyParryWindow(actor);
            } 
            return result;
        }

        //adapted from https://github.com/doodlum/EldenParry/blob/f6bd72eed354a54058805694f7adaecf24a8e0fa/src/EldenParry.cpp
        static bool canParryProjectile(RE::Actor* a_parrier, RE::TESObjectREFR* a_obj) {
            const auto cfg = settings::Get();
            if (!parryStyleEnabled(a_parrier, cfg)) {
                return false;
            }
            if (cfg.log) {
                SKSE::log::info("[canParryProjectile] {} attempting to parry projectile", a_parrier->GetName());
            }

            auto angle = a_parrier->GetHeadingAngle(a_obj->GetPosition(), false);
	        const bool inBlockAngle = (angle <= _GMST_fCombatHitConeAngle && angle >= -_GMST_fCombatHitConeAngle);

            const auto parryState = a_parrier->AsActorState()->GetAttackState();
            const bool isParrying = !utils::hasMGEF(a_parrier, parryDelayEffect) && utils::hasMGEF(a_parrier, parryWindowEffect);
            if ((!cfg.enablePowerBashParry && a_parrier->IsPowerAttacking()) || parryState != RE::ATTACK_STATE_ENUM::kBash) {
                return false;
            }
            return isParrying && inBlockAngle;
        }

        static void queueProjectileStagger(RE::Actor* a_parrier, RE::Actor* a_shooter, float a_magnitude) {
            const auto parrierHandle = a_parrier->GetHandle();
            const auto shooterHandle = a_shooter->GetHandle();
            if (auto* tasks = SKSE::GetTaskInterface()) {
                tasks->AddTask([parrierHandle, shooterHandle, a_magnitude]() {
                    auto parrier = parrierHandle.get();
                    auto shooter = shooterHandle.get();
                    if (!parrier || !shooter) {
                        return;
                    }
                    utils::overrideStaggerMagnitude(loadedForms.core.EP_StaggerSpell, loadedForms.core.EP_StaggerMGEF, a_magnitude);
                    if (utils::ApplySpell(parrier.get(), shooter.get(), loadedForms.core.EP_StaggerSpell) && settings::Get().log) {
                        SKSE::log::info("[projectile parry] staggered shooter={:08X} parrier={:08X}", shooter->GetFormID(), parrier->GetFormID());
                    }
                });
            } else {
                SKSE::log::error("[projectile parry] task interface unavailable; shooter stagger was not applied");
            }
        }

        static void queueRangedParryEvent() {
            if (auto* tasks = SKSE::GetTaskInterface()) {
                tasks->AddTask([]() { utils::sendRangedEvent(); });
            } else {
                SKSE::log::error("[projectile parry] task interface unavailable; ranged event was not sent");
            }
        }

        static bool processProjectileParry(RE::Actor* a_parrier, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable,
            bool a_reflect, bool a_stagger, float a_staggerMagnitude) {
            if (canParryProjectile(a_parrier, a_projectile)) {
                auto shooter = a_projectile->GetProjectileRuntimeData().shooter.get();
                auto* shooterActor = shooter ? shooter->As<RE::Actor>() : nullptr;
                if (a_stagger && shooterActor && shooterActor != a_parrier) {
                    queueProjectileStagger(a_parrier, shooterActor, a_staggerMagnitude);
                }

                if (a_reflect) {
                    utils::resetProjectileOwner(a_projectile, a_parrier, a_projectile_collidable);
                    if (shooter && shooter->Is3DLoaded()) {
                        utils::RetargetProjectile(a_projectile, shooter.get());
                    } else {
                        utils::ReflectProjectile(a_projectile);
                    }
                }

                playParryEffects(a_parrier);
                queueRangedParryEvent();
                if (!a_reflect) {
                    a_projectile->Kill();
                }
                if (settings::Get().log) {
                    SKSE::log::info("[projectile parry] parrier={:08X} shooter={:08X} reflected={} staggerEnabled={} damageCancelled=true",
                        a_parrier->GetFormID(), shooterActor ? shooterActor->GetFormID() : 0, a_reflect, a_stagger);
                }
                return true;
            }
            return false;
        }

        //taken from: https://github.com/doodlum/EldenParry/blob/main/src/Hooks.h
        static bool shouldIgnoreHit(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector) {
            if (!a_projectile || !a_AllCdPointCollector) {
                return false;
            }

            const auto cfg = settings::Get();
            const bool spellProjectile = a_projectile->GetProjectileRuntimeData().spell != nullptr;
            const bool arrowProjectile = !spellProjectile && a_projectile->GetFormType() == RE::FormType::ProjectileArrow;
            if (!spellProjectile && !arrowProjectile) {
                return false;
            }

            const bool reflect = spellProjectile ? cfg.bEnableMagicProjectileDeflection : cfg.bEnableArrowProjectileDeflection;
            const bool stagger = spellProjectile ? cfg.enableSpellCasterStagger : cfg.enableRangedStagger;
            if (!reflect && !stagger) {
                return false;
            }

            for (auto& hit : a_AllCdPointCollector->hits) {
                if (!hit.rootCollidableA || !hit.rootCollidableB) {
                    continue;
                }
                auto* refrA = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableA);
                auto* refrB = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableB);
                auto tryParry = [&](RE::TESObjectREFR* a_ref, RE::hkpCollidable* a_projectileCollidable) {
                    auto* actor = a_ref ? a_ref->As<RE::Actor>() : nullptr;
                    if (!actor || actor->AsActorState()->GetAttackState() != RE::ATTACK_STATE_ENUM::kBash ||
                        (!actor->IsPlayerRef() && !cfg.enableNPCParry)) {
                        return false;
                    }
                    return processProjectileParry(actor, a_projectile, a_projectileCollidable, reflect, stagger,
                        cfg.staggerMagnitude);
                };
                if (tryParry(refrA, const_cast<RE::hkpCollidable*>(hit.rootCollidableB)) ||
                    tryParry(refrB, const_cast<RE::hkpCollidable*>(hit.rootCollidableA))) {
                    return true;
                }
            }
            return false;
		}

        static void OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector) {
			if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
				return;
			}
			_arrowCollission(a_this, a_AllCdPointCollector);
		}

		static void OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector) {
			if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
				return;
			}
			_missileCollission(a_this, a_AllCdPointCollector);
        }

		static inline REL::Relocation<decltype(processHit)> _ProcessHit;
        static inline std::mutex _parryHitsMutex;
        static inline std::unordered_map<RE::FormID, std::unordered_set<RE::FormID>> _parriedAttackers;
        static inline REL::Relocation<decltype(OnArrowCollision)> _arrowCollission;
		static inline REL::Relocation<decltype(OnMissileCollision)> _missileCollission;
        static inline REL::Relocation<decltype(PC_NotifyAnimationGraph)> _original_PC_Notify;
        static inline REL::Relocation<decltype(NPC_NotifyAnimationGraph)> _original_NPC_notify;
};
