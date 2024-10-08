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

    //inventory is open or closed
    bool isOpen = false; 

private:
    std::vector<Item> items;  // List of items in the inventory
};
