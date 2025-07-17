from collections import deque
from typing import Set, Dict, FrozenSet, Tuple
import json
from display import display_automaton, print_automaton

def epsilon_closure(states: Set[str], transitions: Dict[str, Dict[str, Set[str]]]) -> FrozenSet[str]:
    closure = set(states)
    queue = deque(states)
    
    while queue:
        state = queue.popleft()
        for to_state in transitions.get(state, {}).get('e', set()):
            if to_state not in closure:
                closure.add(to_state)
                queue.append(to_state)
    
    return frozenset(closure)

def convert_nfa_to_dfa(
    states: Set[str],
    start: str,
    finals: Set[str],
    transitions: Dict[str, Dict[str, Set[str]]]
) -> Tuple[Set[FrozenSet[str]], FrozenSet[str], Set[FrozenSet[str]], Dict[Tuple[FrozenSet[str], str], FrozenSet[str]]]:
    
    dfa_transitions = {}
    dfa_states = set()
    dfa_finals = set()
    
    initial_state = epsilon_closure({start}, transitions)
    queue = deque([initial_state])
    
    while queue:
        current = queue.popleft()
        
        if current in dfa_states:
            continue
            
        dfa_states.add(current)
        
        if any(s in finals for s in current):
            dfa_finals.add(current)
        
        # Find all unique symbols (excluding epsilon)
        symbols = {sym for state in current 
                    for sym in transitions.get(state, {}).keys()
                    if sym != 'e'}
        
        for sym in symbols:
            next_states = set()
            for state in current:
                next_states.update(transitions.get(state, {}).get(sym, set()))
            
            if next_states:
                next_closure = epsilon_closure(next_states, transitions)
                dfa_transitions[(current, sym)] = next_closure
                
                if next_closure not in dfa_states:
                    queue.append(next_closure)
    
    return dfa_states, initial_state, dfa_finals, dfa_transitions

if __name__ == "__main__":
    with open("nfa_input.json") as f:
        data = json.load(f)
    states = set(data["states"])
    start = data["startState"]
    finals = set(data["acceptingStates"])
    transitions = {}
    for from_state, symbol, to_state in data["transitions"]:
        if from_state not in transitions:
            transitions[from_state] = {}
        if symbol not in transitions[from_state]:
            transitions[from_state][symbol] = set()
        transitions[from_state][symbol].add(to_state)
    dfa_states, initial_state, dfa_finals, dfa_transitions = convert_nfa_to_dfa(states, start, finals, transitions) 
    result = {
        "states": [list(s) for s in dfa_states],
        "startState": list(initial_state),
        "acceptingStates": [list(s) for s in dfa_finals],
        "transitions": [
            {"from": list(k[0]), "symbol": k[1], "to": list(v)}
            for k, v in dfa_transitions.items()
        ]
    }  
    with open("dfa_output.json", "w") as f:
        json.dump(result, f, indent=2)
    print("Converted NFA to DFA successfully!")
    print_automaton(dfa_states, initial_state, dfa_finals, dfa_transitions, "Converted DFA")    
    show_png = input("Would you like to show the visualization? (y/n): ").lower()
    if show_png == 'y':
            display_automaton(dfa_states, initial_state, dfa_finals, dfa_transitions, "DFA")