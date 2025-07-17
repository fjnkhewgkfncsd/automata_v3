from db_config import db_config
import mysql.connector
import sys
import json

def insert_fa(json_file, db_config):
    try:
        # Read JSON file
        with open(json_file, 'r') as f:
            fa_data = json.load(f)
        
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor()
        
        # Extract basic info
        fa_name = fa_data.get('name', 'Unnamed')
        fa_type = fa_data.get('type', 'DFA')  # ✅ FIXED: Get type from JSON
        num_states = fa_data.get('numOfStates', 0)
        num_alphabet = fa_data.get('numOfAlphabet', 0)
        num_accepting = fa_data.get('numOfAcceptingStates', 0)
        start_state_name = fa_data.get('startState', '')
        
        print(f"Inserting {fa_type}: {fa_name}")  # Debug info
        
        # 1. Insert main automaton record with type
        cursor.execute("""
        INSERT INTO Automata (name, type, num_of_states, num_of_alphabet_symbols, num_of_accepting_states)
        VALUES (%s, %s, %s, %s, %s)
        """, (fa_name, fa_type, num_states, num_alphabet, num_accepting))
        
        automaton_id = cursor.lastrowid
        print(f"Created automaton with ID: {automaton_id}")
        
        # 2. Insert states
        state_ids = {}
        for state_name in fa_data.get('states', []):
            if state_name == 'nt':  # Skip "no transition" state
                continue
                
            cursor.execute("""
            INSERT INTO States (automaton_id, state_name)
            VALUES (%s, %s)
            """, (automaton_id, state_name))
            
            state_ids[state_name] = cursor.lastrowid
            
            # Set start state
            if state_name == start_state_name:
                cursor.execute("""
                UPDATE Automata SET start_state_id = %s WHERE automaton_id = %s
                """, (cursor.lastrowid, automaton_id))
        
        # 3. Insert alphabet symbols
        symbol_ids = {}
        for symbol in fa_data.get('alphabet', []):
            cursor.execute("""
            INSERT INTO AlphabetSymbols (automaton_id, symbol_value)
            VALUES (%s, %s)
            """, (automaton_id, symbol))
            
            symbol_ids[symbol] = cursor.lastrowid
        
        # 4. Insert accepting states
        for accepting_state in fa_data.get('acceptingStates', []):
            if accepting_state in state_ids:
                cursor.execute("""
                INSERT INTO AcceptingStates (automaton_id, state_id)
                VALUES (%s, %s)
                """, (automaton_id, state_ids[accepting_state]))
        
        # 5. Insert transitions (FIXED for both DFA and NFA)
        transition_count = 0
        for transition in fa_data.get('transitions', []):
            if len(transition) == 3:
                from_state, symbol, to_state = transition
                
                # Skip invalid transitions
                if (from_state not in state_ids or 
                    to_state not in state_ids or 
                    from_state == 'nt' or 
                    to_state == 'nt'):
                    continue
                
                # Handle epsilon transitions
                if symbol == 'e':
                    # Insert epsilon transition (symbol_id can be NULL)
                    cursor.execute("""
                    INSERT INTO Transitions (automaton_id, current_state_id, next_state_id, symbol_id)
                    VALUES (%s, %s, %s, %s)
                    """, (automaton_id, state_ids[from_state], state_ids[to_state],symbol_ids['e']))
                else:
                    # Regular transition
                    if symbol in symbol_ids:
                        cursor.execute("""
                        INSERT INTO Transitions (automaton_id, current_state_id, next_state_id, symbol_id)
                        VALUES (%s, %s, %s, %s)
                        """, (automaton_id, state_ids[from_state], state_ids[to_state], symbol_ids[symbol]))
                
                transition_count += 1
        
        # Commit all changes
        db.commit()
        print(f"✅ {fa_type} '{fa_name}' saved successfully with {transition_count} transitions.")
        
        cursor.close()
        db.close()
        return True
        
    except mysql.connector.Error as err:
        print(f"❌ Database error: {err}")
        if 'db' in locals():
            db.rollback()
        return False
    except json.JSONDecodeError as err:
        print(f"❌ JSON parsing error: {err}")
        return False
    except Exception as err:
        print(f"❌ Unexpected error: {err}")
        return False
    finally:
        if 'cursor' in locals():
            cursor.close()
        if 'db' in locals():
            db.close()


def list_DFA(db_config):
    try:
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor(dictionary=True)

        cursor.execute("SELECT automaton_id,name,type FROM automata WHERE type = 'DFA' order by automaton_id")

        automata = cursor.fetchall()
        if not automata:
            print("Empty")
            return None
        
        return automata
    except mysql.connector.Error as err:
        print(f"here ERROR: {err}")
        return None
    finally:
        if 'cursor' in locals():
            cursor.close()
        if 'db' in locals():
            db.close()

def list_fa(db_config):
    try:
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor(dictionary=True)

        cursor.execute("""
        SELECT 
            automaton_id,
            name,
            COALESCE(type, 'DFA') as type 
        FROM Automata 
        ORDER BY automaton_id
        """)
        
        automata = cursor.fetchall()
        if not automata:
            print("EMPTY")
            return None

        # Return the automata data instead of printing it
        return automata

    except mysql.connector.Error as err:
        print(f"ERROR: {err}")
        return None
    finally:
        if 'cursor' in locals():
            cursor.close()
        if 'db' in locals():
            db.close()
