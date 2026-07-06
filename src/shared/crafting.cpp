#include "shared/crafting.h"

void RecipeRegistry::registerBuiltins() {
    recipes_.clear();

    // Starter chain: bare hands gather slowly -> craft tools -> gather fast
    // -> craft weapons. Costs are deliberately small so the whole loop can
    // be play-tested in a couple of minutes.
    recipes_.push_back({"crude_axe", "crude_axe", 1, {{"wood", 3}, {"stone", 2}}});
    recipes_.push_back({"crude_pickaxe", "crude_pickaxe", 1, {{"wood", 3}, {"ore", 2}}});
    recipes_.push_back({"basic_sword", "basic_sword", 1, {{"wood", 2}, {"ore", 3}}});
    recipes_.push_back({"basic_staff", "basic_staff", 1, {{"wood", 4}, {"ore", 1}}});
}

void RecipeRegistry::addOrReplace(const Recipe& recipe) {
    for (Recipe& r : recipes_) {
        if (r.id == recipe.id) {
            r = recipe;
            return;
        }
    }
    recipes_.push_back(recipe);
}

bool canCraft(const ItemInventory& inv, const Recipe& recipe) {
    for (const ItemStack& in : recipe.inputs) {
        if (!inv.has(in.itemId, in.count)) return false;
    }
    return true;
}

bool craft(ItemInventory& inv, const Recipe& recipe, const ItemRegistry& items) {
    if (!canCraft(inv, recipe)) return false;

    const ItemDef* out = items.find(recipe.outputItemId);
    int maxStack = out ? out->maxStack : 1;

    // All-or-nothing: try the add on a copy first so a full inventory never
    // eats the ingredients.
    ItemInventory probe = inv;
    for (const ItemStack& in : recipe.inputs) probe.remove(in.itemId, in.count);
    if (probe.add(recipe.outputItemId, recipe.outputCount, maxStack) < recipe.outputCount) {
        return false;
    }
    inv = std::move(probe);
    return true;
}
