#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <glm/glm.hpp> // For vec2

#include "../ext/json.hpp"
using json = nlohmann::json;

enum class InventorySlotType { GENERAL, ARMOR, WEAPON };

struct Item {
    std::string name;  // Name of the item
    int quantity = 1;       
    //companion robot stats
    bool isRobotCompanion = false; 
    int health = 0;         
    int damage = 0;           
    int speed = 0;
    Item() = default;

    // Constructor for regular items
    Item(const std::string& name, int quantity = 1)
        : name(name), quantity(quantity), isRobotCompanion(false) {}

    // Constructor for companion robots
    Item(const std::string& name, int health, int damage, int speed)
        : name(name), quantity(1), isRobotCompanion(true), health(health), damage(damage), speed(speed) {}
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
    void Inventory::addCompanionRobot(const std::string& name, int health, int damage, int speed);
    // Remove a specified quantity of an item
    void removeItem(const std::string& itemName, int quantity);

    // Display the contents of the inventory (in console for now)
    void display() const;

    // Set the selected slot index
    void setSelectedSlot(int slot);

    // Get the currently selected slot index
    int getSelectedSlot() const { return selectedSlot; }
    bool Inventory::containsItem(const std::string& itemName);
    // Get non-empty items
    std::vector<Item> getItems() const;
    static const std::vector<Item> disassembleItems;
    // Swap item from dragged slot to target slot
    void swapItems(int draggedSlot, int targetSlot);

    // Inventory is open or closed
    bool isOpen = false;

    void placeItemInSlot(int draggedSlotIndex, int targetSlotIndex);
    InventorySlot& getArmorSlot();
    InventorySlot& getWeaponSlot();
    Item getArmorItem();
    Item getWeaponItem();
    void moveItem(int fromSlot, int toSlot);

    std::vector<InventorySlot> slots; // List of inventory slots

    //getter for json
    int get_rows() const { return rows; }
    int get_columns() const{ return columns; }
    glm::vec2 get_slotSize()const { return slotSize; }



private:
    int selectedSlot = 0;             // Index of the currently selected slot
    int rows;
    int columns;
    glm::vec2 slotSize;               // Size of each slot for positioning
};


void to_json(json& j, const Item& item);
void from_json(const json& j, Item& item);
void to_json(json& j, const InventorySlot& slot);
void from_json(const json& j, InventorySlot& slot);
void to_json(json& j, const Inventory& inventory);
void from_json(const json& j, Inventory& inventory);