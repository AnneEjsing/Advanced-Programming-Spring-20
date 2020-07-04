#include <list>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <functional>
#include <iostream>
#include <algorithm>
#include "timer.hpp"

#ifndef REACHABILITY_H // include guards
#define REACHABILITY_H

enum class search_order_t
{
    depth_first,
    breadth_first,
    cost_guided
};

template <typename state_t, typename successor_generator>
constexpr auto successors(const successor_generator &transitions) noexcept
{
    return [&transitions](const state_t &current_state) {
        std::list<state_t> all_successors{};
        for (auto &transition : transitions(current_state))
        {
            state_t new_succ = current_state;
            transition(new_succ);
            all_successors.push_back(new_succ);
        }
        return all_successors;
    };
};

// Default cost function
template <typename Cost_t, typename State_t>
const auto default_cost_function = [](const State_t &state, const Cost_t &prev_cost) { return 0; };

// Default invariant function
template <typename State_t>
const auto default_invariant = [](const State_t &state) { return true; };

// Class state_space_t
template <typename State_t, typename Cost_t, typename Successor_gen>
class state_space_t
{
    // Alias for Invariant_type and Cost_fn
    using Invariant_type = std::function<bool(const State_t &)>;
    using Cost_fn = std::function<Cost_t(const State_t &, const Cost_t &)>;


    State_t initial_state;
    Cost_t initial_cost;
    Successor_gen successors_function;
    Invariant_type invariant = default_invariant<State_t>;
    Cost_fn cost_function = default_cost_function<Cost_t, State_t>;
    Cost_t previous_cost = initial_cost;

public:
    explicit state_space_t(const State_t &state, const Successor_gen &succ, const Invariant_type &invariant_fn = default_invariant<State_t>) noexcept
        : initial_state{state},
          initial_cost{0},
          successors_function{succ},
          invariant{invariant_fn} {}
    explicit state_space_t(const State_t &state, const Cost_t &cost, const Successor_gen &succ, const Invariant_type &invariant_fn, const Cost_fn &cost_func) noexcept
        : initial_state{state},
          initial_cost{cost},
          successors_function{succ},
          invariant{invariant_fn},
          cost_function{cost_func} {}

    auto check(const std::function<bool(const State_t &)> &goal_pred, const search_order_t &search_order = search_order_t::breadth_first)
    {
        Timer timer;
        // Waiting holds all states waiting to be visited
        std::deque<State_t> waiting{initial_state};
        // Passed holds all states already visited
        std::vector<State_t> passed{};
        // Trace maps a state to its successor, s.t. the final solution can be computed from the trace
        std::map<State_t, State_t> trace{};
        while (!waiting.empty())
        {
            State_t curr_state = popstate(waiting, search_order);
            if (goal_pred(curr_state))
                return get_solution_from_trace(trace, curr_state);

            // If the current state is not in passed, add it to passed
            passed.push_back(curr_state);
            // Iterate through all successors
            for (auto &succ : successors_function(curr_state)) //Could have used const iterator
            {
                // If the state upholds the invariant and is neither in waiting nor passed, add it to trace and waiting
                if (invariant(succ) && std::find(waiting.begin(), waiting.end(), succ) == waiting.end() && std::find(passed.begin(), passed.end(), succ) == passed.end())
                {
                    trace[succ] = curr_state;
                    waiting.push_back(succ);
                }
            }
        }
        throw new std::logic_error("No solution could be found");
    }

private:
    State_t popstate(std::deque<State_t> &waiting, const search_order_t &search_order)
    {
        State_t state;
        // If the order is depth first, return the last state in waiting
        if (search_order == search_order_t::depth_first)
        {
            state = waiting.back();
            waiting.pop_back();
        }
        // If the order is breadth first, return the first state in waiting
        else if (search_order == search_order_t::breadth_first)
        {
            state = waiting.front();
            waiting.pop_front();
        }
        // If the order is cost guided, return the state in waiting with the lowest cost
        else if (search_order == search_order_t::cost_guided)
        {
            // Compute the cost for each state and put it in the "costs" vector.
            std::vector<Cost_t> costs{};
            std::transform(waiting.begin(), waiting.end(), std::back_inserter(costs), [waiting, this](const State_t &state) { return cost_function(state, previous_cost); });

            // Find the smallest cost and get its index in the costs vector
            auto index = std::distance(costs.begin(), std::min_element(costs.begin(), costs.end()));
            // Update previous_cost
            previous_cost = costs[index];
            // Get the state with the smallest cost and remove this entry from the waitings vector
            state = waiting[index];
            waiting.erase(waiting.begin() + index);
        }
        else
            throw new std::invalid_argument("The given search order is not supported.");

        return state;
    }

    auto get_solution_from_trace(std::map<State_t, State_t> &trace, State_t &curr_state)
    {
        std::list<State_t> solution{};
        State_t state{curr_state};
        solution.push_front(state);
        // Backtracks the trace from the goal state to the inital state
        while (!(state == initial_state))
        {
            state = trace[curr_state];
            solution.push_front(state);
            curr_state = state;
        }
        return solution;
    }
};

// Class Template Argument Deduction (CTAD)
template <typename State_t, typename Cost_t, typename Successor_gen, typename Invariant_type, typename Cost_fn>
state_space_t(const State_t &, const Cost_t &, const Successor_gen &, const Invariant_type &, const Cost_fn &)
    -> state_space_t<State_t, Cost_t, Successor_gen>;

template <typename State_t, typename Successor_gen, typename Invariant_type>
state_space_t(const State_t &, const Successor_gen &, const Invariant_type &)
    -> state_space_t<State_t, int, Successor_gen>;

template <typename State_t, typename Successor_gen>
state_space_t(const State_t &, const Successor_gen &)
    -> state_space_t<State_t, int, Successor_gen>;

//Using variadic template and fold (C++17)
template <typename... Args>
void log(Args... args)
{
    (std::cout << ... << args);
}

#endif //REACHABILITY_H