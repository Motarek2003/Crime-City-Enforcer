#pragma once

#include "../ecs/component.hpp"
#include <vector>
#include <string>

namespace our {

    class InventoryComponent : public Component {
    public:
        // Each slot contains a list of entity names that belong to that weapon set
        std::vector<std::vector<std::string>> slots;
        int activeSlot = 0;

        static std::string getID() { return "Inventory"; }

        void deserialize(const nlohmann::json& data) override {
            if(data.contains("slots")){
                slots = data["slots"].get<std::vector<std::vector<std::string>>>();
            }
            activeSlot = data.value("activeSlot", 0);
        }
    };

}
