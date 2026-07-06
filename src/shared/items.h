#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <vector>

// RPG item foundation: the things that live in the player's item inventory
// (resources, tools, weapons, equipment) as opposed to world entities
// (shared/content.h), which are the things placed IN the world. A world
// entity can reference an item (a resource node yields one, a ground drop
// grants one); gameplay only ever passes item ids around.
//
// Shared (std + glm only): the server will own item inventories per player
// once networking lands, so nothing in here may touch UI or platform code.

enum class ItemKind { Resource, Tool, Weapon, Equipment };

struct ItemDef {
    std::string id;          // "wood", "crude_axe" - stable, save-file safe
    std::string displayName; // "Wood", "Crude Axe"
    ItemKind kind = ItemKind::Resource;
    int maxStack = 99;       // resources stack; tools/weapons are 1 per slot
    glm::vec3 color{1.0f};   // inventory swatch + ground-drop tint

    // Equippable items (tools/weapons) reference a WeaponDef by id: equipping
    // the item puts that weapon in the player's hands. Stats stay on the
    // WeaponDef so combat never needs to know about the item system.
    std::string weaponId;

    // Tools: gathering a node whose toolClass matches doubles the yield.
    // "" = not a gathering tool.
    std::string toolClass; // "axe", "pickaxe"
};

class ItemRegistry {
public:
    void registerBuiltins();
    void addOrReplace(const ItemDef& def); // future data files: same id replaces
    const ItemDef* find(const std::string& id) const;
    const std::vector<ItemDef>& defs() const { return defs_; }

private:
    std::vector<ItemDef> defs_;
};

// --- item inventory: stacks + one equipment slot ---------------------------

struct ItemStack {
    std::string itemId;
    int count = 0;
};

// The player's item inventory. Plain data, no registry pointer: callers pass
// the per-item max stack in so this stays trivially serializable/testable.
struct ItemInventory {
    static constexpr int kSlots = 24;

    std::vector<ItemStack> stacks;
    std::string equippedItemId; // main-hand tool/weapon; "" = bare hands

    void reset() {
        stacks.clear();
        equippedItemId.clear();
    }

    int count(const std::string& itemId) const {
        int total = 0;
        for (const ItemStack& s : stacks) {
            if (s.itemId == itemId) total += s.count;
        }
        return total;
    }

    // Adds up to `amount`, filling existing stacks before opening new slots.
    // Returns how many were actually added (less than `amount` = full).
    int add(const std::string& itemId, int amount, int maxStack) {
        int added = 0;
        for (ItemStack& s : stacks) {
            if (added >= amount) break;
            if (s.itemId != itemId || s.count >= maxStack) continue;
            int take = std::min(amount - added, maxStack - s.count);
            s.count += take;
            added += take;
        }
        while (added < amount && int(stacks.size()) < kSlots) {
            int take = std::min(amount - added, maxStack);
            stacks.push_back({itemId, take});
            added += take;
        }
        return added;
    }

    // Removes exactly `amount` if available (false = nothing was touched).
    // Unequips the item if its last copy leaves the inventory.
    bool remove(const std::string& itemId, int amount) {
        if (count(itemId) < amount) return false;
        for (size_t i = stacks.size(); i-- > 0 && amount > 0;) {
            if (stacks[i].itemId != itemId) continue;
            int take = std::min(amount, stacks[i].count);
            stacks[i].count -= take;
            amount -= take;
            if (stacks[i].count <= 0) stacks.erase(stacks.begin() + long(i));
        }
        if (!equippedItemId.empty() && count(equippedItemId) <= 0) {
            equippedItemId.clear();
        }
        return true;
    }

    bool has(const std::string& itemId, int amount) const {
        return count(itemId) >= amount;
    }
};
