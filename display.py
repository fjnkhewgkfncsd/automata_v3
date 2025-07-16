from typing import Dict, Tuple, FrozenSet, Set, Optional, Any

# Conditional import and type handling
GRAPHVIZ_AVAILABLE = False
GraphType = Any  # Default type

try:
    from graphviz import Digraph
    GraphType = Digraph
    GRAPHVIZ_AVAILABLE = True
except ImportError:
    print("Note: Graphviz not installed - using text display only")

def display_automaton(
    states: Set[FrozenSet[str]], 
    start: FrozenSet[str], 
    finals: Set[FrozenSet[str]], 
    transitions: Dict[Tuple[FrozenSet[str], str], FrozenSet[str]], 
    name: str = "Automaton"
) -> Optional[Any]:
    """Visualize automaton using Graphviz (if available) or text output"""
    if not GRAPHVIZ_AVAILABLE:
        print("\nGraph visualization not available - displaying text representation instead:")
        print_automaton(states, start, finals, transitions, name)
        return None
        
    try:
        dot = Digraph(format='png')
        dot.attr(rankdir='LR', label=f"{name}\\n(States: {len(states)}, Transitions: {len(transitions)})")
        
        # Add states
        for state in states:
            label = format_state(state)
            if state in finals:
                dot.node(label, shape='doublecircle', color='green')
            else:
                dot.node(label)
            
            if state == start:
                dot.node('start', shape='none', label='')
                dot.edge('start', label)
        
        # Add transitions
        for (from_state, symbol), to_state in transitions.items():
            from_label = format_state(from_state)
            to_label = format_state(to_state)
            dot.edge(from_label, to_label, label=symbol)
        
        output_path = dot.render(f'automaton_{name}', view=True, cleanup=True)
        print(f"Visualization saved to: {output_path}")
        return dot
    except Exception as e:
        print(f"Visualization error: {e}")
        return None

def format_state(state: FrozenSet[str]) -> str:
    """Helper function to format a state (which might be a frozenset of strings)"""
    
    # This check is crucial for handling nested frozensets
    if isinstance(next(iter(state)), frozenset):
        # If the state is a frozenset of frozensets, format each inner frozenset recursively
        return ','.join(sorted([format_state(s) for s in state]))
    else:
        # Otherwise, assume it's a simple frozenset of strings
        if len(state) == 1 and isinstance(next(iter(state)), str):
            return next(iter(state))
        return ','.join(sorted(state))

def print_automaton(
    states: Set[FrozenSet[str]],
    start: FrozenSet[str],
    finals: Set[FrozenSet[str]],
    transitions: Dict[Tuple[FrozenSet[str], str], FrozenSet[str]],
    title: str = "Automaton"
) -> None:
    """Print automaton details to console"""
    print(f"\n{title} Summary:")
    
    # Format states
    formatted_states = [f"{{{format_state(s)}}}" for s in sorted(states, key=lambda x: sorted(format_state(x)))]
    print(f"States ({len(states)}):", ', '.join(formatted_states))
    
    # Format start state
    print("Start State:", f"{{{format_state(start)}}}")
    
    # Format final states
    formatted_finals = [f"{{{format_state(f)}}}" for f in sorted(finals, key=lambda x: sorted(format_state(x)))]
    print(f"Final States ({len(finals)}):", ', '.join(formatted_finals))
    
    print("\nTransitions:")
    for (from_state, symbol), to_state in sorted(transitions.items(), key=lambda x: (sorted(format_state(x[0][0])), x[0][1])):
        print(f"{{ {format_state(from_state)} }} --[{symbol}]--> {{ {format_state(to_state)} }}")