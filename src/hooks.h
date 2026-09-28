#pragma once

#include "form_config.h"
#include "settings.h"
#include "utils.h"

class hooks {
    public:
        static inline RE::SpellItem* parryDelaySpell = nullptr;
        static inline RE::EffectSetting* parryDelayEffect = nullptr;
        static inline RE::SpellItem* parryWindowSpell = nullptr;
        static inline RE::EffectSetting* parryWindowEffect = nullptr;

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

            // {
            //     REL::Relocation<std::uintptr_t> vtblNPC{RE::VTABLE_Character[3]};
            //     _original_NPC_notify = vtblNPC.write_vfunc(0x1, NPC_NotifyAnimationGraph);
            // }
            // SKSE::log::info("NPC->NotifyAnimationGraph() Hooked");
            
            //process event hooks. both must be done.
            REL::Relocation<std::uintptr_t> vtblNPC{RE::VTABLE_Character[2]};
            REL::Relocation<std::uintptr_t> vtblPC{RE::VTABLE_PlayerCharacter[2]};

            _originalNPC = vtblNPC.write_vfunc(0x1, ProcessEvent_NPC);
            _originalPC = vtblPC.write_vfunc(0x1, ProcessEvent_PC);
            SKSE::log::info("Installed PC & NPC processEvent() Hooks");

            auto& trampoline = SKSE::GetTrampoline();
			_ProcessHit = trampoline.write_call<5>(RELOCATION_ID(37650, 38603).address() + REL::Relocate(0x38B, 0x45A), processHit); // SE:627930 + 38B AE:64D350 + 40A / 45A
			SKSE::log::info("Melee Hit hook installed."); 
            
            // _arrowCollission = arrowProjectileVtbl.write_vfunc(190, OnArrowCollision);
			// _missileCollission = missileProjectileVtbl.write_vfunc(190, OnMissileCollision);
            // SKSE::log::info("Ranged Hooks installed");

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
            return true;
        }

    private:
		static void OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector) {
			// if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
			// 	return;
			// };
			_arrowCollission(a_this, a_AllCdPointCollector);
		}

		static void OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector) {
			// if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
			// 	return;
			// };
			_missileCollission(a_this, a_AllCdPointCollector);
        }

        //melee collision hook. same as the original. 
        static void processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr) {
            SKSE::log::info("[processHit] func call");
			if (a_victim->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
                SKSE::log::info("[processHit] Victim bashing");
                const auto cfg = settings::Get();
                if (!utils::hasMGEF(a_victim, parryDelayEffect) && utils::hasMGEF(a_victim, parryWindowEffect)) {
                    SKSE::log::info("[processHit] Successful parry");
                    utils::ApplySpell(a_victim, a_victim, loadedForms.core.EP_BasherSpell);
                    if (a_aggressor) {
                        utils::ApplySpell(a_victim, a_aggressor, loadedForms.core.EP_AttackerSpell);
                        utils::overrideStaggerMagnitude(loadedForms.core.EP_StaggerSpell, loadedForms.core.EP_StaggerMGEF, cfg.staggerMagnitude);
                        utils::ApplySpell(a_victim, a_aggressor, loadedForms.core.EP_StaggerSpell);
                    }
                    return;
                }
            }
			_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
		}

        static bool applyParryWindow(RE::Actor* actor) {
            const auto cfg = settings::Get();
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
                SKSE::log::info("[NPC_NotifyAnimationGraph] bashRelease");
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
                SKSE::log::info("[handleEvent] bashRelease");
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

		static inline REL::Relocation<decltype(processHit)> _ProcessHit;
        static inline REL::Relocation<decltype(OnArrowCollision)> _arrowCollission;
		static inline REL::Relocation<decltype(OnMissileCollision)> _missileCollission;
        static inline REL::Relocation<decltype(PC_NotifyAnimationGraph)> _original_PC_Notify;
        static inline REL::Relocation<decltype(NPC_NotifyAnimationGraph)> _original_NPC_notify;
        static inline REL::Relocation<decltype(ProcessEvent_NPC)> _originalNPC;
        static inline REL::Relocation<decltype(ProcessEvent_PC)> _originalPC;


};
