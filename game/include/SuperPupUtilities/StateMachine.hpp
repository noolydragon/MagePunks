#pragma once

#include <string>
#include <vector>

namespace SuperPupUtilities
{
    class StateMachine;

    class State
    {
    public:
        explicit State(std::string _name);
        virtual ~State() = default;

        const std::string& GetName() const;
        StateMachine* GetStateMachine() const;

        virtual void Enter() {}
        virtual void Update(float _dt) = 0;
        virtual void Exit() {}

    private:
        friend class StateMachine;

        std::string m_name = "";
        StateMachine* m_stateMachine = nullptr;
    };

    class StateMachine
    {
    public:
        void ClearStates();
        void AddState(State& _state);
        bool ChangeState(const std::string& _stateName);
        void Update(float _dt);

        State* GetCurrentState() const;
        const std::string& GetCurrentStateName() const;

    private:
        std::vector<State*> m_states = {};
        State* m_currentState = nullptr;
    };
}
