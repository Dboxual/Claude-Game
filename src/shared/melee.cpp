#include "shared/melee.h"

#include <algorithm>

namespace melee {

namespace {

const Spec kSpecs[] = {
    // windup active recovery dmgMul stamina blockCost
    {0.35f, 0.18f, 0.42f, 1.00f, 7.0f, 12.0f},      // SlashLeft
    {0.35f, 0.18f, 0.42f, 1.00f, 7.0f, 12.0f},      // SlashRight
    {0.42f, 0.16f, 0.48f, 1.20f, 8.0f, 16.0f},      // Overhead
    {0.30f, 0.12f, 0.38f, 0.85f, 6.0f, 10.0f},      // Stab
    {0.20f, 0.10f, 0.40f, 0.00f, kKickCost, 0.0f},  // Kick (flat kKickDamage)
    {0.13f, 0.08f, 0.34f, 0.00f, kJabCost, 4.0f},   // Jab (flat kJabDamage)
};

void enterPhase(Actor& a, Phase p, float duration) {
    a.phase = p;
    a.phaseLeft = a.phaseTotal = duration;
}

void stagger(Actor& a, float duration) {
    // A fresh stagger never shortens one already running.
    if (a.phase == Phase::Stagger && a.phaseLeft > duration) return;
    enterPhase(a, Phase::Stagger, duration);
}

void drain(Actor& a, float cost) {
    a.stamina = std::max(0.0f, a.stamina - cost);
    a.regenDelay = kRegenDelay;
}

bool weightless(Attack t) { return t == Attack::Kick || t == Attack::Jab; }

} // namespace

const Spec& spec(Attack a) { return kSpecs[int(a)]; }

void tick(Actor& a, float dt) {
    a.regenDelay -= dt;
    a.kickCooldown = std::max(0.0f, a.kickCooldown - dt);
    a.jabCooldown = std::max(0.0f, a.jabCooldown - dt);
    a.riposteWindow = std::max(0.0f, a.riposteWindow - dt);
    if (a.blockHeld) a.blockTime += dt;

    // Holding the guard up costs effort (more with a shield) and pauses
    // regen - turtling is a losing stamina game. It never breaks the guard
    // by itself; only an incoming hit on an empty tank does that.
    if (guardUp(a)) {
        float mul = a.hasShield ? kGuardHoldDrainShieldMul : 1.0f;
        a.stamina = std::max(0.0f, a.stamina - kGuardHoldDrain * mul * dt);
    } else if (a.regenDelay <= 0.0f && !a.blockHeld && a.phase != Phase::Stagger) {
        a.stamina = std::min(kMaxStamina, a.stamina + kRegenPerSecond * dt);
    }

    if (a.phase == Phase::Idle) return;
    a.phaseLeft -= dt;
    if (a.phaseLeft > 0.0f) return;

    switch (a.phase) {
    case Phase::Windup: {
        const Spec& s = spec(a.attack);
        a.strikeDone = false;
        enterPhase(a, Phase::Active, s.active); // active window is never weight-scaled
        break;
    }
    case Phase::Active: {
        const Spec& s = spec(a.attack);
        float w = weightless(a.attack) ? 1.0f : a.weight;
        float recover = s.recovery * w + (a.heavy ? kHeavyExtraRecover : 0.0f);
        // Commitment: swinging at air is the most punishable mistake.
        if (!a.strikeDone) {
            recover *= a.attack == Attack::Kick ? kKickWhiffRecoverMul : kWhiffRecoverMul;
        }
        enterPhase(a, Phase::Recovery, recover);
        break;
    }
    case Phase::Recovery:
    case Phase::Stagger:
        a.phase = Phase::Idle;
        a.phaseLeft = a.phaseTotal = 0.0f;
        a.comboDepth = 0;
        break;
    case Phase::Idle:
        break;
    }
}

void setBlock(Actor& a, bool held) {
    if (held && !a.blockHeld) a.blockTime = 0.0f; // guard just came up
    a.blockHeld = held;
}

bool canStartAttack(const Actor& a) {
    if (a.phase == Phase::Idle) return true;
    // Combo window: the tail of recovery chains into the next attack.
    if (a.phase == Phase::Recovery && a.comboDepth < kMaxComboDepth &&
        a.phaseLeft <= a.phaseTotal * kComboWindowFrac) {
        return true;
    }
    return false;
}

bool startAttack(Actor& a, Attack type, bool heavy) {
    if (!canStartAttack(a)) return false;
    if (type == Attack::Kick && a.kickCooldown > 0.0f) return false;
    if (type == Attack::Jab && a.jabCooldown > 0.0f) return false;

    const Spec& s = spec(type);
    bool combo = a.phase == Phase::Recovery;
    bool riposte = a.riposteWindow > 0.0f && !weightless(type);
    float w = weightless(type) ? 1.0f : a.weight;

    float cost = s.stamina * w * (heavy ? kHeavyStaminaMul : 1.0f);
    if (combo) cost += kComboStaminaStep * float(a.comboDepth + 1); // chains tire you
    if (a.stamina < cost) return false; // too tired to swing at all

    drain(a, cost);
    a.attack = type;
    a.heavy = heavy;
    a.riposte = riposte;
    a.strikeDone = false;
    a.comboDepth = combo ? a.comboDepth + 1 : 0;
    if (type == Attack::Kick) a.kickCooldown = kKickCooldown;
    if (type == Attack::Jab) a.jabCooldown = kJabCooldown;

    float windup = s.windup * w + (heavy ? kHeavyExtraWindup : 0.0f);
    if (combo) windup *= kComboWindupMul;
    if (riposte) {
        windup *= kRiposteWindupMul;
        a.riposteWindow = 0.0f; // one riposte per parry
    }
    enterPhase(a, Phase::Windup, windup);
    return true;
}

bool upgradeToHeavy(Actor& a) {
    if (a.phase != Phase::Windup || a.heavy || weightless(a.attack)) return false;
    const Spec& s = spec(a.attack);
    float extraCost = s.stamina * a.weight * (kHeavyStaminaMul - 1.0f);
    if (a.stamina < extraCost) return false; // stays light
    drain(a, extraCost);
    a.heavy = true;
    a.phaseLeft += kHeavyExtraWindup;
    a.phaseTotal += kHeavyExtraWindup;
    return true;
}

bool feint(Actor& a) {
    if (a.phase != Phase::Windup) return false;
    if (a.stamina < kFeintCost) return false; // no free cancels, no infinite spam
    drain(a, kFeintCost);
    a.heavy = false;
    a.riposte = false;
    // A short recovery whose combo window opens quickly: cancel INTO another
    // attack, but never instantly.
    enterPhase(a, Phase::Recovery, kFeintRecover);
    return true;
}

bool spendStamina(Actor& a, float cost) {
    if (a.stamina < cost) return false;
    drain(a, cost);
    return true;
}

void bounceOff(Actor& a) {
    a.strikeDone = true;
    enterPhase(a, Phase::Recovery, kBlockBounceRecover);
}

float damageOf(const Actor& a, float weaponBaseDamage) {
    if (a.attack == Attack::Kick) return kKickDamage;
    if (a.attack == Attack::Jab) return kJabDamage;
    float dmg = weaponBaseDamage * spec(a.attack).damageMul;
    if (a.heavy) dmg *= kHeavyDamageMul;
    if (a.riposte) dmg *= kRiposteDamageMul;
    return dmg;
}

Outcome resolveStrike(Actor& attacker, Actor& defender) {
    attacker.strikeDone = true;

    // Kicks do not clash with blades - they exist to open guards.
    if (attacker.attack == Attack::Kick) {
        if (guardUp(defender)) {
            drain(defender, kKickStaminaDamage);
            stagger(defender, kKickGuardStagger);
            return Outcome::GuardOpened;
        }
        if (defender.phase == Phase::Windup && !defender.riposte) {
            stagger(defender, kInterruptFlinch);
            return Outcome::Interrupted;
        }
        return Outcome::Hit;
    }

    // Jab: a fast pommel bash. Its whole purpose is stopping a windup;
    // into a guard it just thuds, and on an idle body it barely scratches.
    if (attacker.attack == Attack::Jab) {
        if (defender.phase == Phase::Windup && !defender.riposte) {
            stagger(defender, kJabInterruptStagger);
            return Outcome::Interrupted;
        }
        if (guardUp(defender)) {
            drain(defender, spec(Attack::Jab).blockCost);
            return defender.hasShield ? Outcome::ShieldBlocked : Outcome::Blocked;
        }
        return Outcome::Hit;
    }

    // Perfect counter: the defender is winding up the SAME attack and reads
    // the exchange right - the blades meet, the attacker is thrown off, and
    // the defender's own strike accelerates into a riposte.
    if (defender.counters && defender.phase == Phase::Windup &&
        defender.attack == attacker.attack) {
        stagger(attacker, kCounterStagger);
        defender.riposte = true;
        defender.phaseLeft = std::min(defender.phaseLeft, 0.10f);
        defender.phaseTotal = std::max(defender.phaseTotal, 0.10f);
        return Outcome::PerfectCountered;
    }

    // Ripostes are protected: a riposte in its windup shrugs strikes off
    // and keeps coming. This is what makes winning the parry exchange
    // actually feel winning.
    if (defender.phase == Phase::Windup && defender.riposte) {
        drain(attacker, kAttackerBlockCost);
        enterPhase(attacker, Phase::Recovery, kBlockBounceRecover);
        return Outcome::Deflected;
    }

    if (guardUp(defender)) {
        // Timed parry: the guard came up just before the hit.
        if (defender.blockTime <= kParryWindow) {
            drain(defender, kParryCost);
            stagger(attacker, kParriedStagger);
            defender.riposteWindow = kRiposteWindow;
            return Outcome::Parried;
        }

        // Held (late) block: safe but expensive - low stamina makes it
        // worse, until the guard finally shatters.
        float cost = spec(attacker.attack).blockCost;
        if (attacker.heavy) cost *= kHeavyBlockMul;
        if (defender.hasShield) cost *= kShieldBlockCostMul;
        if (defender.stamina < kLowStamina) cost *= kLowStaminaBlockMul;

        if (defender.stamina <= cost) {
            defender.stamina = kGuardBreakRefund;
            defender.regenDelay = kRegenDelay;
            stagger(defender, kGuardBreakStagger);
            return Outcome::GuardBroken; // attacker recovers freely: punish window
        }
        drain(defender, cost);
        drain(attacker, kAttackerBlockCost); // clanging off a guard tires you too
        enterPhase(attacker, Phase::Recovery, kBlockBounceRecover);
        return defender.hasShield ? Outcome::ShieldBlocked : Outcome::Blocked;
    }

    if (defender.phase == Phase::Windup) {
        stagger(defender, kInterruptFlinch);
        return Outcome::Interrupted;
    }
    return Outcome::Hit;
}

} // namespace melee
