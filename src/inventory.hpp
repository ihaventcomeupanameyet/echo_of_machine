#pragma once
#include <string>
#include <vector>
#include <iostream>

struct Item {
    std::string name;  // Name of the item
    int quantity;      // Quantity of the item
};


class Inventory {
public:
    // Add a specified quantity of an item
    void addItem(const std::string& itemName, int quantity);

    // Remove a specified quantity of an item
    void removeItem(const std::string& itemName, int quantity);

    // Display the contents of the inventory (in console for now)
    void display() const;

    // Getter for inventory items
    const std::vector<Item>& getItems() const { return items; }

    void setSelectedSlot(int slot) {
        if (slot >= 0 && slot < 3) selectedSlot = slot;
    }

    // Getter for selected slot
    int getSelectedSlot() const { return selectedSlot; }

    // Inventory is open or closed
    bool isOpen = false;

private:
    std::vector<Item> items;  // List of items in the inventory
    int selectedSlot = 0;     // Index of the currently selected slot (default to the first slot)
};