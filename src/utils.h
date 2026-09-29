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

    inline static void sendMeleeEvent(RE::Actor* a_attacker) {
        SKSE::ModCallbackEvent modEvent{RE::BSFixedString("EP_MeleeParryEvent"), RE::BSFixedString(), 0.0f, a_attacker};
	    SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }

    inline static void sendRangedEvent() {
            SKSE::ModCallbackEvent modEvent{RE::BSFixedString("EP_RangedParryEvent"), RE::BSFixedString(), 0.0f, nullptr };
            SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
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

    inline static void StaggerNearby(RE::Actor* a_defender, float radius, RE::SpellItem* a_spell, RE::Actor* excluded) {
        auto* cell = a_defender ? a_defender->GetParentCell() : nullptr;
        if (!cell || !cell->IsAttached() || !a_spell || radius <= 0.0f) {
            return;
        }
        const bool log = settings::Get().log;
        radius = std::min(radius, 4095.0f);
        cell->ForEachReferenceInRange(a_defender->GetPosition(), radius, [&](RE::TESObjectREFR* ref){
            auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
            if (!actor || actor->IsDisabled() || !actor->Is3DLoaded() || actor == a_defender || actor == excluded) {
                return RE::BSContainer::ForEachResult::kContinue;
            }
            if (log) SKSE::log::info("[StaggerNearby] target={:08X}", actor->GetFormID());
            ApplySpell(a_defender, actor, a_spell);
            return RE::BSContainer::ForEachResult::kContinue;
        });
    }

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
    
    //from: https://github.com/doodlum/EldenParry/blob/f6bd72eed354a54058805694f7adaecf24a8e0fa/src/Utils.hpp#L177
    static inline void resetProjectileOwner(RE::Projectile* a_projectile, RE::Actor* a_actor, RE::hkpCollidable* a_projectile_collidable) {
		a_projectile->SetActorCause(a_actor->GetActorCause());
		a_projectile->GetProjectileRuntimeData().shooter = a_actor->GetHandle();
		RE::CFilter collisionFilterInfo{};
		a_actor->GetCollisionFilterInfo(collisionFilterInfo);
		a_projectile_collidable->broadPhaseHandle.collisionFilterInfo.SetSystemGroup(collisionFilterInfo.GetSystemGroup());
	}

    static inline void SetRotationMatrix(RE::NiMatrix3& a_matrix, float sacb, float cacb, float sb) {
		float cb = std::sqrtf(1 - sb * sb);
		float ca = cacb / cb;
		float sa = sacb / cb;
		a_matrix.entry[0][0] = ca;
		a_matrix.entry[0][1] = -sacb;
		a_matrix.entry[0][2] = sa * sb;
		a_matrix.entry[1][0] = sa;
		a_matrix.entry[1][1] = cacb;
		a_matrix.entry[1][2] = -ca * sb;
		a_matrix.entry[2][0] = 0.0;
		a_matrix.entry[2][1] = sb;
		a_matrix.entry[2][2] = cb;
	}

    static inline void ReflectProjectile(RE::Projectile* a_projectile) {
        const float PI = 3.1415926535897932384626f;
		a_projectile->GetProjectileRuntimeData().linearVelocity *= -1.f;

		// rotate model
		auto projectileNode = a_projectile->Get3D2();
		if (projectileNode) {
			RE::NiPoint3 direction = a_projectile->GetProjectileRuntimeData().linearVelocity;
			direction.Unitize();

			a_projectile->data.angle.x = asin(direction.z);
			a_projectile->data.angle.z = atan2(direction.x, direction.y);

			if (a_projectile->data.angle.z < 0.0) {
				a_projectile->data.angle.z += PI;
			}

			if (direction.x < 0.0) {
				a_projectile->data.angle.z += PI;
			}

			SetRotationMatrix(projectileNode->local.rotate, -direction.x, direction.y, direction.z);
		}
	}

    static inline void getBodyPos(RE::Actor* a_actor, RE::NiPoint3& pos) {
		if (!a_actor->GetActorRuntimeData().race) {
			return;
		}
		RE::BGSBodyPart* bodyPart = a_actor->GetActorRuntimeData().race->bodyPartData->parts[0];
		if (!bodyPart) {
			return;
		}
		auto targetPoint = a_actor->GetNodeByName(bodyPart->targetName.c_str());
		if (!targetPoint) {
			return;
		}

		pos = targetPoint->world.translate;
	}

    static inline bool ApproximatelyEqual(float A, float B) {
		return ((A - B) < FLT_EPSILON) && ((B - A) < FLT_EPSILON);
	}


    static inline bool PredictAimProjectile(RE::NiPoint3 a_projectilePos, RE::NiPoint3 a_targetPosition, RE::NiPoint3 a_targetVelocity, float a_gravity, RE::NiPoint3& a_projectileVelocity) {
		// http://ringofblades.com/Blades/Code/PredictiveAim.cs

		float projectileSpeedSquared = a_projectileVelocity.SqrLength();
		float projectileSpeed = std::sqrtf(projectileSpeedSquared);

		if (projectileSpeed <= 0.f || a_projectilePos == a_targetPosition) {
			return false;
		}

		float targetSpeedSquared = a_targetVelocity.SqrLength();
		float targetSpeed = std::sqrtf(targetSpeedSquared);
		RE::NiPoint3 targetToProjectile = a_projectilePos - a_targetPosition;
		float distanceSquared = targetToProjectile.SqrLength();
		float distance = std::sqrtf(distanceSquared);
		RE::NiPoint3 direction = targetToProjectile;
		direction.Unitize();
		RE::NiPoint3 targetVelocityDirection = a_targetVelocity;
		targetVelocityDirection.Unitize();

		float cosTheta = (targetSpeedSquared > 0) ? direction.Dot(targetVelocityDirection) : 1.0f;

		bool bValidSolutionFound = true;
		float t;

		if (ApproximatelyEqual(projectileSpeedSquared, targetSpeedSquared)) {
			// We want to avoid div/0 that can result from target and projectile traveling at the same speed
			//We know that cos(theta) of zero or less means there is no solution, since that would mean B goes backwards or leads to div/0 (infinity)
			if (cosTheta > 0) {
				t = 0.5f * distance / (targetSpeed * cosTheta);
			} else {
				bValidSolutionFound = false;
				t = 1;
			}
		} else {
			float a = projectileSpeedSquared - targetSpeedSquared;
			float b = 2.0f * distance * targetSpeed * cosTheta;
			float c = -distanceSquared;
			float discriminant = b * b - 4.0f * a * c;

			if (discriminant < 0) {
				// NaN
				bValidSolutionFound = false;
				t = 1;
			} else {
				// a will never be zero
				float uglyNumber = sqrtf(discriminant);
				float t0 = 0.5f * (-b + uglyNumber) / a;
				float t1 = 0.5f * (-b - uglyNumber) / a;

				// Assign the lowest positive time to t to aim at the earliest hit
				t = std::min(t0, t1);
				if (t < FLT_EPSILON) {
					t = std::max(t0, t1);
				}

				if (t < FLT_EPSILON) {
					// Time can't flow backwards when it comes to aiming.
					// No real solution was found, take a wild shot at the target's future location
					bValidSolutionFound = false;
					t = 1;
				}
			}
		}

		a_projectileVelocity = a_targetVelocity + (-targetToProjectile / t);

		if (!bValidSolutionFound) {
			a_projectileVelocity.Unitize();
			a_projectileVelocity *= projectileSpeed;
		}

		if (!ApproximatelyEqual(a_gravity, 0.f)) {
			float netFallDistance = (a_projectileVelocity * t).z;
			float gravityCompensationSpeed = (netFallDistance + 0.5f * a_gravity * t * t) / t;
			a_projectileVelocity.z = gravityCompensationSpeed;
		}

		return bValidSolutionFound;
	}

    static inline void RetargetProjectile(RE::Projectile* a_projectile, RE::TESObjectREFR* a_target) {
        const float PI = 3.1415926535897932384626f;
		a_projectile->GetProjectileRuntimeData().desiredTarget = a_target;

		auto projectileNode = a_projectile->Get3D2();
		auto targetHandle = a_target->GetHandle();

		RE::NiPoint3 targetPos = a_target->GetPosition();
		if (a_target->GetFormType() == RE::FormType::ActorCharacter) {
			getBodyPos(a_target->As<RE::Actor>(), targetPos);
		}

		RE::NiPoint3 targetVelocity;
		targetHandle.get()->GetLinearVelocity(targetVelocity);

		float projectileGravity = 0.f;
		if (auto ammo = a_projectile->GetProjectileRuntimeData().ammoSource) {
			if (auto bgsProjectile = ammo->GetRuntimeData().data.projectile) {
				projectileGravity = bgsProjectile->data.gravity;
				if (auto bhkWorld = a_projectile->parentCell->GetbhkWorld()) {
					if (auto hkpWorld = bhkWorld->GetWorld1()) {
						auto vec4 = hkpWorld->gravity;
						float quad[4];
						_mm_store_ps(quad, vec4.quad);
						float gravity = -quad[2] * RE::bhkWorld::GetWorldScaleInverse();
						projectileGravity *= gravity;
					}
				}
			}
		}

		PredictAimProjectile(a_projectile->data.location, targetPos, targetVelocity, projectileGravity, a_projectile->GetProjectileRuntimeData().linearVelocity);

		// rotate
		RE::NiPoint3 direction = a_projectile->GetProjectileRuntimeData().linearVelocity;
		direction.Unitize();

		a_projectile->data.angle.x = asin(direction.z);
		a_projectile->data.angle.z = atan2(direction.x, direction.y);

		if (a_projectile->data.angle.z < 0.0) {
			a_projectile->data.angle.z += PI;
		}

		if (direction.x < 0.0) {
			a_projectile->data.angle.z += PI;
		}

		SetRotationMatrix(projectileNode->local.rotate, -direction.x, direction.y, direction.z);
	}

}
