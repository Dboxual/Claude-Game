#include "shared/melee.h"

#include <algorithm>

namespace melee {

namespace {

const Spec kSpecs[] = {
    // windup active recovery dmgMul stamina
    {0.28f, 0.14f, 0.34f, 1.00f, 6.0f}, // SlashLeft
    {0.28f, 0.14f, 0.34f, 1.00f, 6.0f}, // SlashRight
    {0.34f, 0.12f, 0.38f, 1.15f, 7.0f}, // Overhead
    {0.24f, 0.12f, 0.30f, 0.85f, 5.0f}, // Stab
    {0.18f, 0.10f, 0.35f, 0.00f, kKickCost}, // Kick (flat kKickDamage)
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

// Hit clean while winding up: the attack is knocked out of your hands.
void interruptIfWindup(Actor& a) {
    if (a.phase == Phase::Windup) stagger(a, kInterruptFlinch);
}

} // namespace

const Spec& spec(Attack a) { return kSpecs[int(a)]; }

void tick(Actor& a, float dt) {
    a.regenDelay -= dt;
    a.kickCooldown = std::max(0.0f, a.kickCooldown - dt);
    a.riposteWindow = std::max(0.0f, a.riposteWindow - dt);
    if (a.blockHeld) a.blockTime += dt;

    // Regen pauses while spending, while the guard is up (raising a shield
    // or blade costs effort), and while staggered.
    if (a.regenDelay <= 0.0f && !a.blockHeld && a.phase != Phase::Stagger) {
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
        float w = a.attack == Attack::Kick ? 1.0f : a.weight;
        float recover = s.recovery * w + (a.heavy ? kHeavyExtraRecover : 0.0f);
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

    const Spec& s = spec(type);
    bool combo = a.phase == Phase::Recovery;
    bool riposte = a.riposteWindow > 0.0f && type != Attack::Kick;
    float w = type == Attack::Kick ? 1.0f : a.weight;

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
    if (a.phase != Phase::Windup || a.heavy || a.attack == Attack::Kick) return false;
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

float damageOf(const Actor& a, float weaponBaseDamage) {
    if (a.attack == Attack::Kick) return kKickDamage;
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
        interruptIfWindup(defender);
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

    if (guardUp(defender)) {
        // Timed parry: the guard came up just before the hit.
        if (defender.blockTime <= kParryWindow) {
            drain(defender, kParryCost);
            stagger(attacker, kParriedStagger);
            defender.riposteWindow = kRiposteWindow;
            return Outcome::Parried;
        }

        // Held (late) block: safe but expensive - and low stamina makes it
        // worse, until the guard finally shatters.
        float cost = attacker.heavy ? kHeavyBlockStaminaCost : kBlockStaminaCost;
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

    interruptIfWindup(defender);
    return Outcome::Hit;
}

} // namespace melee
