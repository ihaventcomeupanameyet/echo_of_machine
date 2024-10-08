#include "inventory.hpp"
#include <iostream>

void Inventory::addItem(const std::string& itemName, int quantity) {
	for (auto& item : items) {
		if (item.name == itemName) {
			item.quantity += quantity; // Check if item exists; if so, increase quantity
			return;
		}
	}
	// create new item if not 
	items.push_back({ itemName, quantity });
}

void Inventory::removeItem(const std::string& itemName, int quantity) {
	for (auto& item : items) {
		if (item.name == itemName) {
			item.quantity -= quantity; 
			if (item.quantity <= 0) {
				// remove the item if the quantity is zero or less
				items.erase(std::remove_if(items.begin(), items.end(),
					[&](const Item& i) { return i.name == itemName; }), items.end());
			}
			return;
		}
	}
}

void Inventory::display() const {
	if (items.empty()) {
		std::cout << "Inventory is empty." << std::endl;
		return;
	}
	std::cout << "Inventory:" << std::endl;
	for (const auto& item : items) {
		std::cout << "- " << item.name << ": " << item.quantity << std::endl;
	}
}
