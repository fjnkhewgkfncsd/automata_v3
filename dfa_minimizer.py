from typing import Set, Dict, Tuple, FrozenSet
from collections import defaultdict
import json
from display import display_automaton, print_automaton

def frozenset_to_list(obj):
    if isinstance(obj, frozenset) or isinstance(obj, set):
        return [frozenset_to_list(e) for e in obj]
    return obj
def minimize_dfa(
    states: Set[FrozenSet[str]],
    start: FrozenSet[str],
    finals: Set[FrozenSet[str]],
    transitions: Dict[Tuple[FrozenSet[str], str], FrozenSet[str]]
) -> Tuple[Set[FrozenSet[str]], FrozenSet[str], Set[FrozenSet[str]], Dict[Tuple[FrozenSet[str], str], FrozenSet[str]]]:
    
    # Initial partition
    partitions = {frozenset(finals), frozenset(states - finals)}
    waiting = set(partitions)
    
    while waiting:
        current = waiting.pop()
        
        # Get all transition symbols
        symbols = {sym for (_, sym) in transitions.keys()}
        
        for symbol in symbols:
            # Find states that transition into current set
            inverse = defaultdict(set)
            for (from_state, sym), to_state in transitions.items():
                if sym == symbol:
                    for part in partitions:
                        if to_state in part:
                            inverse[part].add(from_state)
                            break
            
            # Refine partitions
            new_partitions = set()
            for part in partitions:
                split1 = part & inverse.get(part, set())
                split2 = part - inverse.get(part, set())
                
                if split1 and split2:
                    new_partitions.add(frozenset(split1))
                    new_partitions.add(frozenset(split2))
                    
                    if part in waiting:
                        waiting.remove(part)
                        waiting.add(frozenset(split1))
                        waiting.add(frozenset(split2))
                    else:
                        waiting.add(frozenset(split1) if len(split1) <= len(split2) else waiting.add(frozenset(split2)))
                else:
                    new_partitions.add(part)
            
            partitions = new_partitions
    
    # Create state mapping
    state_map = {state: part for part in partitions for state in part}
    
    # Build new transitions
    new_transitions = {
        (state_map[from_state], sym): state_map[to_state]
        for (from_state, sym), to_state in transitions.items()
    }
    
    new_start = state_map[start]
    new_finals = {state_map[state] for state in finals}
    
    return set(partitions), new_start, new_finals, new_transitions

if __name__ == "__main__":
    with open("dfa_input.json") as f:
        data = json.load(f)
    states = set(data["states"])    
    start = data["startState"]
    finals = set(data["acceptingStates"])
    transitions = {}
    for from_state, symbol, to_state in data["transitions"]:
        transitions[(frozenset({from_state}), symbol)] = frozenset({to_state})

    frozen_states = {frozenset({s}) for s in states}
    frozen_start = frozenset({start})
    frozen_finals = {frozenset({f}) for f in finals}
    partitions, new_start, new_finals, new_transitions = minimize_dfa(frozen_states, frozen_start, frozen_finals, transitions)
    result = {
        "states": [frozenset_to_list(s) for s in partitions],
        "startState": frozenset_to_list(new_start),
        "acceptingStates": [frozenset_to_list(s) for s in new_finals],
        "transitions": [
            {"from": frozenset_to_list(k[0]), "symbol": k[1], "to": frozenset_to_list(v)}
            for k, v in new_transitions.items()
        ]
    }
    with open("minimized.json", "w") as f:
        json.dump(result, f, indent=4)
    print_automaton(partitions, new_start, new_finals, new_transitions, "Minimized DFA")
    
    show_png = input("Would you like to show the visualization? (y/n): ").lower()
    if show_png == 'y':
        display_automaton(partitions, new_start, new_finals, new_transitions, "Minimized_DFA")
