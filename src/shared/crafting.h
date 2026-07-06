#pragma once

#include "shared/items.h"

#include <string>
#include <vector>

// Recipe foundation: consume input item stacks, produce an output item.
// Deliberately flat and data-shaped so recipes can move into content files
// (server/content/recipes/*.cfg) without changing any call site.

struct Recipe {
    std::string id;           // "crude_axe" - usually named after the output
    std::string outputItemId;
    int outputCount = 1;
    std::vector<ItemStack> inputs; // itemId + count consumed per craft
};

class RecipeRegistry {
public:
    void registerBuiltins();
    void addOrReplace(const Recipe& recipe);
    const std::vector<Recipe>& recipes() const { return recipes_; }

private:
    std::vector<Recipe> recipes_;
};

bool canCraft(const ItemInventory& inv, const Recipe& recipe);

// Consumes the inputs and adds the output. False (and no changes) if the
// inputs are missing or the inventory cannot hold the result.
bool craft(ItemInventory& inv, const Recipe& recipe, const ItemRegistry& items);
