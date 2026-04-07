#include <SuperPupUtilities/Inventory.hpp>

#include <Canis/App.hpp>
#include <Canis/Debug.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/InputManager.hpp>

namespace SuperPupUtilities
{

    ScriptConf inventoryConf = {};

    void RegisterInventoryScript(App& _app)
    {
        DEFAULT_CONFIG(inventoryConf, Inventory);

        inventoryConf.DEFAULT_DRAW_INSPECTOR(Inventory,
            ImGui::Text("Slots:");

            const int slotCount = component->GetSlotCount();
            if (slotCount == 0)
            {
                ImGui::TextDisabled("none");
            }
            else
            {
                for (int i = 0; i < slotCount; i++)
                    ImGui::Text("%d: %s %d", i, component->GetSlotName(i).c_str(), component->GetSlotItemCount(i));
            }
        );

        _app.RegisterScript(inventoryConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(inventoryConf, Inventory)

    void Inventory::Create() {}

    void Inventory::Ready() {}

    void Inventory::Destroy() {}

    void Inventory::Update(float _dt) {}

    void Inventory::Add(I_Item &_item, int _amount) {
        Add(_item.GetName(), _amount);
    }

    void Inventory::Add(const std::string &_name, int _amount)
    {
        if (Slot* slot = GetSlot(_name)) {
            slot->count += _amount;
            ClampSelectedSlotIndex();
            return;
        }

        if (_amount <= 0)
            Debug::Warning("Called Inventory::Add with _amount: %i", _amount);
        
        Slot slot;
        slot.name = _name;
        slot.count = _amount;
        m_slots.push_back(slot);
        ClampSelectedSlotIndex();
    }

    bool Inventory::Remove(std::string _name, int _amount)
    {
        if (_amount <= 0)
        {
            Debug::Warning("Called Inventory::Remove with _amount: %i can not remove this amount", _amount);
            return false;
        }

        if (Slot* slot = GetSlot(_name))
        {
            if (slot->count - _amount < 0)
            {
                Debug::Warning("Called Inventory::Remove with _amount: %i can not remove this amount", _amount);
                return false;
            }

            slot->count -= _amount;

            if (slot->count == 0)
            {
                int index = -1;

                for (int i = 0; i < m_slots.size(); i++)
                    if (m_slots[i].name == _name)
                        index = i;

                if (index >= 0)
                    m_slots.erase(m_slots.begin() + index);
            }

            ClampSelectedSlotIndex();
            return true;
        }

        Debug::Warning("Inventory::Remove was called but item was not found in Inventory");
        return false;
    }

    bool Inventory::Remove(I_Item &_item, int _amount)
    {
        return Remove(_item.GetName(), _amount);
    }

    int Inventory::GetCount(I_Item &_item) {
        return GetCount(_item.GetName());
    }

    int Inventory::GetCount(const std::string &_name) const
    {
        if (const Slot* slot = GetSlot(_name))
            return slot->count;

        return 0;
    }

    int Inventory::GetSlotCount() const
    {
        return static_cast<int>(m_slots.size());
    }

    std::string Inventory::GetSlotName(int _index) const
    {
        if (_index < 0 || _index >= static_cast<int>(m_slots.size()))
            return "";

        return m_slots[static_cast<size_t>(_index)].name;
    }

    int Inventory::GetSlotItemCount(int _index) const
    {
        if (_index < 0 || _index >= static_cast<int>(m_slots.size()))
            return 0;

        return m_slots[static_cast<size_t>(_index)].count;
    }

    int Inventory::GetSelectedSlotIndex() const
    {
        if (m_slots.empty())
            return -1;

        return std::clamp(m_selectedSlotIndex, 0, static_cast<int>(m_slots.size()) - 1);
    }

    void Inventory::SetSelectedSlotIndex(int _index)
    {
        m_selectedSlotIndex = _index;
        ClampSelectedSlotIndex();
    }

    void Inventory::SelectRelative(int _delta)
    {
        if (m_slots.empty() || _delta == 0)
            return;

        const int slotCount = static_cast<int>(m_slots.size());
        int index = GetSelectedSlotIndex();
        if (index < 0)
            index = 0;

        index = (index + _delta) % slotCount;
        if (index < 0)
            index += slotCount;

        m_selectedSlotIndex = index;
    }

    Inventory::Slot* Inventory::GetSlot(std::string _name) {
        for (int i = 0; i < m_slots.size(); i++)
            if (m_slots[i].name == _name)
                return &m_slots[i];

        return nullptr;
    }

    const Inventory::Slot* Inventory::GetSlot(std::string _name) const
    {
        for (const Slot& slot : m_slots)
            if (slot.name == _name)
                return &slot;

        return nullptr;
    }

    void Inventory::ClampSelectedSlotIndex()
    {
        if (m_slots.empty())
        {
            m_selectedSlotIndex = 0;
            return;
        }

        m_selectedSlotIndex = std::clamp(m_selectedSlotIndex, 0, static_cast<int>(m_slots.size()) - 1);
    }
}
