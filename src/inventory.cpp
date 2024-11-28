#include "inventory.hpp"
#include "tiny_ecs_registry.hpp"

// Constructor initializes slots based on the number of rows and columns
Inventory::Inventory(int rows, int columns, glm::vec2 slotSize)
    : rows(rows), columns(columns), slotSize(slotSize), selectedSlot(0) {
    slots.resize(rows * columns + 2);  // +2 for armor and weapon slots
    slots[slots.size() - 2].type = InventorySlotType::ARMOR;   // Second last slot as Armor slot
    slots.back().type = InventorySlotType::WEAPON;             // Last slot as Weapon slot
}
const std::vector<Item> Inventory::disassembleItems = {
     {"Energy Core", 1},
    {"Robot Parts", 1},
    {"Teleporter", 1},
    {"ArmorPlate", 1}
};
// Add item to inventory, increase quantity if it already exists
void Inventory::addItem(const std::string& itemName, int quantity) {
    for (auto& slot : slots) {
        if (slot.item.name == itemName && !slot.item.isRobotCompanion) {
            slot.item.quantity += quantity;
            return;
        }
    }
    for (auto& slot : slots) {
        if (slot.item.name.empty()) {
            slot.item = Item(itemName, quantity);
            return;
        }
    }
}
bool Inventory::containsItem(const std::string& itemName)  {
    for (const auto& slot : slots) {
        if (slot.item.name == itemName) { // Assuming slot.item has a 'name' field
            return true;
        }
    }
    return false;
}
void Inventory::addCompanionRobot(const std::string& name, int health, int damage, int speed) {
    //// Check if the robot companion with the same name already exists
    //for (const auto& slot : slots) {
    //    if (slot.item.isRobotCompanion && slot.item.name == name) {
    //        std::cout << "Companion robot '" << name << "' already exists in the inventory!" << std::endl;
    //        return;
    //    }
    //}

    for (auto& slot : slots) {
        if (slot.item.name.empty()) {
            slot.item = Item(name, health, damage, speed);
            slot.item.isRobotCompanion = true; 
            std::cout << "Companion robot '" << name << "' added to the inventory." << std::endl;
            return;
        }
    }

    std::cout << "No empty slot available to add the companion robot!" << std::endl;
}

void Inventory::removeItem(const std::string& itemName, int quantity) {
    for (size_t i = 0; i < slots.size(); ++i) {
        InventorySlot& slot = slots[i];
        if (slot.item.name == itemName) {
            if (!slot.item.isRobotCompanion) {
                slot.item.quantity -= quantity;
                if (slot.item.quantity <= 0) {
                    slot.item = Item();
                    for (size_t j = i; j < slots.size() - 1; ++j) {
                        slots[j].item = slots[j + 1].item;
                    }
                    slots.back().item = Item();
                }
            }
            else {
                // For robot companions, just remove the item
                slot.item = Item();
                for (size_t j = i; j < slots.size() - 1; ++j) {
                    slots[j].item = slots[j + 1].item;
                }
                slots.back().item = Item(); 
            }
            return;
        }
    }
}


void Inventory::display() const {
    for (const auto& slot : slots) {
        if (!slot.item.name.empty()) {
            if (slot.item.isRobotCompanion) {
                std::cout << "Companion Robot: " << slot.item.name
                    << ", Health: " << slot.item.health
                    << ", Damage: " << slot.item.damage
                    << ", Speed: " << slot.item.speed << std::endl;
            }
            else {
                std::cout << slot.item.name << " x" << slot.item.quantity << std::endl;
            }
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
    if (targetSlotIndex == slots.size() - 2) {
        slots[slots.size() - 2].item = draggedItem;
        std::cout << "Placed " << draggedItem.name << " in armor slot." << std::endl;
    }
    // Place in weapon slot if target is weapon
    else if (targetSlotIndex == slots.size() - 1) {
        slots.back().item = draggedItem;
        std::cout << "Placed " << draggedItem.name << " in weapon slot." << std::endl;
    }
    else {
        // Otherwise, place in the target slot
        slots[targetSlotIndex].item = draggedItem;
        std::cout << "Placed " << draggedItem.name << " in slot index " << targetSlotIndex << "." << std::endl;
    }
}

// Get the armor slot (second last slot)
InventorySlot& Inventory::getArmorSlot() {
    return slots[slots.size() - 2];
}

// Get the weapon slot (last slot)
InventorySlot& Inventory::getWeaponSlot() {
    return slots.back();
}

// Retrieve the armor item
Item Inventory::getArmorItem() {
    return slots[slots.size() - 2].item;
}

// Retrieve the weapon item
Item Inventory::getWeaponItem() {
    return slots.back().item;
}


//TODO: FIX JSON
void to_json(json& j, const Item& item) {
    j = json{
        {"name", item.name},
        {"quantity", item.quantity},
        {"isRobotCompanion", item.isRobotCompanion},
        { "health", item.health },
        { "damage", item.damage},
        { "speed", item.speed }
    };
}

void from_json(const json& j, Item& item) {
    j.at("name").get_to(item.name);
    j.at("quantity").get_to(item.quantity);
    j.at("isRobotCompanion").get_to(item.isRobotCompanion);
    j.at("health").get_to(item.health);
    j.at("damage").get_to(item.damage);
    j.at("speed").get_to(item.speed);
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

void Inventory::moveItem(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= slots.size() || toIndex < 0 || toIndex >= slots.size()) {
        std::cerr << "Invalid slot indices." << std::endl;
        return;
    }

    // Move item from 'fromIndex' to 'toIndex' and clear the original slot
    slots[toIndex].item = slots[fromIndex].item;
    slots[fromIndex].item = {};  // Clear the original slot
}