def list_NFA(db_config):
    try:
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor(dictionary=True)

        cursor.execute("select automaton_id, name, type from Automata where type = 'NFA' order by automaton_id")

        automata = cursor.fetchall()
        if not automata:
            print("Empty")
            return None
        
        return automata
    except mysql.connector.Error as err:
        print(f"ERROR: {err}")
        return None
    finally:
        if 'cursor' in locals():
            cursor.close()
        if 'db' in locals():
            db.close()
            
def load_fa(automaton_id, db_config):
    try:
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor()

        # 1. Load main automaton info with start state
        cursor.execute("""
        SELECT a.automaton_id, a.name, a.num_of_states, a.num_of_alphabet_symbols, 
                a.num_of_accepting_states, s.state_name as start_state
        FROM Automata a 
        LEFT JOIN States s ON a.start_state_id = s.state_id 
        WHERE a.automaton_id = %s
        """, (automaton_id,))  # ✅ Fixed: Added comma for tuple
        
        automaton = cursor.fetchone()
        if not automaton:
            print(f" Automaton with ID {automaton_id} not found.")
            return None

        # Extract main info
        fa_data = {
            "id": automaton[0],
            "name": automaton[1],
            "numOfStates": automaton[2],
            "numOfAlphabet": automaton[3],
            "numOfAcceptingStates": automaton[4],
            "startState": automaton[5]
        }

        # 2. Load all states
        cursor.execute("""
        SELECT s.state_name 
        FROM States s 
        WHERE s.automaton_id = %s
        ORDER BY s.state_id
        """, (automaton_id,))
        
        fa_data["states"] = [row[0] for row in cursor.fetchall()]

        # 3. Load alphabet symbols
        cursor.execute("""
        SELECT al.symbol_value 
        FROM AlphabetSymbols al 
        WHERE al.automaton_id = %s
        ORDER BY al.symbol_id
        """, (automaton_id,))
        
        fa_data["alphabet"] = [row[0] for row in cursor.fetchall()]

        # 4. Load accepting states using JOIN
        cursor.execute("""
        SELECT s.state_name 
        FROM AcceptingStates acc 
        JOIN States s ON acc.state_id = s.state_id 
        WHERE acc.automaton_id = %s
        ORDER BY s.state_name
        """, (automaton_id,))
        
        fa_data["acceptingStates"] = [row[0] for row in cursor.fetchall()]

        # 5. Load transitions using multiple JOINs
        cursor.execute("""
        SELECT s1.state_name as from_state, 
                al.symbol_value as symbol, 
                s2.state_name as to_state
        FROM Transitions t
        JOIN States s1 ON t.current_state_id = s1.state_id
        JOIN AlphabetSymbols al ON t.symbol_id = al.symbol_id  
        JOIN States s2 ON t.next_state_id = s2.state_id
        WHERE t.automaton_id = %s
        ORDER BY s1.state_name, al.symbol_value
        """, (automaton_id,))
        
        fa_data["transitions"] = [(row[0], row[1], row[2]) for row in cursor.fetchall()]
        return fa_data

    except mysql.connector.Error as err:
        print(f" Database error: {err}")
        return None
    finally:
        if 'cursor' in locals():
            cursor.close()
        if 'db' in locals():
            db.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python db_operation.py <command> [args]")
        sys.exit(1)

    command = sys.argv[1]

    if command == "insert":
        if len(sys.argv) != 3:
            print("Usage: python db_operation.py insert <json_file>")
            sys.exit(1)
        
        json_file = sys.argv[2]
        try:
            
            result = insert_fa(json_file, db_config)
            if result is not None:
                print("SUCCESS")
                sys.exit(0)
            else:
                print("FAILED")
                sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}")
    elif command == "load":
        if len(sys.argv) != 4:
            print("Usage: python db_operation.py load <automaton_id> <output_file>")
            sys.exit(1)
        
        automaton_id = int(sys.argv[2])
        output_file = sys.argv[3]
        
        try:
            fa_data = load_fa(automaton_id, db_config)
            if fa_data:
                with open(output_file, 'w') as f:
                    json.dump(fa_data, f, indent=2)
                sys.exit(0)
            else:
                print("NOT_FOUND")
                sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}")
    elif command == "list":
        try:
            automata = list_fa(db_config)
            if automata:
                result = {
                    "automata": [
                        {
                            "id": str(row["automaton_id"]),
                            "name": row["name"],
                            "type": row["type"]
                        }
                        for row in automata
                    ]
                }
                print(json.dumps(result))
                sys.exit(0)
            else:
                print("EMPTY")
                sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}")
            sys.exit(1)
    elif command == "listNFA":
        try:
            automata = list_NFA(db_config)
            if automata:
                result = {
                    "automata": [
                        {
                            "id": str(row["automaton_id"]),
                            "name": row["name"],
                            "type": row["type"]
                        }
                        for row in automata
                    ]
                }
                print(json.dumps(result))
                sys.exit(0)
            else:
                print("Empty")
                sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}")
            sys.exit(1)
    elif command == "listDFA":
        try:
            automata = list_DFA(db_config)
            if automata:
                result = {
                    "automata": [
                        {
                            "id": str(row["automaton_id"]),
                            "name": row["name"],
                            "type": row["type"]
                        }
                        for row in automata
                    ]
                }
                print(json.dumps(result))
                sys.exit(0)
            else:
                print(f"Empty")
                sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}")
            sys.exit(1)                        
    else:
        print("Unknown command. Use 'insert' or 'load'")
        sys.exit(1)
