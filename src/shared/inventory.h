#pragma once

#include <string>
#include <vector>

// The smallest possible weapon/equipment state: which weapons the player
// owns (by content id, in acquisition order) and which is in hand. Shared
// because the server will simulate this per player once networking lands.
// Header-only plain data; no content lookups, no IO.
struct Inventory {
    std::vector<std::string> weapons;
    int equipped = 0; // index into weapons; always valid after reset()

    void reset(const std::string& baseWeaponId) {
        weapons.assign(1, baseWeaponId);
        equipped = 0;
    }

    bool has(const std::string& id) const {
        for (const std::string& w : weapons) {
            if (w == id) return true;
        }
        return false;
    }

    // Adds if not owned; either way equips it. True if newly acquired.
    bool acquireAndEquip(const std::string& id) {
        for (size_t i = 0; i < weapons.size(); ++i) {
            if (weapons[i] == id) {
                equipped = int(i);
                return false;
            }
        }
        weapons.push_back(id);
        equipped = int(weapons.size()) - 1;
        return true;
    }

    // 0-based slot; false if the slot is empty.
    bool selectSlot(int slot) {
        if (slot < 0 || slot >= int(weapons.size())) return false;
        equipped = slot;
        return true;
    }

    const std::string& equippedId() const {
        static const std::string none;
        if (weapons.empty()) return none;
        return weapons[size_t(equipped)];
    }
};
