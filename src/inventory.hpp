#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <glm/glm.hpp> // For vec2

enum class InventorySlotType { GENERAL, ARMOR, WEAPON };

struct Item {
    std::string name;  // Name of the item
    int quantity;      // Quantity of the item
};

struct InventorySlot {
    InventorySlotType type;
    Item item;          // Item in the slot
    glm::vec2 position; // Position of the slot
};

class Inventory {
public:
    Inventory(int rows = 2, int columns = 5, glm::vec2 slotSize = glm::vec2(64.f, 64.f));

    // Add a specified quantity of an item
    void addItem(const std::string& itemName, int quantity);

    // Remove a specified quantity of an item
    void removeItem(const std::string& itemName, int quantity);

    // Display the contents of the inventory (in console for now)
    void display() const;

    // Set the selected slot index
    void setSelectedSlot(int slot);

    // Get the currently selected slot index
    int getSelectedSlot() const { return selectedSlot; }

    // Get non-empty items
    std::vector<Item> getItems() const;

    // Swap item from dragged slot to target slot
    void swapItems(int draggedSlot, int targetSlot);

    // Inventory is open or closed
    bool isOpen = false;

    void placeItemInSlot(int draggedSlotIndex, int targetSlotIndex);
    InventorySlot& getArmorSlot();
    InventorySlot& getWeaponSlot();
    Item getArmorItem();
    Item getWeaponItem();
    void useSelectedItem();
    void moveItem(int fromSlot, int toSlot);

    std::vector<InventorySlot> slots; // List of inventory slots

private:
    int selectedSlot = 0;             // Index of the currently selected slot
    int rows;
    int columns;
    glm::vec2 slotSize;               // Size of each slot for positioning
};
