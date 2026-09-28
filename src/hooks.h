#pragma once

#include "form_config.h"
#include "settings.h"
#include "utils.h"
#include "extern/PrecisionAPI.h"


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
            
            //process event hooks. both must be done.
            REL::Relocation<std::uintptr_t> vtblNPC{RE::VTABLE_Character[2]};
            REL::Relocation<std::uintptr_t> vtblPC{RE::VTABLE_PlayerCharacter[2]};

            _originalNPC = vtblNPC.write_vfunc(0x1, ProcessEvent_NPC);
            _originalPC = vtblPC.write_vfunc(0x1, ProcessEvent_PC);
            SKSE::log::info("Installed PC & NPC processEvent() Hooks");

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

        //melee
        static bool attemptParry(RE::Actor* attacker, RE::Actor* victim) {
            if (!attacker || !victim) return false;
            const auto cfg = settings::Get();
            if (!parryStyleEnabled(victim, cfg)) return false;
            if (!victim->IsPlayerRef() && !cfg.enableNPCParry) return false;
            const auto attackerState = attacker->AsActorState()->GetAttackState();
            const auto victimState = victim->AsActorState()->GetAttackState();
            // const bool attackerSwinging = attackerState == RE::ATTACK_STATE_ENUM::kSwing || attackerState == RE::ATTACK_STATE_ENUM::kHit;
            // const bool attackerSwinging = attackerState == RE::ATTACK_STATE_ENUM::kSwing;
            const bool attackerSwinging = attackerState <= RE::ATTACK_STATE_ENUM::kHit;
            const bool victimBashing = victimState == RE::ATTACK_STATE_ENUM::kBash;
            if (!cfg.enablePowerBashParry) {
                if (victim->IsPowerAttacking()) return false;
            }
            if (cfg.log) {
                SKSE::log::info("[attemptParry] attackerState={} victimState={}",
                    static_cast<std::uint32_t>(attackerState), static_cast<std::uint32_t>(victimState));
            }
            if (victimBashing && attackerSwinging) {
                const bool hasDelay = utils::hasMGEF(victim, parryDelayEffect);
                const bool hasWindow = utils::hasMGEF(victim, parryWindowEffect);
                if (cfg.log) {
                    SKSE::log::info("[attemptParry] attacker={:08X} victim={:08X} delayEffect={} windowEffect={}",
                        attacker->GetFormID(), victim->GetFormID(), hasDelay, hasWindow);
                }
                if (!hasDelay && hasWindow) {
                    if (cfg.log) SKSE::log::info("[attemptParry] Successful parry");
                    playParryEffects(victim);
                    if (attacker) {
                        utils::ApplySpell(victim, attacker, loadedForms.core.EP_AttackerSpell);
                        utils::overrideStaggerMagnitude(loadedForms.core.EP_StaggerSpell, loadedForms.core.EP_StaggerMGEF, cfg.staggerMagnitude);
                        utils::ApplySpell(victim, attacker, loadedForms.core.EP_StaggerSpell);
                    }
                    return true;
                }
            }
            return false;
        }

        static PRECISION_API::PreHitCallbackReturn OnPrecisionPreHit(const PRECISION_API::PrecisionHitData& hit) {
            PRECISION_API::PreHitCallbackReturn result{};
            const auto cfg = settings::Get();
            if (!hit.attacker) {
                return result;
            }
            //remove attacker prec hitframe
            const auto attackerState = hit.attacker->AsActorState()->GetAttackState();

            if (attackerState == RE::ATTACK_STATE_ENUM::kBash) {
                if (!parryStyleEnabled(hit.attacker, cfg)) return result;
                if (!cfg.enablePowerBashParry) {
                    if (hit.attacker->IsPowerAttacking()) return result;
                }
                if (!hit.attacker->IsPlayerRef() && !cfg.enableNPCParry){
                    return result;
                }
                if (cfg.log) SKSE::log::info("[Precision pre-hit] Ignoring bash hit from {:08X}", hit.attacker->GetFormID());
                result.bIgnoreHit = true;
                return result;
            }
            auto* victim = hit.target ? hit.target->As<RE::Actor>() : nullptr;
            if (!victim) {
                return result;
            }
            if (cfg.log) SKSE::log::info("[Precision pre-hit] attacker={:08X} victim={:08X}", hit.attacker->GetFormID(), victim->GetFormID());

            if (!victim->IsPlayerRef() && !cfg.enableNPCParry){
                return result;
            }
            if (attemptParry(hit.attacker, victim)) {
                result.bIgnoreHit = true;
            }
            return result;
        }

        //melee collision hook. same as the original. 
        static void processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr) {
            const auto cfg = settings::Get();
            if (a_aggressor && a_victim && cfg.log) {
                SKSE::log::info("[processHit] attacker={:08X} victim={:08X}",
                    a_aggressor->GetFormID(), a_victim->GetFormID());
            }
            //remove aggressor bash hitframe
            if (a_aggressor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
                if (!parryStyleEnabled(a_aggressor, cfg) ||
                    (!a_aggressor->IsPlayerRef() && !cfg.enableNPCParry) ||
                    (!cfg.enablePowerBashParry && a_aggressor->IsPowerAttacking())) {
                    return _ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
                }
                if (cfg.log) SKSE::log::info("[processHit] a_aggressor bashing hitframe cancel");
                return;
            }
            //doesn't fire off with precision installed
			if (attemptParry(a_aggressor, a_victim)) {
                if (cfg.log) SKSE::log::info("[processHit] Successful Parry");
                return;
            }
			_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
		}

        static bool applyParryWindow(RE::Actor* actor) {
            if (!actor) {
                return false;
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
            static const RE::BSFixedString bashRelease{ "bashRelease" }; 
            static auto* const player = RE::PlayerCharacter::GetSingleton();

            if (a_eventName == bashRelease) {
                SKSE::log::info("[PC_NotifyAnimationGraph] bashRelease");
                applyParryWindow(player);
            }
            return result;
        }

        static bool NPC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
            const bool result = _original_NPC_notify(a_this, a_eventName);
            static const RE::BSFixedString bashRelease{ "bashRelease" }; 
            // auto* refr = static_cast<RE::TESObjectREFR*>(a_this);
            auto* refr = SKSE::stl::adjust_pointer<RE::TESObjectREFR>(a_this, -0x38);
            auto* actor = refr ? refr->As<RE::Actor>() : nullptr;
            if (!actor) {
                return result;
            }
            if (a_eventName == bashRelease) {
                // SKSE::log::info("[NPC_NotifyAnimationGraph] bashRelease");
                applyParryWindow(actor);
            }
            return result;
        }

        static void handleEvent(const RE::BSAnimationGraphEvent* a_event) {
            static const  std::string_view bashRelease{ "bashRelease" }; 
            if (!a_event || !a_event->holder || !a_event->tag.data()) return;
            auto* holder = const_cast<RE::TESObjectREFR*>(a_event->holder);
            if (!holder) return;
            auto* actor = holder ? holder->As<RE::Actor>() : nullptr;
            if (!actor) return;
            const auto& tag = a_event->tag;

            if (utils::compare(bashRelease, tag)) {
                // if SKSE::log::info("[handleEvent] bashRelease");
                applyParryWindow(actor);
            }
        }

        static RE::BSEventNotifyControl ProcessEvent_NPC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
            handleEvent(a_event);
            return _originalNPC(a_sink, a_event, a_eventSource);
        }

        static RE::BSEventNotifyControl ProcessEvent_PC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
            handleEvent(a_event);
            return _originalPC(a_sink, a_event, a_eventSource);
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

        static bool processProjectileParry(RE::Actor* a_parrier, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
            if (canParryProjectile(a_parrier, a_projectile)) {
                RE::TESObjectREFR* shooter = nullptr;
                if (a_projectile->GetProjectileRuntimeData().shooter && a_projectile->GetProjectileRuntimeData().shooter.get()) {
                    shooter = a_projectile->GetProjectileRuntimeData().shooter.get().get();
                }

                utils::resetProjectileOwner(a_projectile, a_parrier, a_projectile_collidable);

                if (shooter && shooter->Is3DLoaded()) {
                    utils::RetargetProjectile(a_projectile, shooter);
                } else {
                    utils::ReflectProjectile(a_projectile);
                }
                
                playParryEffects(a_parrier);
                // if (a_parrier->IsPlayerRef()) {
                //     RE::PlayerCharacter::GetSingleton()->AddSkillExperience(RE::ActorValue::kBlock, Settings::fProjectileParryExp);
                // }
                // if (Settings::bSuccessfulParryNoCost) {
                //     negateParryCost(a_parrier);
                // }
                // send_ranged_parry_event();
                return true;
            }
            return false;

        }

        //taken from: https://github.com/doodlum/EldenParry/blob/main/src/Hooks.h
        static bool shouldIgnoreHit(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector) {
			if (a_AllCdPointCollector) {
                const auto cfg = settings::Get();
				const bool deflectionEnabled = a_projectile->GetProjectileRuntimeData().spell
				    ? cfg.bEnableMagicProjectileDeflection
				    : (a_projectile->GetFormType() == RE::FormType::ProjectileArrow && cfg.bEnableArrowProjectileDeflection);
				for (auto& hit : a_AllCdPointCollector->hits) {
					auto refrA = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableA);
					auto refrB = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableB);
					if (refrA && refrA->formType == RE::FormType::ActorCharacter && refrA->As<RE::Actor>()->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
						if (refrA->IsPlayerRef() || cfg.enableNPCParry) {
							if (deflectionEnabled) {
								return processProjectileParry(refrA->As<RE::Actor>(), a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableB));
							}
						}
					}
					if (refrB && refrB->formType == RE::FormType::ActorCharacter && refrB->As<RE::Actor>()->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
						if (refrB->IsPlayerRef() || cfg.enableNPCParry) {
							if (deflectionEnabled) {
								return processProjectileParry(refrB->As<RE::Actor>(), a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableA));
							}
						}
					}
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
        static inline REL::Relocation<decltype(OnArrowCollision)> _arrowCollission;
		static inline REL::Relocation<decltype(OnMissileCollision)> _missileCollission;
        static inline REL::Relocation<decltype(PC_NotifyAnimationGraph)> _original_PC_Notify;
        static inline REL::Relocation<decltype(NPC_NotifyAnimationGraph)> _original_NPC_notify;
        static inline REL::Relocation<decltype(ProcessEvent_NPC)> _originalNPC;
        static inline REL::Relocation<decltype(ProcessEvent_PC)> _originalPC;


};
