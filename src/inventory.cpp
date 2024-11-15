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
    // Check for valid indices
    if (draggedSlotIndex < 0 || draggedSlotIndex >= slots.size() ||
        targetSlotIndex < 0 || targetSlotIndex >= slots.size()) {
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

    // Clear the original slot
    draggedSlot.item = {};

    // Place the item in the armor slot explicitly
    if (targetSlotIndex == slots.size() - 1) { // Check if target is the armor slot
        slots.back().item = draggedItem; // Directly assign the item
        std::cout << "Placed " << draggedItem.name << " in armor slot." << std::endl;
    }
    else {
        slots[targetSlotIndex].item = draggedItem; // Regular slot assignment
        std::cout << "Placed " << draggedItem.name << " in slot index " << targetSlotIndex << "." << std::endl;
    }
}

InventorySlot& Inventory::getArmorSlot() {
    // Find the armor slot in the slots vector
    for (auto& slot : slots) {
        if (slot.type == InventorySlotType::ARMOR) {
            return slot;
        }
    }

    // If no armor slot exists, throw an error (or handle as needed)
    throw std::runtime_error("Armor slot not found in inventory.");
}

Item Inventory::getArmorItem() {
    // Check if the last slot is of type ARMOR and contains an item
    if (!slots.empty()) {
        Item armorItem = slots.back().item;
        std::cout << "Armor item name: " << armorItem.name << std::endl; // Debug print
        return armorItem;
    }

    // If no armor item is present, return an empty item (or handle as needed)
    return Item{ "", 0 };
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




void to_json(json& j, const Item& item) {
    j = json{
        {"name", item.name},
        {"quantity", item.quantity}
    };
}

void from_json(const json& j, Item& item) {
    j.at("name").get_to(item.name);
    j.at("quantity").get_to(item.quantity);
}


void to_json(json& j, const InventorySlot& slot) {
    json position_json;
    to_json(position_json, slot.position);  

    j = json{
        {"type", slot.type},
        {"item", slot.item},
        {"position", position_json}
    };
}

void from_json(const json& j, InventorySlot& slot) {
    from_json(j.at("position"), slot.position); 

    j.at("type").get_to(slot.type);
    j.at("item").get_to(slot.item);
}


void to_json(json& j, const Inventory& inventory) {
    j = json{
        {"rows", inventory.get_rows()},
        {"columns", inventory.get_columns()},
        {"slotSize", {inventory.get_slotSize().x, inventory.get_slotSize().y}},
        {"selectedSlot", inventory.getSelectedSlot()},
        {"slots", inventory.slots},
        {"isOpen", inventory.isOpen}
    };
}

void from_json(const json& j, Inventory& inventory) {
    int rows, columns, selectedSlot;
    glm::vec2 slotSize;
    bool isOpen;
    std::vector<InventorySlot> slots;

    j.at("rows").get_to(rows);
    j.at("columns").get_to(columns);
    j.at("slotSize").at(0).get_to(slotSize.x);
    j.at("slotSize").at(1).get_to(slotSize.y);
    j.at("selectedSlot").get_to(selectedSlot);
    j.at("slots").get_to(slots);
    j.at("isOpen").get_to(isOpen);

    inventory = Inventory(rows, columns, slotSize);
    inventory.setSelectedSlot(selectedSlot);
    inventory.slots = slots;
    inventory.isOpen = isOpen;
}