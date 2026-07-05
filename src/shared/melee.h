#pragma once

// Timing-based melee combat core: stamina, attack phases (windup -> active
// -> recovery, with a combo window), blocking/parry/riposte, perfect
// counters, feints and kicks. Inspired by heavy medieval duel games in
// spirit only - every value and rule here is original.
//
// Design constraints:
// - Pure state + pure functions. No IO, no rendering, no pointers, no
//   content lookups. An Actor is a plain struct a future authoritative
//   server can tick per player and serialize; the client keeps presentation
//   (sounds, sparks, screen shake) entirely outside this module.
// - The caller owns targeting (raycasts, ranges) and health. This module
//   owns time, stamina, and the outcome when a strike meets a defender.

namespace melee {

// --- tuning constants (one place, so retuning the feel is one diff) ------

inline constexpr float kMaxStamina = 100.0f;
inline constexpr float kRegenPerSecond = 32.0f;
inline constexpr float kRegenDelay = 0.9f;   // after any stamina spend
inline constexpr float kLowStamina = 30.0f;  // below this, defense weakens
inline constexpr float kLowStaminaBlockMul = 1.5f;

inline constexpr float kParryWindow = 0.15f;    // guard-raise time that parries
inline constexpr float kParryCost = 3.0f;
inline constexpr float kParriedStagger = 1.1f;  // attacker stagger when parried
inline constexpr float kRiposteWindow = 0.8f;   // after a parry, attacks are ripostes
inline constexpr float kRiposteWindupMul = 0.55f;
inline constexpr float kRiposteDamageMul = 1.25f;
inline constexpr float kCounterStagger = 1.3f;  // attacker stagger on a perfect counter

inline constexpr float kBlockStaminaCost = 12.0f; // defender cost, light hit blocked
inline constexpr float kHeavyBlockStaminaCost = 20.0f;
inline constexpr float kShieldBlockCostMul = 0.6f; // shields block cheaper, but
                                                   // slow you while raised (the
                                                   // caller applies the slow)
inline constexpr float kAttackerBlockCost = 4.0f;  // hitting a guard tires the attacker too
inline constexpr float kBlockBounceRecover = 0.30f; // attacker bounce-off after a block

inline constexpr float kGuardBreakStagger = 1.4f;
inline constexpr float kGuardBreakRefund = 15.0f; // stamina after a break so play resumes
inline constexpr float kInterruptFlinch = 0.5f;   // hit clean during your windup

inline constexpr float kHeavyExtraWindup = 0.22f;
inline constexpr float kHeavyDamageMul = 1.6f;
inline constexpr float kHeavyStaminaMul = 1.8f;
inline constexpr float kHeavyExtraRecover = 0.10f;
inline constexpr float kHeavyHoldTime = 0.10f; // hold LMB this long into windup to commit

inline constexpr float kComboWindowFrac = 0.45f; // last part of recovery chains
inline constexpr float kComboWindupMul = 0.65f;
inline constexpr float kComboStaminaStep = 2.0f; // +N stamina per chained attack
inline constexpr int kMaxComboDepth = 3;

inline constexpr float kFeintCost = 5.0f;
inline constexpr float kFeintRecover = 0.18f;

inline constexpr float kKickCost = 10.0f;
inline constexpr float kKickCooldown = 1.2f;
inline constexpr float kKickDamage = 5.0f;
inline constexpr float kKickRange = 1.3f;         // caller enforces
inline constexpr float kKickGuardStagger = 1.0f;  // guard kicked open
inline constexpr float kKickStaminaDamage = 15.0f;

inline constexpr float kThrowCost = 8.0f;

// --- attacks --------------------------------------------------------------

enum class Attack : int { SlashLeft = 0, SlashRight, Overhead, Stab, Kick };

struct Spec {
    float windup;    // seconds (scaled by weapon weight; kick ignores weight)
    float active;    // hit window
    float recovery;
    float damageMul; // times the weapon's base damage (kick uses kKickDamage flat)
    float stamina;   // base cost to swing
};
const Spec& spec(Attack a);

enum class Phase : int { Idle, Windup, Active, Recovery, Stagger };

// --- combatant state -------------------------------------------------------

struct Actor {
    // Stamina.
    float stamina = kMaxStamina;
    float regenDelay = 0.0f;

    // Attack state machine.
    Phase phase = Phase::Idle;
    Attack attack = Attack::SlashLeft;
    bool heavy = false;
    bool riposte = false;    // this attack was started inside a riposte window
    bool strikeDone = false; // the active window already resolved against a target
    float phaseLeft = 0.0f;  // seconds remaining in the current phase
    float phaseTotal = 0.0f; // full phase length (for animation progress)
    int comboDepth = 0;

    // Defense.
    bool blockHeld = false;
    float blockTime = 999.0f; // seconds since the guard came up (parry timing)
    bool hasShield = false;
    bool counters = true;     // can perfect-counter (training bots leave this off)

    // Windows / cooldowns / equipment.
    float riposteWindow = 0.0f;
    float kickCooldown = 0.0f;
    float weight = 1.0f; // weapon weight: scales windup/recovery/stamina costs
};

// The guard only protects while idle: attacking, recovering, or staggering
// drops it. (Input keeps blockHeld true; this is the effective state.)
inline bool guardUp(const Actor& a) {
    return a.blockHeld && a.phase == Phase::Idle;
}

inline bool inParryWindow(const Actor& a) {
    return guardUp(a) && a.blockTime <= kParryWindow;
}

// 0 -> 1 progress through the current phase (for animation).
inline float phaseProgress(const Actor& a) {
    return a.phaseTotal > 0.0f ? 1.0f - a.phaseLeft / a.phaseTotal : 0.0f;
}

void tick(Actor& a, float dt);
void setBlock(Actor& a, bool held); // call every frame with the raw input

bool canStartAttack(const Actor& a);
bool startAttack(Actor& a, Attack type, bool heavy);
bool upgradeToHeavy(Actor& a); // light windup -> heavy (hold left click)
bool feint(Actor& a);          // cancel the windup; costs stamina
bool spendStamina(Actor& a, float cost); // for throws etc; false if too tired

// Damage the current attack deals if it lands clean.
float damageOf(const Actor& a, float weaponBaseDamage);

// --- strike resolution ------------------------------------------------------

enum class Outcome {
    Hit,             // clean hit: caller applies damageOf() to health
    Blocked,         // guard held (weapon): stamina traded, attacker bounces
    ShieldBlocked,   // guard held (shield): cheaper for the defender
    Parried,         // guard raised inside the parry window: attacker staggered,
                     //   defender gets a riposte window
    PerfectCountered,// defender was winding up the SAME attack: attacker
                     //   staggered, defender's attack accelerates into a riposte
    GuardBroken,     // block drained the last stamina: defender staggers long
    GuardOpened,     // kick vs a raised guard: defender staggered + drained
};

// Resolves the attacker's active strike against the defender's state,
// applying stamina and phase changes to BOTH actors. Health is the
// caller's job (only Outcome::Hit / GuardOpened deal any).
Outcome resolveStrike(Actor& attacker, Actor& defender);

} // namespace melee
