#pragma once

#include "form_config.h"

class hooks {
    public:
        static void Install() {
            SKSE::log::info("Installing Hooks...");
            REL::Relocation<std::uintptr_t> arrowProjectileVtbl{ RE::VTABLE_ArrowProjectile[0] };
			REL::Relocation<std::uintptr_t> missileProjectileVtbl{ RE::VTABLE_MissileProjectile[0] };

			// _arrowCollission = arrowProjectileVtbl.write_vfunc(190, OnArrowCollision);
			// _missileCollission = missileProjectileVtbl.write_vfunc(190, OnMissileCollision);
            // SKSE::log::info("Ranged Hooks installed");
			auto& trampoline = SKSE::GetTrampoline();

            {
                REL::Relocation<uintptr_t> vtblPC{RE::VTABLE_PlayerCharacter[3]};
                _original_PC_Notify = vtblPC.write_vfunc(0x1, PC_NotifyAnimationGraph);
            }

            {
                REL::Relocation<std::uintptr_t> vtblNPC{RE::VTABLE_Character[3]};
                _original_NPC_notify = vtblNPC.write_vfunc(0x1, NPC_NotifyAnimationGraph);
            }

            //SE:627930 + 38B AE:64D350 + 40A / 45A
			_ProcessHit = trampoline.write_call<5>(RELOCATION_ID(37650, 38603).address() + REL::Relocate(0x38B, 0x45A), processHit);
			SKSE::log::info("Melee Hit hook installed.");

            SKSE::log::info("Finished Installing Hooks. ");
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

        static void processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr) {
			// if (shouldIgnoreHit(a_aggressor, a_victim)) {
			// 	return;
			// }
			_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
		}

        static bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
            static auto* const player = RE::PlayerCharacter::GetSingleton();
            const bool result = _original_PC_Notify(a_this, a_eventName);
            return result;
        }

        static bool NPC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this,const RE::BSFixedString& a_eventName) {
            const bool result = _original_NPC_notify(a_this, a_eventName);
            return result;
        }
		static inline REL::Relocation<decltype(processHit)> _ProcessHit;
        static inline REL::Relocation<decltype(OnArrowCollision)> _arrowCollission;
		static inline REL::Relocation<decltype(OnMissileCollision)> _missileCollission;
        inline static REL::Relocation<decltype(PC_NotifyAnimationGraph)> _original_PC_Notify;
        inline static REL::Relocation<decltype(NPC_NotifyAnimationGraph)> _original_NPC_notify;


};
