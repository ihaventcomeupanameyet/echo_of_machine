#include "inventory.hpp"
#include "tiny_ecs_registry.hpp"

// Constructor initializes slots based on the number of rows and columns
Inventory::Inventory(int rows, int columns, glm::vec2 slotSize)
    : rows(rows), columns(columns), slotSize(slotSize), selectedSlot(0) {
    slots.resize(rows * columns + 1);  // +1 for the armor slot
    slots.back().type = InventorySlotType::ARMOR;  // Assign last slot as Armor slot
}


// Add item to inventory, increase quantity if it already exists
void Inventory::addItem(const std::string& itemName, int quantity) {
    for (auto& slot : slots) {
        if (slot.item.name == itemName) {
            slot.item.quantity += quantity;
            return;
        }
    }
    // Add to the first empty slot if item not found
    for (auto& slot : slots) {
        if (slot.item.name.empty()) {
            slot.item = { itemName, quantity };
            return;
        }
    }
}

// Remove a specified quantity of an item from the inventory
void Inventory::removeItem(const std::string& itemName, int quantity) {
    for (auto& slot : slots) {
        if (slot.item.name == itemName) {
            slot.item.quantity -= quantity;
            if (slot.item.quantity <= 0) {
                slot.item = { "", 0 }; // Clear slot if quantity <= 0
            }
            return;
        }
    }
}

// Display inventory items
void Inventory::display() const {
    for (const auto& slot : slots) {
        if (!slot.item.name.empty()) {
            std::cout << slot.item.name << " x" << slot.item.quantity << std::endl;
        }
    }
}

// Set the selected slot within bounds
void Inventory::setSelectedSlot(int slot) {
    if (slot >= 0 && slot < static_cast<int>(slots.size())) {
        selectedSlot = slot;
    }
}

// Get all non-empty items in the inventory
std::vector<Item> Inventory::getItems() const {
    std::vector<Item> nonEmptyItems;
    for (const auto& slot : slots) {
        if (!slot.item.name.empty()) {
            nonEmptyItems.push_back(slot.item);
        }
    }
    return nonEmptyItems;
}

// Swap items between two slots
void Inventory::swapItems(int draggedSlot, int targetSlot) {
    if (draggedSlot >= 0 && draggedSlot < static_cast<int>(slots.size()) &&
        targetSlot >= 0 && targetSlot < static_cast<int>(slots.size())) {
        std::swap(slots[draggedSlot].item, slots[targetSlot].item);
    }
}

void Inventory::placeItemInSlot(int draggedSlotIndex, int targetSlotIndex) {
    // Check if dragged item is valid
    InventorySlot& draggedSlot = slots[draggedSlotIndex];
    Item draggedItem = draggedSlot.item;
    if (draggedItem.name.empty() || draggedItem.quantity <= 0) {
        std::cerr << "No item to place in the target slot." << std::endl;
        return;
    }

    // Clear the original slot
    draggedSlot.item = {};

    // Place in armor slot if target is armor
    if (targetSlotIndex == slots.size() - 1) {
        slots.back().item = draggedItem;
        std::cout << "Placed " << draggedItem.name << " in armor slot." << std::endl;
    }
    else {
        // Otherwise, place in the target slot
        slots[targetSlotIndex].item = draggedItem;
        std::cout << "Placed " << draggedItem.name << " in slot index " << targetSlotIndex << "." << std::endl;
    }
}


InventorySlot& Inventory::getArmorSlot() {
    return slots.back();
}
Item Inventory::getArmorItem() {
    return slots.back().item;
}
void Inventory::useSelectedItem() {
    Item& selectedItem = slots[selectedSlot].item;
    if (selectedItem.name == "HealthPotion") {
        Entity player_e = registry.players.entities[0];
        Player& player = registry.players.get(player_e);
        player.current_health += 30.f;
        if (player.current_health > 100.f) {
            player.current_health = 100.f;
        }; 
        removeItem(selectedItem.name, 1);
    }
}

void Inventory::moveItem(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= slots.size() || toIndex < 0 || toIndex >= slots.size()) {
        std::cerr << "Invalid slot indices." << std::endl;
        return;
    }

    // Move item from 'fromIndex' to 'toIndex' and clear the original slot
    slots[toIndex].item = slots[fromIndex].item;
    slots[fromIndex].item = {};  // Clear the original slot
}