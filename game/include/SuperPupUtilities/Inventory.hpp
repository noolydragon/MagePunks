#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/I_Item.hpp>

namespace SuperPupUtilities
{
    class Inventory : public Canis::ScriptableEntity
    {
    private:
        struct Slot {
            std::string name = "";
            int count = 0;
        };

        std::vector<Slot> m_slots = {};
        int m_selectedSlotIndex = 0;

        Slot* GetSlot(std::string _name);
        const Slot* GetSlot(std::string _name) const;
        void ClampSelectedSlotIndex();
    public:
        static constexpr const char* ScriptName = "SuperPupUtilities::Inventory";

        Inventory(Canis::Entity &_entity) : Canis::ScriptableEntity(_entity) {}

        void Create();
        void Ready();
        void Destroy();
        void Update(float _dt);

        void Add(I_Item &_item, int _amount);
        void Add(const std::string &_name, int _amount);
        bool Remove(std::string _name, int _amount);
        bool Remove(I_Item &_item, int _amount);

        int GetCount(I_Item &_item);
        int GetCount(const std::string &_name) const;
        int GetSlotCount() const;
        std::string GetSlotName(int _index) const;
        int GetSlotItemCount(int _index) const;
        int GetSelectedSlotIndex() const;
        void SetSelectedSlotIndex(int _index);
        void SelectRelative(int _delta);
    };

    extern void RegisterInventoryScript(Canis::App& _app);
    extern void UnRegisterInventoryScript(Canis::App& _app);
}
