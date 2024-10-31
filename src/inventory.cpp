#include "inventory.hpp"

// Constructor initializes slots based on the number of rows and columns
Inventory::Inventory(int rows, int columns, glm::vec2 slotSize)
    : rows(rows), columns(columns), slotSize(slotSize), selectedSlot(0) {
    slots.resize(rows * columns);
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

void Inventory::placeItemInSlot(int draggedSlotIndex, InventorySlotType targetSlotType) {
    // Ensure dragged slot is valid
    if (draggedSlotIndex < 0 || draggedSlotIndex >= slots.size()) {
        std::cerr << "Invalid slot index." << std::endl;
        return;
    }

    // Get the dragged item
    InventorySlot& draggedSlot = slots[draggedSlotIndex];
    Item draggedItem = draggedSlot.item;

    // Check if the dragged item is valid
    if (draggedItem.name.empty() || draggedItem.quantity <= 0) {
        std::cerr << "No item to place in the target slot." << std::endl;
        return;
    }

    // Find the target slot based on the targetSlotType
    for (auto& slot : slots) {
        if (slot.type == targetSlotType) {
            // Swap items if the target slot already has an item
            if (!slot.item.name.empty()) {
                std::swap(slot.item, draggedSlot.item);
            }
            else {
                // Otherwise, move the item to the target slot
                slot.item = draggedSlot.item;
                draggedSlot.item = {}; // Clear the original slot
            }
            std::cout << "Placed " << draggedItem.name << " in "
                << (targetSlotType == InventorySlotType::ARMOR ? "Armor" : "Weapon") << " slot." << std::endl;
            return;
        }
    }

    std::cerr << "Target slot type not found in inventory." << std::endl;
}
